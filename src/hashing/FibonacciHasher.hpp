#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>



// https://iq.opengenus.org/fibonacci-hashing/
// https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/

namespace hashing::fib {

template <typename UInt>
struct FibonacciTraits;

template <>
struct FibonacciTraits<std::uint32_t> {
    static constexpr std::uint32_t constant = 0x9E3779B9u;
    static constexpr unsigned bits = 32;
};

template <>
struct FibonacciTraits<std::uint64_t> {
    static constexpr std::uint64_t constant = 0x9E3779B97F4A7C15ull;
    static constexpr unsigned bits = 64;
};

template <typename UInt>
UInt mix(UInt key) {
    static_assert(
        std::is_same_v<UInt, std::uint32_t> ||
        std::is_same_v<UInt, std::uint64_t>,
        "Fibonacci hashing currently supports uint32_t and uint64_t"
    );

    return key * FibonacciTraits<UInt>::constant;
}

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