#pragma once

#include "../identity_hashing/IIdentityHasher.hpp"
#include "../similarity_hashing/ISimilarityHasher.hpp"
#include "BlobErrors.hpp"
#include "BlobFingerprintGenerator.hpp"
#include "BlobTypes.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace blobstore {

namespace fs = std::filesystem;

/**
 * @brief Database-free content-addressed blob store.
 *
 * BlobStore stores each file under a deterministic path derived from the first
 * exact hasher passed to the constructor. The payload is stored as `DATA.BLB`;
 * exact hashes and similarity signatures are stored next to it in `META.TXT`.
 *
 * The first hasher is the canonical/path hasher. Additional exact hashers are
 * verification metadata. Similarity hashers are optional metadata only.
 */
class BlobStore {
public:
    /**
     * @brief Construct a blob store rooted at a filesystem directory.
     *
     * @param root Store root. `OBJECTS` and `TMP` directories are created under it.
     * @param identityHashers Exact identity hashers. The first one becomes the canonical path hasher.
     * @param similarityHashers Optional similarity hashers used only for metadata.
     * @param pathChunkChars Number of hex characters per directory component.
     *
     * @throws BlobStoreError if no exact hasher is provided or if pathChunkChars is invalid.
     */
    BlobStore(
        fs::path root,
        std::vector<identity_hashing::IdentityHasherFactory> identityHashers,
        std::vector<similarity_hashing::SimilarityHasherFactory> similarityHashers = {},
        std::size_t pathChunkChars = 8
    );

    /**
     * @brief Store a file as a content-addressed blob.
     *
     * The input file is streamed through all configured hashers and written to a
     * temporary file first. If the final canonical path already exists, the
     * temporary file is byte-compared against the existing blob to distinguish a
     * duplicate from a true hash collision.
     *
     * @param sourcePath Path to an existing regular file.
     * @return Details about the stored or already-existing blob.
     *
     * @throws BlobStoreError for I/O failures.
     * @throws HashCollisionError if the canonical hash collides with different bytes.
     */
    PutResult putFile(const fs::path& sourcePath);

    /**
     * @brief Return the physical DATA.BLB path for a canonical BlobId.
     *
     * @param id Blob id using the primary algorithm selected by the constructor.
     * @return Deterministic path of the payload file.
     *
     * @throws BlobStoreError if id.algorithm is not the primary algorithm.
     */
    fs::path pathFor(const BlobId& id) const;

    /**
     * @brief Test whether the payload file for a canonical BlobId exists.
     * @param id Blob id using the primary algorithm.
     * @return True if DATA.BLB exists at the derived path.
     */
    bool contains(const BlobId& id) const;

    /**
     * @brief Recompute configured exact hashes and compare them with META.TXT.
     *
     * @param id Blob id using the primary algorithm.
     * @return True if the payload exists and every configured hash found in
     *         metadata matches the recomputed value.
     */
    bool verify(const BlobId& id) const;

    /**
     * @brief Copy a stored blob payload to another filesystem path.
     *
     * @param id Blob id using the primary algorithm.
     * @param outputPath Destination path. Parent directories are created if needed.
     * @return True when the payload existed and the copy succeeded.
     */
    bool copyToFile(const BlobId& id, const fs::path& outputPath) const;

    /**
     * @brief Read all exact hash digests from a blob's META.TXT.
     * @param id Blob id using the primary algorithm.
     * @return Exact hash digests found in metadata.
     */
    std::vector<HashDigest> readMetaFor(const BlobId& id) const;

    /**
     * @brief Read all similarity signatures from a blob's META.TXT.
     * @param id Blob id using the primary algorithm.
     * @return Similarity signatures found in metadata.
     */
    std::vector<SimilarityDigest> readSimilarityMetaFor(const BlobId& id) const;

private:
    /** @brief Store root supplied by the user. */
    fs::path root_;

    /** @brief Directory containing content-addressed object subdirectories. */
    fs::path objectsDir_;

    /** @brief Directory used for temporary files before atomic-ish renames. */
    fs::path tmpDir_;

    /**
     * @brief Generates byte-derived fingerprints for source files and stored payloads.
     *
     * This object owns the configured identity/similarity hasher factories and
     * performs the streaming hash pass. BlobStore delegates fingerprinting to it
     * instead of doing hashing itself.
     */
    BlobFingerprintGenerator fingerprintGenerator_;

    /** @brief Algorithm label of the canonical/path hasher. */
    std::string primaryAlgorithm_;

    /** @brief Number of digest hex characters used per path directory component. */
    std::size_t pathChunkChars_;

    /** @brief Convert a canonical hex digest into the object directory path. */
    fs::path objectDirForDigest(const std::string& hexDigest) const;

    /** @brief Convert a canonical hex digest into the DATA.BLB path. */
    fs::path dataPathForDigest(const std::string& hexDigest) const;

    /** @brief Convert a canonical hex digest into the META.TXT path. */
    fs::path metaPathForDigest(const std::string& hexDigest) const;

    /** @brief Create a unique temporary path under TMP. */
    fs::path makeTempPath() const;

    /**
     * @brief Write META.TXT for a stored blob.
     * @param primaryDigest Canonical digest used to locate the object directory.
     * @param size Payload size in bytes.
     * @param digests Exact digests to write as `hash.<algorithm>=...` lines.
     * @param similarityDigests Similarity signatures to write as `similarity.<algorithm>=...` lines.
     */
    void writeMeta(
        const std::string& primaryDigest,
        std::uintmax_t size,
        const std::vector<HashDigest>& digests,
        const std::vector<SimilarityDigest>& similarityDigests
    ) const;

    /** @brief Compare two files byte-for-byte without loading them entirely into memory. */
    static bool equalFiles(const fs::path& a, const fs::path& b);

    /** @brief Normalize hexadecimal text to uppercase for stable paths and metadata. */
    static std::string normalizeHex(std::string hex);

    /** @brief Throw if a BlobId does not use the configured primary algorithm. */
    static void requireCanonicalId(
        const BlobId& id,
        const std::string& primaryAlgorithm
    );
};

} // namespace blobstore
