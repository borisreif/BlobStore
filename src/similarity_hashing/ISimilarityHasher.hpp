#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>

namespace similarity_hashing {

/**
 * @brief Interface for hash-like algorithms that describe similarity,
 *        not exact identity.
 * similarity_hashing/
 *   "Is this blob similar to another blob?"
 * Similarity hashers are used to produce metadata that helps compare blobs
 * for similarity. They must not be used as canonical blob identities.
 * Examples include ssdeep and TLSH.
 *
 * A normal cryptographic hasher such as BLAKE3 or SHA-256 answers:
 *
 *     "Are these bytes exactly identical?"
 *
 * A similarity hasher such as TLSH answers:
 *
 *     "Are these byte streams similar?"
 *
 * Similarity hashes should be stored as metadata. They should not be used
 * as canonical blob IDs and should not determine the storage path.
 */
class ISimilarityHasher {
public:
    /**
     * @brief Virtual destructor for safe deletion through base-class pointers.
     */
    virtual ~ISimilarityHasher() = default;

    /**
     * @brief Returns the algorithm name.
     *
     * Example values:
     *
     *     "tlsh"
     *     "ssdeep"
     *
     * @return Stable algorithm name.
     */
    virtual const std::string& algorithm() const = 0;

    /**
     * @brief Feeds another chunk of raw blob bytes into the hasher.
     *
     * The blob store calls this repeatedly while streaming a file.
     *
     * @param data Read-only byte span containing the next file chunk.
     */
    virtual void update(std::span<const std::uint8_t> data) = 0;

    /**
     * @brief Finishes the similarity hash and returns its signature string.
     *
     * Unlike cryptographic hashes, the result is not necessarily hexadecimal.
     * For TLSH it is usually a TLSH digest string such as "T1...".
     *
     * @return Similarity signature string.
     */
    virtual std::string finalSignature() = 0;

    /**
     * @brief Resets the hasher so it can process a new byte stream.
     */
    virtual void reset() = 0;
};

/**
 * @brief Factory function type for creating fresh similarity hashers.
 *
 * Hashers are stateful, so the blob store should create a new hasher for
 * each put/verify operation.
 */
using SimilarityHasherFactory =
    std::function<std::unique_ptr<ISimilarityHasher>()>;

} // namespace similarity_hashing