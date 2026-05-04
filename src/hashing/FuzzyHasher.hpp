#pragma once

#include "IHasher.hpp"

#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <fuzzy.h>
#include <algorithm>

/**
 * The Fuzzy hash function
 * 
 * @link https://en.wikipedia.org/wiki/Fuzzy_hashing
 * @link https://docs.rspamd.com/tutorials/fuzzy_storage/
 * @link https://www.microsoft.com/en-us/security/blog/2021/07/27/combing-through-the-fuzz-using-fuzzy-hashing-and-deep-learning-to-counter-malware-detection-evasion-techniques/
 *
 * @author Boris A. Reif
 * @version 0.0.1
 */

namespace hashing {


class FuzzyHasher final : public IHasher {
public:
    static_assert(
        std::is_unsigned_v<UInt>,
        "MultiplicativeHasher requires an unsigned integer type"
    );



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


private:
    std::string algorithm_;
};


} // namespace hashing