#pragma once

#include "IHasher.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace blobdb {

class Sha256Hasher : public IHasher {
public:
    Sha256Hasher();

    void reset() override;
    void update(const void* data, std::size_t len) override;
    std::string finishHex() override;

private:
    std::array<std::uint32_t, 8> state_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::uint64_t bitLen_ = 0;
    std::size_t bufferLen_ = 0;

    void transform(const std::uint8_t block[64]);
    static std::string toHex(const std::uint8_t* bytes, std::size_t n);
};

} // namespace blobdb
