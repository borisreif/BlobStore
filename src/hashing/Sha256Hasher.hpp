#include "openssl/sha.h"

#pragma once

#include "IHasher.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <sstream>
#include <openssl/evp.h>

// https://www.zedwood.com/article/cpp-sha256-function
// https://stackoverflow.com/questions/50489951/openssl-convert-binary-bytes-to-sha256-c

namespace hashing {

class Sha256Hasher final : public IHasher {

public:
    Sha256Hasher();

    const std::string& algorithm() const override;
    void update(std::span<const std::uint8_t> data) override {
        SHA256_CTX sha256;
        SHA256_Update(&sha256, str.c_str(), str.size());
        SHA256_Final(SHA256_DIGEST_LENGTH, &sha256);
        for (std::uint8_t byte : data) {
            state_ = static_cast<UInt>(
                state_ * multiplier_ + static_cast<UInt>(byte)
            );
        }
    }
    std::string finalHex() override;

private:
    EVP_MD_CTX* ctx_;
    static std::string toHex(const unsigned char* bytes, std::size_t n);
};

} // namespace hashing
