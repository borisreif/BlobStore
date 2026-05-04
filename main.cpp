#include "src/blobstore/BlobStore.hpp"
#include "src/hashing/FnvHasher.hpp"
#include "blobstore/BlobStore.hpp"
#include "hashing/Blake3Hasher.hpp"

#include <filesystem>
#include <iostream>
#include <exception>
#include <string>

int main(int argc, char* argv[]) {
    // Source - https://stackoverflow.com/a/51536462
    // Posted by Deepanshu, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-05-04, License - CC BY-SA 4.0
    if (__cplusplus == 202302L) std::cout << "C++23";
    else if (__cplusplus == 202002L) std::cout << "C++20";
    else if (__cplusplus == 201703L) std::cout << "C++17";
    else if (__cplusplus == 201402L) std::cout << "C++14";
    else if (__cplusplus == 201103L) std::cout << "C++11";
    else if (__cplusplus == 199711L) std::cout << "C++98";
    else std::cout << "pre-standard C++." << __cplusplus;
    std::cout << "\n";

    try {
        blobstore::BlobStore store(
            "MY_STORE",
            {
                hashing::makeFnv1a_64Factory()
            }
        );



        namespace fs = std::filesystem;

        fs::path inputPath{"test_file.pdf"};
        fs::path outputPath{"out/report_copy.pdf"};
        if (argc > 1) inputPath = fs::path{std::string{argv[1]}};
        if (argc > 2) outputPath = fs::path{std::string{argv[2]}};

        /*
        blobdb::BlobStore store{fs::path{"data_root"}};

        std::cout << "Current working directory: " << fs::current_path() << '\n';
        std::cout << "Input file: " << inputPath << '\n';

        const std::string id = store.putFile(inputPath);
        std::cout << "Blob id: " << id << '\n';
        std::cout << "Stored at: " << store.pathOf(id) << '\n';
        std::cout << "Exists? " << std::boolalpha << store.exists(id) << '\n';
        std::cout << "Size: " << store.size(id) << " bytes\n";
        std::cout << "Integrity OK? " << store.verify(id) << '\n';

        if (store.getToFile(id, outputPath)) {
            std::cout << "Copied blob to: " << outputPath << '\n';
        } else {
            std::cout << "Failed to copy blob to: " << outputPath << '\n';
            return 2;
        }*/

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
