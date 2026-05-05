#pragma once

#include "../identity_hashing/IIdentityHasher.hpp"
#include "../similarity_hashing/ISimilarityHasher.hpp"
#include "BlobTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace blobstore {

namespace fs = std::filesystem;

/**
 * @brief Generates byte-derived fingerprints for blob payloads.
 *
 * BlobFingerprintGenerator is responsible for the streaming work that turns a
 * source file into automatically generated blob-level facts:
 *
 * - total byte size
 * - exact identity hashes such as BLAKE3 or SHA-256
 * - optional similarity signatures such as ssdeep or TLSH
 *
 * The class intentionally does not know about the final object directory,
 * `DATA.BLB`, `META.TXT`, manifests, user metadata, or ingestion logs. It only
 * observes the bytes of a file and derives fingerprints from those bytes.
 *
 * This keeps BlobStore closer to a facade: BlobStore coordinates storage, while
 * BlobFingerprintGenerator performs the hashing/fingerprinting pass.
 */
class BlobFingerprintGenerator {
public:
    /** @brief Default file I/O buffer size used while streaming payload bytes. */
    static constexpr std::size_t DefaultIoChunkSize = 64 * 1024;

    /**
     * @brief Construct a generator from hasher factories.
     *
     * @param identityHashers Exact identity hashers. The first one is normally
     *        the canonical/path hasher chosen by BlobStore.
     * @param similarityHashers Optional similarity hashers used only for
     *        derived metadata.
     * @param ioChunkSize Number of bytes read from disk at a time.
     *
     * @throws BlobStoreError if no identity hasher is supplied, if a factory
     *         returns nullptr, or if ioChunkSize is zero.
     */
    BlobFingerprintGenerator(
        std::vector<identity_hashing::IdentityHasherFactory> identityHashers,
        std::vector<similarity_hashing::SimilarityHasherFactory> similarityHashers = {},
        std::size_t ioChunkSize = DefaultIoChunkSize
    );

    /**
     * @brief Return the algorithm name of the primary identity hasher.
     *
     * The primary algorithm is the first identity hasher passed to the
     * constructor. BlobStore uses it to derive object paths.
     *
     * @return Stable algorithm label, for example `blake3-256` or `sha256`.
     */
    const std::string& primaryAlgorithm() const;

    /**
     * @brief Fingerprint an existing file without copying it.
     *
     * This is useful for verification: the stored `DATA.BLB` is streamed
     * through the configured hashers and the resulting values are returned.
     *
     * @param sourcePath Existing regular file to fingerprint.
     * @return Byte-derived fingerprint information.
     */
    BlobFingerprint generateForFile(const fs::path& sourcePath) const;

    /**
     * @brief Fingerprint a file while copying its bytes to a temporary path.
     *
     * This is useful for insertion: the input file is streamed once, all hashes
     * are computed, and the exact same bytes are written to `tempPayloadPath`.
     * The caller can later commit that temp file into the object store.
     *
     * @param sourcePath Existing regular file to read.
     * @param tempPayloadPath Temporary output file to create/overwrite.
     * @return Byte-derived fingerprint information.
     */
    BlobFingerprint generateForFileAndCopyTo(
        const fs::path& sourcePath,
        const fs::path& tempPayloadPath
    ) const;

private:
    /** @brief Exact hasher factories copied from BlobStore configuration. */
    std::vector<identity_hashing::IdentityHasherFactory> identityHasherFactories_;

    /** @brief Similarity hasher factories copied from BlobStore configuration. */
    std::vector<similarity_hashing::SimilarityHasherFactory> similarityHasherFactories_;

    /** @brief Algorithm name of the first identity hasher. */
    std::string primaryAlgorithm_;

    /** @brief Number of bytes read from disk at a time. */
    std::size_t ioChunkSize_;

    /** @brief Create one fresh identity hasher from every configured factory. */
    std::vector<std::unique_ptr<identity_hashing::IIdentityHasher>>
    makeIdentityHashers() const;

    /** @brief Create one fresh similarity hasher from every configured factory. */
    std::vector<std::unique_ptr<similarity_hashing::ISimilarityHasher>>
    makeSimilarityHashers() const;

    /**
     * @brief Shared streaming implementation for fingerprinting with optional copy.
     *
     * When tempPayloadPath has a value, the bytes are also written to that file.
     * When it is empty, the file is only fingerprinted.
     */
    BlobFingerprint generate(
        const fs::path& sourcePath,
        const std::optional<fs::path>& tempPayloadPath
    ) const;
};

} // namespace blobstore
