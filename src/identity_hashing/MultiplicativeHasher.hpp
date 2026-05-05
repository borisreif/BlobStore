#pragma once

#include "IIdentityHasher.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

/**
 * The Multiplicative hash function
 * 
 * @link https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 * @link https://fahadsultan.com/csc223/datastructs/hash_types.html
 * @link https://stackoverflow.com/questions/11871245/knuth-multiplicative-hash
 *
 * This file contains a small experimental streaming hasher of the form
 * `state = state * multiplier + byte`. It is not a cryptographic hash; it is a
 * compact test strategy for exercising the BlobStore hasher interface.
 *
 * @author Boris A. Reif
 * @version 0.0.2
 */

namespace identity_hashing {

/**
 * @brief Traits template specialized for each supported multiplicative width.
 * @tparam UInt Unsigned integer state type.
 */
template <typename UInt>
struct MultiplicativeTraits;

/** @brief Default parameters and output formatting for an 8-bit toy hasher. */
template <>
struct MultiplicativeTraits<std::uint8_t> {
    /** @brief Initial state. */
    static constexpr std::uint8_t defaultSeed = 37;

    /** @brief Multiplication factor. */
    static constexpr std::uint8_t defaultMultiplier = 131;

    /** @brief Number of output bits. */
    static constexpr int bits = 8;

    /** @brief Number of hexadecimal digits in the output. */
    static constexpr int hexWidth = 2;

    /** @brief Algorithm label stored in metadata. */
    static constexpr const char* algorithm = "mul-8";
};

/** @brief Default parameters and output formatting for a 16-bit toy hasher. */
template <>
struct MultiplicativeTraits<std::uint16_t> {
    /** @brief Initial state. */
    static constexpr std::uint16_t defaultSeed = 251;

    /** @brief Multiplication factor. */
    static constexpr std::uint16_t defaultMultiplier = 257;

    /** @brief Number of output bits. */
    static constexpr int bits = 16;

    /** @brief Number of hexadecimal digits in the output. */
    static constexpr int hexWidth = 4;

    /** @brief Algorithm label stored in metadata. */
    static constexpr const char* algorithm = "mul-16";
};

/** @brief Default parameters and output formatting for a 32-bit toy hasher. */
template <>
struct MultiplicativeTraits<std::uint32_t> {
    /** @brief Initial state. */
    static constexpr std::uint32_t defaultSeed = 2166136261u;

    /** @brief Multiplication factor. */
    static constexpr std::uint32_t defaultMultiplier = 16777619u;

    /** @brief Number of output bits. */
    static constexpr int bits = 32;

    /** @brief Number of hexadecimal digits in the output. */
    static constexpr int hexWidth = 8;

    /** @brief Algorithm label stored in metadata. */
    static constexpr const char* algorithm = "mul-32";
};

/** @brief Default parameters and output formatting for a 64-bit toy hasher. */
template <>
struct MultiplicativeTraits<std::uint64_t> {
    /** @brief Initial state. */
    static constexpr std::uint64_t defaultSeed = 14695981039346656037ull;

    /** @brief Multiplication factor. */
    static constexpr std::uint64_t defaultMultiplier = 1099511628211ull;

    /** @brief Number of output bits. */
    static constexpr int bits = 64;

    /** @brief Number of hexadecimal digits in the output. */
    static constexpr int hexWidth = 16;

    /** @brief Algorithm label stored in metadata. */
    static constexpr const char* algorithm = "mul-64";
};

/**
 * @brief Templated experimental multiplicative byte-stream hasher.
 *
 * @tparam UInt Unsigned integer type used as the state. Overflow is intentional
 *         modulo arithmetic.
 */
template <typename UInt>
class MultiplicativeHasher final : public IIdentityHasher {
public:
    static_assert(
        std::is_unsigned_v<UInt>,
        "MultiplicativeHasher requires an unsigned integer type"
    );

    /**
     * @brief Construct a multiplicative hasher.
     * @param seed Initial state.
     * @param multiplier Multiplication factor applied before adding each byte.
     */
    MultiplicativeHasher(
        UInt seed = MultiplicativeTraits<UInt>::defaultSeed,
        UInt multiplier = MultiplicativeTraits<UInt>::defaultMultiplier
    )
        : seed_(seed),
          multiplier_(multiplier),
          state_(seed),
          algorithm_(MultiplicativeTraits<UInt>::algorithm)
    {
    }

    /** @brief Return the algorithm label, for example `mul-64`. */
    const std::string& algorithm() const override {
        return algorithm_;
    }

    /** @brief Feed bytes into the multiplicative state. */
    void update(std::span<const std::uint8_t> data) override {
        for (std::uint8_t byte : data) {
            state_ = static_cast<UInt>(
                state_ * multiplier_ + static_cast<UInt>(byte)
            );
        }
    }

    /** @brief Return the current state as fixed-width uppercase hexadecimal. */
    std::string finalHex() override {
        std::ostringstream out;

        out << std::uppercase
            << std::hex
            << std::setw(MultiplicativeTraits<UInt>::hexWidth)
            << std::setfill('0')
            << static_cast<std::uint64_t>(state_);

        return out.str();
    }

    /** @brief Reset the hasher back to its seed state. */
    void reset() {
        state_ = seed_;
    }

private:
    /** @brief Initial state used by reset(). */
    UInt seed_;

    /** @brief Multiplication factor applied to the state for every byte. */
    UInt multiplier_;

    /** @brief Current hash state. */
    UInt state_;

    /** @brief Cached algorithm label. */
    std::string algorithm_;
};

/** @brief Convenient alias for an 8-bit toy multiplicative hasher. */
using Multiplicative8Hasher  = MultiplicativeHasher<std::uint8_t>;

/** @brief Convenient alias for a 16-bit toy multiplicative hasher. */
using Multiplicative16Hasher = MultiplicativeHasher<std::uint16_t>;

/** @brief Convenient alias for a 32-bit toy multiplicative hasher. */
using Multiplicative32Hasher = MultiplicativeHasher<std::uint32_t>;

/** @brief Convenient alias for a 64-bit toy multiplicative hasher. */
using Multiplicative64Hasher = MultiplicativeHasher<std::uint64_t>;

} // namespace identity_hashing
