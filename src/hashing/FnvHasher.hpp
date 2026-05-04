#pragma once

#ifndef HASHING_FNV_HASHER_HPP
#define HASHING_FNV_HASHER_HPP

#include "IHasher.hpp"

#include <cstdint>
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
 *
 * @author Boris A. Reif
 * @version 0.0.1
 */


namespace hashing {

enum class FnvVariant {
    Fnv1,
    Fnv1a
};

template <typename UInt>
struct FnvTraits;

template <>
struct FnvTraits<std::uint32_t> {
    static constexpr std::uint32_t prime =
        0x01000193u;

    static constexpr std::uint32_t offsetBasis =
        0x811C9DC5u;

    static constexpr int bits = 32;
    static constexpr int hexWidth = 8;
};

template <>
struct FnvTraits<std::uint64_t> {
    static constexpr std::uint64_t prime =
        0x00000100000001B3ull;

    static constexpr std::uint64_t offsetBasis =
        0xCBF29CE484222325ull;

    static constexpr int bits = 64;
    static constexpr int hexWidth = 16;
};

template <typename UInt, FnvVariant Variant>
class FnvHasher final : public IHasher {
public:
    static_assert(
        std::is_same_v<UInt, std::uint32_t> ||
        std::is_same_v<UInt, std::uint64_t>,
        "FnvHasher currently supports only uint32_t and uint64_t"
    );

    FnvHasher();

    const std::string& algorithm() const override;

    void update(
        std::span<const std::uint8_t> data
        //const std::uint8_t* data,
        //std::size_t size
    ) override;

    std::string finalHex() override;

    void reset();

private:
    UInt state_;
    std::string algorithm_;

    static std::string makeAlgorithmName();
};

using Fnv1_32Hasher  = FnvHasher<std::uint32_t, FnvVariant::Fnv1>;
using Fnv1_64Hasher  = FnvHasher<std::uint64_t, FnvVariant::Fnv1>;
using Fnv1a_32Hasher = FnvHasher<std::uint32_t, FnvVariant::Fnv1a>;
using Fnv1a_64Hasher = FnvHasher<std::uint64_t, FnvVariant::Fnv1a>;

HasherFactory makeFnv1_32Factory();
HasherFactory makeFnv1_64Factory();
HasherFactory makeFnv1a_32Factory();
HasherFactory makeFnv1a_64Factory();

} // namespace hashing

#include "FnvHasher.tpp"

#endif // HASHING_FNV_HASHER_HPP