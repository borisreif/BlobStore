#pragma once

#include "IHasher.hpp"

#include <cstdint>
#include <iomanip>
#include <limits>
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
 *
 * @author Boris A. Reif
 * @version 0.0.1
 */

namespace hashing {

template <typename UInt>
struct MultiplicativeTraits;

template <>
struct MultiplicativeTraits<std::uint8_t> {
    static constexpr std::uint8_t defaultSeed = 37;
    static constexpr std::uint8_t defaultMultiplier = 131;
    static constexpr int bits = 8;
    static constexpr int hexWidth = 2;
    static constexpr const char* algorithm = "mul-8";
};

template <>
struct MultiplicativeTraits<std::uint16_t> {
    static constexpr std::uint16_t defaultSeed = 251;
    static constexpr std::uint16_t defaultMultiplier = 257;
    static constexpr int bits = 16;
    static constexpr int hexWidth = 4;
    static constexpr const char* algorithm = "mul-16";
};

template <>
struct MultiplicativeTraits<std::uint32_t> {
    static constexpr std::uint32_t defaultSeed = 2166136261u;
    static constexpr std::uint32_t defaultMultiplier = 16777619u;
    static constexpr int bits = 32;
    static constexpr int hexWidth = 8;
    static constexpr const char* algorithm = "mul-32";
};

template <>
struct MultiplicativeTraits<std::uint64_t> {
    static constexpr std::uint64_t defaultSeed = 14695981039346656037ull;
    static constexpr std::uint64_t defaultMultiplier = 1099511628211ull;
    static constexpr int bits = 64;
    static constexpr int hexWidth = 16;
    static constexpr const char* algorithm = "mul-64";
};

template <typename UInt>
class MultiplicativeHasher final : public IHasher {
public:
    static_assert(
        std::is_unsigned_v<UInt>,
        "MultiplicativeHasher requires an unsigned integer type"
    );

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

    const std::string& algorithm() const override {
        return algorithm_;
    }

    void update(std::span<const std::uint8_t> data) override {
        for (std::uint8_t byte : data) {
            state_ = static_cast<UInt>(
                state_ * multiplier_ + static_cast<UInt>(byte)
            );
        }
    }

    std::string finalHex() override {
        std::ostringstream out;

        out << std::uppercase
            << std::hex
            << std::setw(MultiplicativeTraits<UInt>::hexWidth)
            << std::setfill('0')
            << static_cast<std::uint64_t>(state_);

        return out.str();
    }

    void reset() {
        state_ = seed_;
    }

private:
    UInt seed_;
    UInt multiplier_;
    UInt state_;
    std::string algorithm_;
};

using Multiplicative8Hasher  = MultiplicativeHasher<std::uint8_t>;
using Multiplicative16Hasher = MultiplicativeHasher<std::uint16_t>;
using Multiplicative32Hasher = MultiplicativeHasher<std::uint32_t>;
using Multiplicative64Hasher = MultiplicativeHasher<std::uint64_t>;

} // namespace hashing