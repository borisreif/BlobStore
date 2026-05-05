#include "blobstore/BlobStore.hpp"
#include "identity_hashing/FnvHasher.hpp"

#ifdef BLOBSTORE_WITH_BLAKE3
#include "identity_hashing/Blake3Hasher.hpp"
#endif

#ifdef BLOBSTORE_WITH_OPENSSL
#include "identity_hashing/Sha256OpenSSLHasher.hpp"
#endif

#ifdef BLOBSTORE_WITH_SSDEEP
#include "similarity_hashing/SsdeepHasher.hpp"
#endif

#ifdef BLOBSTORE_WITH_TLSH
#include "similarity_hashing/TlshHasher.hpp"
#endif

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Print the C++ language standard detected by the compiler.
 *
 * Source - https://stackoverflow.com/a/51536462
 * Posted by Deepanshu, modified by community. See post 'Timeline' for change history
 * Retrieved 2026-05-04, License - CC BY-SA 4.0
 *
 * This diagnostic is useful while learning because `std::span` requires C++20.
 */
static void printCppStandard() {
    if (__cplusplus == 202302L) std::cout << "C++23";
    else if (__cplusplus == 202002L) std::cout << "C++20";
    else if (__cplusplus == 201703L) std::cout << "C++17";
    else if (__cplusplus == 201402L) std::cout << "C++14";
    else if (__cplusplus == 201103L) std::cout << "C++11";
    else if (__cplusplus == 199711L) std::cout << "C++98";
    else std::cout << "pre-standard C++. __cplusplus=" << __cplusplus;
    std::cout << '\n';
}

/**
 * @brief Build the exact-hasher list used by the demo program.
 *
 * The first hasher is the canonical/path hasher. If BLAKE3 is enabled, it is
 * used first. If OpenSSL is enabled, SHA-256 is added as a secondary verifier.
 * If BLAKE3 is disabled, FNV-1a-64 is used as a dependency-free fallback.
 *
 * @return Exact hasher factories for BlobStore.
 */
static std::vector<identity_hashing::IdentityHasherFactory> makeIdentityHashers() {
    std::vector<identity_hashing::IdentityHasherFactory> hashers;

#ifdef BLOBSTORE_WITH_BLAKE3
    hashers.push_back(identity_hashing::makeBlake3Factory());
#else
    hashers.push_back(identity_hashing::makeFnv1a_64Factory());
#endif

#ifdef BLOBSTORE_WITH_OPENSSL
    hashers.push_back(identity_hashing::makeSha256OpenSSLFactory());
#endif

    return hashers;
}

/**
 * @brief Build the optional similarity-hasher list used by the demo program.
 * Similarity hashes are metadata only; they are not used for content identity.
 *
 * @return Similarity hasher factories for BlobStore, possibly empty.
 */
static std::vector<similarity_hashing::SimilarityHasherFactory> makeSimilarityHashers() {
    std::vector<similarity_hashing::SimilarityHasherFactory> similarityHashers;

#ifdef BLOBSTORE_WITH_SSDEEP
    similarityHashers.push_back(similarity_hashing::makeSsdeepFactory());
#endif

#ifdef BLOBSTORE_WITH_TLSH
    similarityHashers.push_back(similarity_hashing::makeTlshFactory());
#endif

    return similarityHashers;
}

/**
 * @brief Demo entry point for storing, verifying, and copying one blob file.
 *
 * @param argc Number of command-line arguments.
 * @param argv Optional arguments: input path and output copy path.
 * @return 0 on success, 1 on exceptions, 2 if copying the stored blob fails.
 */
int main(int argc, char* argv[]) {
    printCppStandard();

    try {
        namespace fs = std::filesystem;

        fs::path inputPath{"test_file.pdf"};
        fs::path outputPath{"out/report_copy.pdf"};

        if (argc > 1) {
            inputPath = fs::path{std::string{argv[1]}};
        }

        if (argc > 2) {
            outputPath = fs::path{std::string{argv[2]}};
        }

        blobstore::BlobStore store(
            "MY_STORE",
            makeIdentityHashers(),
            makeSimilarityHashers()
        );

        std::cout << "Current working directory: " << fs::current_path() << '\n';
        std::cout << "Input file: " << inputPath << '\n';

        const blobstore::PutResult result = store.putFile(inputPath);

        std::cout << "Blob id: "
                  << result.id.algorithm
                  << ':'
                  << result.id.hex
                  << '\n';

        std::cout << "Stored at: " << result.dataPath << '\n';
        std::cout << "Already existed? " << std::boolalpha << result.alreadyExisted << '\n';
        std::cout << "Exists? " << store.contains(result.id) << '\n';
        std::cout << "Integrity OK? " << store.verify(result.id) << '\n';

        for (const auto& digest : result.digests) {
            std::cout << "Exact digest: "
                      << digest.algorithm
                      << ':'
                      << digest.hex
                      << '\n';
        }

        for (const auto& similarity : result.similarityDigests) {
            std::cout << "Similarity digest: "
                      << similarity.algorithm
                      << ':'
                      << similarity.signature
                      << '\n';
        }

        if (store.copyToFile(result.id, outputPath)) {
            std::cout << "Copied blob to: " << outputPath << '\n';
        } else {
            std::cout << "Failed to copy blob to: " << outputPath << '\n';
            return 2;
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
