#pragma once

#include "IHasher.hpp"
#include "FibonacciHasher.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <span>
#include <string>
#include <type_traits>


// experimental
// generatd by chatGPT

namespace hashing {

template <typename UInt>
struct FibonacciByteHasherTraits;

template <>
struct FibonacciByteHasherTraits<std::uint32_t> {
    static constexpr std::uint32_t seed = 0x811C9DC5u;
    static constexpr int hexWidth = 8;
    static constexpr const char* algorithm = "fib-byte-32";
};

template <>
struct FibonacciByteHasherTraits<std::uint64_t> {
    static constexpr std::uint64_t seed = 0xCBF29CE484222325ull;
    static constexpr int hexWidth = 16;
    static constexpr const char* algorithm = "fib-byte-64";
};

template <typename UInt>
class FibonacciByteHasher final : public IHasher {
public:
    static_assert(
        std::is_same_v<UInt, std::uint32_t> ||
        std::is_same_v<UInt, std::uint64_t>,
        "FibonacciByteHasher supports only uint32_t and uint64_t"
    );

    FibonacciByteHasher()
        : state_(FibonacciByteHasherTraits<UInt>::seed),
          algorithm_(FibonacciByteHasherTraits<UInt>::algorithm)
    {
    }

    const std::string& algorithm() const override {
        return algorithm_;
    }

    void update(std::span<const std::uint8_t> data) override {
        for (std::uint8_t byte : data) {
            state_ ^= static_cast<UInt>(byte);
            state_ *= fib::FibonacciTraits<UInt>::constant;
        }
    }

    std::string finalHex() override {
        std::ostringstream out;

        out << std::uppercase
            << std::hex
            << std::setw(FibonacciByteHasherTraits<UInt>::hexWidth)
            << std::setfill('0')
            << state_;

        return out.str();
    }

    void reset() {
        state_ = FibonacciByteHasherTraits<UInt>::seed;
    }

private:
    UInt state_;
    std::string algorithm_;
};

using FibonacciByte32Hasher = FibonacciByteHasher<std::uint32_t>;
using FibonacciByte64Hasher = FibonacciByteHasher<std::uint64_t>;

} // namespace hashing