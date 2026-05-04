#pragma once

#ifndef HASHING_FNV_HASHER_HPP
#include "FnvHasher.hpp"
#else

#include <iomanip>
#include <sstream>

namespace hashing {

template <typename UInt, FnvVariant Variant>
FnvHasher<UInt, Variant>::FnvHasher()
    : state_(FnvTraits<UInt>::offsetBasis),
      algorithm_(makeAlgorithmName())
{
}

template <typename UInt, FnvVariant Variant>
const std::string& FnvHasher<UInt, Variant>::algorithm() const {
    return algorithm_;
}

template <typename UInt, FnvVariant Variant>
void FnvHasher<UInt, Variant>::update(std::span<const std::uint8_t> data) {
    for (std::uint8_t byteValue : data) {
        UInt byte = static_cast<UInt>(byteValue);

        if constexpr (Variant == FnvVariant::Fnv1) {
            state_ *= FnvTraits<UInt>::prime;
            state_ ^= byte;
        } else {
            state_ ^= byte;
            state_ *= FnvTraits<UInt>::prime;
        }
    }
}

template <typename UInt, FnvVariant Variant>
std::string FnvHasher<UInt, Variant>::finalHex() {
    std::ostringstream out;

    out << std::uppercase
        << std::hex
        << std::setw(FnvTraits<UInt>::hexWidth)
        << std::setfill('0')
        << state_;

    return out.str();
}

template <typename UInt, FnvVariant Variant>
void FnvHasher<UInt, Variant>::reset() {
    state_ = FnvTraits<UInt>::offsetBasis;
}

template <typename UInt, FnvVariant Variant>
std::string FnvHasher<UInt, Variant>::makeAlgorithmName() {
    std::string name;

    if constexpr (Variant == FnvVariant::Fnv1) {
        name = "fnv1-";
    } else {
        name = "fnv1a-";
    }

    name += std::to_string(FnvTraits<UInt>::bits);

    return name;
}

} // namespace hashing

#endif // HASHING_FNV_HASHER_HPP
