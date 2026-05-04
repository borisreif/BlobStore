#pragma once

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <type_traits>

// https://iq.opengenus.org/fibonacci-hashing/
// https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
//
// Fibonacci hashing is used here as an integer-to-bucket indexing helper. It is
// not a cryptographic hash and it is not a complete byte-stream blob hasher by
// itself.

namespace hashing::fib {

/**
 * @brief Traits template specialized for Fibonacci-hashing word sizes.
 * @tparam UInt Unsigned integer type used as the input/mixed word.
 */
template <typename UInt>
struct FibonacciTraits;

/** @brief 32-bit Fibonacci hashing constant and word-size metadata. */
template <>
struct FibonacciTraits<std::uint32_t> {
    /** @brief 32-bit golden-ratio multiplier. */
    static constexpr std::uint32_t constant = 0x9E3779B9u;

    /** @brief Number of bits in the supported word type. */
    static constexpr unsigned bits = 32;
};

/** @brief 64-bit Fibonacci hashing constant and word-size metadata. */
template <>
struct FibonacciTraits<std::uint64_t> {
    /** @brief 64-bit golden-ratio multiplier. */
    static constexpr std::uint64_t constant = 0x9E3779B97F4A7C15ull;

    /** @brief Number of bits in the supported word type. */
    static constexpr unsigned bits = 64;
};

/**
 * @brief Mix an integer key by multiplying it with the Fibonacci constant.
 *
 * @tparam UInt `std::uint32_t` or `std::uint64_t`.
 * @param key Integer key or already-computed integer hash value.
 * @return Mixed integer word. Unsigned overflow is intentional.
 */
template <typename UInt>
UInt mix(UInt key) {
    static_assert(
        std::is_same_v<UInt, std::uint32_t> ||
        std::is_same_v<UInt, std::uint64_t>,
        "Fibonacci hashing currently supports uint32_t and uint64_t"
    );

    return key * FibonacciTraits<UInt>::constant;
}

/**
 * @brief Map an integer key to a power-of-two table index.
 *
 * The table size is `2^indexBits`. The function keeps the high bits of the
 * multiplied word, which usually distribute patterned integer keys better than
 * simply taking the low bits.
 *
 * @tparam UInt `std::uint32_t` or `std::uint64_t`.
 * @param key Integer key or already-computed integer hash value.
 * @param indexBits Number of high bits to keep; table size is `2^indexBits`.
 * @return Bucket index in the range `[0, 2^indexBits)`.
 * @throws std::invalid_argument if indexBits is larger than the word size.
 */
template <typename UInt>
std::size_t index(UInt key, unsigned indexBits) {
    static_assert(
        std::is_same_v<UInt, std::uint32_t> ||
        std::is_same_v<UInt, std::uint64_t>,
        "Fibonacci hashing currently supports uint32_t and uint64_t"
    );

    if (indexBits == 0) {
        return 0;
    }

    if (indexBits > FibonacciTraits<UInt>::bits) {
        throw std::invalid_argument("indexBits is too large for this integer type");
    }

    UInt mixed = mix(key);

    return static_cast<std::size_t>(
        mixed >> (FibonacciTraits<UInt>::bits - indexBits)
    );
}

} // namespace hashing::fib
