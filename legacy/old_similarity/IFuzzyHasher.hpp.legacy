#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>

namespace hashing {

/**
 * @brief Common streaming interface for fuzzy/similarity hash algorithms.
 *
 * Fuzzy hashes are different from exact hashes. They should not be used as
 * canonical blob identities; they are metadata that can help detect similar
 * blobs. ssdeep and TLSH are examples of algorithms that could implement this
 * interface.
 */
class IFuzzyHasher {
public:
    /** @brief Virtual destructor for safe deletion through base-class pointers. */
    virtual ~IFuzzyHasher() = default;

    /**
     * @brief Return the stable fuzzy algorithm name used in metadata.
     * @return Algorithm label, for example `ssdeep`.
     */
    virtual const std::string& algorithm() const = 0;

    /**
     * @brief Feed another chunk of blob bytes into the fuzzy hasher.
     * @param data Read-only bytes to process.
     */
    virtual void update(std::span<const std::uint8_t> data) = 0;

    /**
     * @brief Return the final fuzzy signature string.
     *
     * The returned value is algorithm-specific and is not necessarily
     * hexadecimal.
     */
    virtual std::string finalSignature() = 0;

    /** @brief Reset the fuzzy hasher to an empty state. */
    virtual void reset() = 0;
};

/**
 * @brief Factory function type that creates fresh fuzzy hasher instances.
 */
using FuzzyHasherFactory = std::function<std::unique_ptr<IFuzzyHasher>()>;

} // namespace hashing
