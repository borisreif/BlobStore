#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>

namespace hashing {

class IFuzzyHasher {
public:
    virtual ~IFuzzyHasher() = default;

    virtual const std::string& algorithm() const = 0;

    // Feed another chunk of blob bytes into the fuzzy hasher.
    virtual void update(std::span<const std::uint8_t> data) = 0;

    // Return the final fuzzy signature.
    // This is not necessarily hexadecimal.
    virtual std::string finalSignature() = 0;

    virtual void reset() = 0;
};

using FuzzyHasherFactory = std::function<std::unique_ptr<IFuzzyHasher>()>;

} // namespace hashing