#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <span>


/**
 * The Hasher Interface: IHasher
 * 
 * @link https://en.wikipedia.org/wiki/Hash_function
 * @link https://en.wikipedia.org/wiki/Cryptographic_hash_function
 * @link https://www.geeksforgeeks.org/dsa/hash-functions-and-list-types-of-hash-functions/
 * @link https://asecuritysite.com/hash/index
 *
 * The Strategy interface declares operations common to all supported versions
 * of some algorithm.
 * @link https://refactoring.guru/design-patterns/strategy
 * @link https://refactoring.guru/design-patterns/strategy/cpp/example
 * The Context uses this interface to call the algorithm defined by Concrete
 * Strategies.
 *
 * @author Boris A. Reif
 * @version 0.0.1
 */


namespace hashing {

class IHasher {
public:
    virtual ~IHasher() = default;

    virtual const std::string& algorithm() const = 0;
    // virtual void update(const std::uint8_t* data, std::size_t size) = 0;
    virtual void update(std::span<const std::uint8_t> data) = 0;
    virtual std::string finalHex() = 0;
};

using HasherFactory = std::function<std::unique_ptr<IHasher>()>;

} // namespace hashing