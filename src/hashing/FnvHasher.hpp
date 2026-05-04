#pragma once

#ifndef HASHING_FNV_HASHER_HPP
#define HASHING_FNV_HASHER_HPP

#include "IHasher.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <type_traits>

/**
 * The Fowler–Noll–Vo hash function
 * Fowler–Noll–Vo (or FNV) is a non-cryptographic hash function created by 
 * Glenn Fowler, Landon Curt Noll, and Kiem-Phong Vo.
 *
 * Variant 1 and Variant 1a
 * 
 * @link https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 * @link https://iq.opengenus.org/fnv-hash-algorithm/
 *
 * This file implements FNV as a small templated test hasher. It is useful for
 * learning and for deliberately testing collision behavior, but it should not
 * be used as a long-term cryptographic blob identity.
 *
 * @author Boris A. Reif
 * @version 0.0.2
 */

namespace hashing {

/**
 * @brief Selects the byte-mixing order of the FNV algorithm.
 *
 * FNV-1 multiplies first and then XORs the next byte. FNV-1a XORs the next byte
 * first and then multiplies. FNV-1a is the variant most commonly used in simple
 * hash-table experiments.
 */
enum class FnvVariant {
    /** @brief Original FNV-1 order: multiply, then XOR byte. */
    Fnv1,

    /** @brief FNV-1a order: XOR byte, then multiply. */
    Fnv1a
};

/**
 * @brief Traits template specialized for each supported FNV integer width.
 * @tparam UInt Unsigned integer type that holds the FNV state.
 */
template <typename UInt>
struct FnvTraits;

/**
 * @brief FNV constants and output formatting information for 32-bit FNV.
 */
template <>
struct FnvTraits<std::uint32_t> {
    /** @brief 32-bit FNV prime, documented as 0x01000193. */
    static constexpr std::uint32_t prime = 0x01000193u;

    /** @brief 32-bit FNV offset basis, documented as 0x811C9DC5. */
    static constexpr std::uint32_t offsetBasis = 0x811C9DC5u;

    /** @brief Number of output bits. */
    static constexpr int bits = 32;

    /** @brief Number of hexadecimal digits needed to print the digest. */
    static constexpr int hexWidth = 8;
};

/**
 * @brief FNV constants and output formatting information for 64-bit FNV.
 */
template <>
struct FnvTraits<std::uint64_t> {
    /** @brief 64-bit FNV prime, documented as 0x00000100000001B3. */
    static constexpr std::uint64_t prime = 0x00000100000001B3ull;

    /** @brief 64-bit FNV offset basis, documented as 0xCBF29CE484222325. */
    static constexpr std::uint64_t offsetBasis = 0xCBF29CE484222325ull;

    /** @brief Number of output bits. */
    static constexpr int bits = 64;

    /** @brief Number of hexadecimal digits needed to print the digest. */
    static constexpr int hexWidth = 16;
};

/**
 * @brief Templated streaming FNV hasher.
 *
 * @tparam UInt Unsigned integer state type. Currently `std::uint32_t` or
 *         `std::uint64_t`.
 * @tparam Variant FNV-1 or FNV-1a byte mixing order.
 */
template <typename UInt, FnvVariant Variant>
class FnvHasher final : public IHasher {
public:
    static_assert(
        std::is_same_v<UInt, std::uint32_t> ||
        std::is_same_v<UInt, std::uint64_t>,
        "FnvHasher currently supports only uint32_t and uint64_t"
    );

    /**
     * @brief Construct a hasher in the standard offset-basis initial state.
     */
    FnvHasher();

    /**
     * @brief Return the algorithm label, for example `fnv1a-64`.
     */
    const std::string& algorithm() const override;

    /**
     * @brief Feed another byte chunk into the FNV state.
     * @param data Read-only bytes to incorporate into the hash.
     */
    void update(std::span<const std::uint8_t> data) override;

    /**
     * @brief Return the current FNV state as fixed-width uppercase hexadecimal.
     */
    std::string finalHex() override;

    /**
     * @brief Reset the hasher back to the standard offset-basis initial state.
     */
    void reset();

private:
    /** @brief Current FNV state. Unsigned overflow is intentional modulo arithmetic. */
    UInt state_;

    /** @brief Cached algorithm label. */
    std::string algorithm_;

    /** @brief Build the algorithm label from Variant and UInt width. */
    static std::string makeAlgorithmName();
};

/** @brief Convenient alias for 32-bit FNV-1. */
using Fnv1_32Hasher  = FnvHasher<std::uint32_t, FnvVariant::Fnv1>;

/** @brief Convenient alias for 64-bit FNV-1. */
using Fnv1_64Hasher  = FnvHasher<std::uint64_t, FnvVariant::Fnv1>;

/** @brief Convenient alias for 32-bit FNV-1a. */
using Fnv1a_32Hasher = FnvHasher<std::uint32_t, FnvVariant::Fnv1a>;

/** @brief Convenient alias for 64-bit FNV-1a. */
using Fnv1a_64Hasher = FnvHasher<std::uint64_t, FnvVariant::Fnv1a>;

/** @brief Factory for fresh 32-bit FNV-1 hashers. */
HasherFactory makeFnv1_32Factory();

/** @brief Factory for fresh 64-bit FNV-1 hashers. */
HasherFactory makeFnv1_64Factory();

/** @brief Factory for fresh 32-bit FNV-1a hashers. */
HasherFactory makeFnv1a_32Factory();

/** @brief Factory for fresh 64-bit FNV-1a hashers. */
HasherFactory makeFnv1a_64Factory();

} // namespace hashing

#include "FnvHasher.tpp"

#endif // HASHING_FNV_HASHER_HPP
