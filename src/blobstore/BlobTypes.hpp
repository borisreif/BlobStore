#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace blobstore {

namespace fs = std::filesystem;

/**
 * @brief Exact hash digest produced by one normal hasher.
 *
 * A digest is stored together with the algorithm name so that the blob store
 * remains algorithm-agile. For example, two different algorithms may both
 * produce a string of hexadecimal characters, but `sha256:ABCD...` and
 * `blake3-256:ABCD...` are not the same kind of identifier.
 */
struct HashDigest {
    /** @brief Stable algorithm label, for example `fnv1a-64`, `sha256`, or `blake3-256`. */
    std::string algorithm;

    /** @brief Hexadecimal digest string, normalized to uppercase by BlobStore. */
    std::string hex;
};

/**
 * @brief Similarity/fuzzy digest produced by a fuzzy hasher.
 *
 * Fuzzy signatures are metadata only. They are useful for similarity search,
 * but they should not be used as canonical blob identities.
 */
struct FuzzyDigest {
    /** @brief Stable fuzzy-hash algorithm label, for example `ssdeep`. */
    std::string algorithm;

    /** @brief Algorithm-specific fuzzy signature string. It is not necessarily hexadecimal. */
    std::string signature;
};

/**
 * @brief Public identifier for a blob in this database-free layout.
 *
 * The current BlobStore can directly locate blobs by the primary/canonical
 * hash algorithm selected in the constructor. Other digests can be stored in
 * metadata, but without a database/index they are not enough to derive a path.
 */
struct BlobId {
    /** @brief Canonical algorithm name used to derive the storage path. */
    std::string algorithm;

    /** @brief Canonical hexadecimal digest used to derive the storage path. */
    std::string hex;
};

/**
 * @brief Result returned by BlobStore::putFile().
 *
 * This packages the new or existing blob id, the final physical data path, and
 * all digests computed while streaming the input file.
 */
struct PutResult {
    /** @brief Canonical blob identifier. */
    BlobId id;

    /** @brief True when the same blob was already present and no new DATA.BLB was written. */
    bool alreadyExisted = false;

    /** @brief Final path of the stored blob payload file. */
    fs::path dataPath;

    /** @brief Exact digests computed for this blob. */
    std::vector<HashDigest> digests;

    /** @brief Optional fuzzy signatures computed for this blob. */
    std::vector<FuzzyDigest> fuzzyDigests;
};

} // namespace blobstore
