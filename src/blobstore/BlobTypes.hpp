#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace blobstore {

namespace fs = std::filesystem;

/**
 * @brief Exact hash digest produced by one identity hasher.
 *
 * A digest is stored together with the algorithm name so that the blob store
 * remains algorithm-agile. For example, two different algorithms may both
 * produce hexadecimal text, but `sha256:ABCD...` and `blake3-256:ABCD...` are
 * different identifiers.
 */
struct HashDigest {
    /** @brief Stable algorithm label, for example `fnv1a-64`, `sha256`, or `blake3-256`. */
    std::string algorithm;

    /** @brief Hexadecimal digest string, normalized to uppercase by BlobStore. */
    std::string hex;
};

/**
 * @brief Similarity digest produced by one similarity hasher.
 *
 * Similarity signatures are metadata only. They are useful for comparing blobs
 * with algorithms such as ssdeep or TLSH, but they should not be used as
 * canonical blob identities and should not determine the storage path.
 */
struct SimilarityDigest {
    /** @brief Stable similarity algorithm label, for example `ssdeep` or `tlsh`. */
    std::string algorithm;

    /** @brief Algorithm-specific signature string. It is not necessarily hexadecimal. */
    std::string signature;
};

/**
 * @brief Backward-compatible alias for older code/comments that used the term fuzzy.
 *
 * New code should prefer SimilarityDigest because the folder/interface names now
 * use the broader term similarity hashing.
 */
using FuzzyDigest = SimilarityDigest;

/**
 * @brief Public identifier for a blob in this database-free layout.
 *
 * The current BlobStore can directly locate blobs by the primary/canonical hash
 * algorithm selected in the constructor. Other digests can be stored in
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
 * all exact/similarity digests computed while streaming the input file.
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

    /** @brief Optional similarity signatures computed for this blob. */
    std::vector<SimilarityDigest> similarityDigests;

    /**
     * @brief Deprecated alias-style field is intentionally not stored here.
     *
     * Use similarityDigests. The old name fuzzyDigests was removed from the
     * active API to make the identity/similarity split clearer.
     */
};

} // namespace blobstore
