#pragma once

// #include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>

/**
 * The Identity Hasher Interface: IIdentityHasher
 * identity_hashing/
 *  "Does this blob have exactly this identity?"
 * 
 * @link https://en.wikipedia.org/wiki/Hash_function
 * @link https://en.wikipedia.org/wiki/Cryptographic_hash_function
 * @link https://www.geeksforgeeks.org/dsa/hash-functions-and-list-types-of-hash-functions/
 * @link https://asecuritysite.com/hash/index
 *
 * The Strategy interface declares operations common to all supported versions
 * of some algorithm.
 * @link https://refactoring.guru/design-patterns/strategy
 * @link https://refactoring.guru/design-patterns/strategy/cpp/example
 * The Context uses this interface to call the algorithm defined by Concrete
 * Strategies.
 *
 * In this project, BlobStore is the context and concrete hashers such as FNV,
 * BLAKE3, or SHA-256 are strategies. The blob store streams file data into the
 * interface without knowing which concrete algorithm is behind it.
 *
 * @author Boris A. Reif
 * @version 0.0.2
 */

namespace identity_hashing {

/**
 * @brief Common streaming interface for exact hash algorithms.
 *
 * A hasher is stateful: it starts in an initial state, accepts one or more
 * byte chunks through update(), and finally returns a digest through finalHex().
 * The interface is intentionally byte-oriented because blob data is arbitrary
 * binary data, not text.
 */
class IIdentityHasher {
public:
    /**
     * @brief Virtual destructor for safe deletion through base-class pointers.
     */
    virtual ~IIdentityHasher() = default;

    /**
     * @brief Return the stable algorithm name used in metadata.
     *
     * @return A reference to the concrete hasher's algorithm label, for example
     *         `fnv1a-64`, `sha256`, or `blake3-256`.
     */
    virtual const std::string& algorithm() const = 0;

    /**
     * @brief Feed the next chunk of bytes into the hash state.
     *
     * @param data Read-only view over the bytes to process. The hasher must not
     *        store this span beyond the call because the caller owns the memory.
     */
    virtual void update(std::span<const std::uint8_t> data) = 0;

    /**
     * @brief Produce the digest as hexadecimal text.
     *
     * @return Hexadecimal digest string. Implementations should make repeated
     *         calls safe when possible; for OpenSSL this is done by finalizing a
     *         copied context.
     */
    virtual std::string finalHex() = 0;
};

/**
 * @brief Factory function type that creates a fresh hasher instance.
 *
 * Hashers are stateful, so BlobStore asks each factory for a new object for
 * every put/verify operation rather than reusing one finalized object.
 */
using IdentityHasherFactory = std::function<std::unique_ptr<IIdentityHasher>()>;

} // namespace identity_hashing
