#pragma once

#include "IHasher.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>

// https://www.zedwood.com/article/cpp-sha256-function
// https://stackoverflow.com/questions/50489951/openssl-convert-binary-bytes-to-sha256-c

namespace hashing {

class Sha256OpenSSLHasher final : public IHasher {
public:
    Sha256OpenSSLHasher();

    ~Sha256OpenSSLHasher() override;

    Sha256OpenSSLHasher(const Sha256OpenSSLHasher&) = delete;
    Sha256OpenSSLHasher& operator=(const Sha256OpenSSLHasher&) = delete;

    Sha256OpenSSLHasher(Sha256OpenSSLHasher&&) noexcept;
    Sha256OpenSSLHasher& operator=(Sha256OpenSSLHasher&&) noexcept;

    const std::string& algorithm() const override;

    void update(std::span<const std::uint8_t> data) override;

    std::string finalHex() override;

    void reset();

private:
    struct State;

    std::unique_ptr<State> state_;

    std::string algorithm_;

    static std::string toHex(const unsigned char* bytes, std::size_t n);
};

HasherFactory makeSha256OpenSSLFactory();

} // namespace hashing