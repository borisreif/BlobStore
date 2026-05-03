#include "../include/Sha256Hasher.h"

#include <algorithm>
#include <cstring>

namespace blobdb {
namespace {

constexpr std::uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

constexpr std::uint32_t rotr(std::uint32_t x, std::uint32_t n) {
    return (x >> n) | (x << (32u - n));
}
constexpr std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (~x & z);
}
constexpr std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}
constexpr std::uint32_t bigSigma0(std::uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}
constexpr std::uint32_t bigSigma1(std::uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}
constexpr std::uint32_t smallSigma0(std::uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}
constexpr std::uint32_t smallSigma1(std::uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}
std::uint32_t readBigEndian32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}
void writeBigEndian64(std::uint8_t* p, std::uint64_t value) {
    for (int i = 7; i >= 0; --i) {
        p[i] = static_cast<std::uint8_t>(value & 0xffu);
        value >>= 8u;
    }
}

} // namespace

Sha256Hasher::Sha256Hasher() { reset(); }

void Sha256Hasher::reset() {
    state_ = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
              0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    buffer_.fill(0);
    bitLen_ = 0;
    bufferLen_ = 0;
}

void Sha256Hasher::update(const void* data, std::size_t len) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    bitLen_ += static_cast<std::uint64_t>(len) * 8u;

    while (len > 0) {
        const std::size_t toCopy = std::min(len, buffer_.size() - bufferLen_);
        std::memcpy(buffer_.data() + bufferLen_, bytes, toCopy);
        bufferLen_ += toCopy;
        bytes += toCopy;
        len -= toCopy;

        if (bufferLen_ == buffer_.size()) {
            transform(buffer_.data());
            bufferLen_ = 0;
        }
    }
}

std::string Sha256Hasher::finishHex() {
    const std::uint64_t originalBitLen = bitLen_;
    buffer_[bufferLen_++] = 0x80u;

    if (bufferLen_ > 56) {
        while (bufferLen_ < 64) buffer_[bufferLen_++] = 0;
        transform(buffer_.data());
        bufferLen_ = 0;
    }

    while (bufferLen_ < 56) buffer_[bufferLen_++] = 0;
    writeBigEndian64(buffer_.data() + 56, originalBitLen);
    transform(buffer_.data());

    std::uint8_t digest[32];
    for (std::size_t i = 0; i < state_.size(); ++i) {
        digest[i * 4 + 0] = static_cast<std::uint8_t>((state_[i] >> 24) & 0xffu);
        digest[i * 4 + 1] = static_cast<std::uint8_t>((state_[i] >> 16) & 0xffu);
        digest[i * 4 + 2] = static_cast<std::uint8_t>((state_[i] >> 8) & 0xffu);
        digest[i * 4 + 3] = static_cast<std::uint8_t>(state_[i] & 0xffu);
    }

    const std::string hex = toHex(digest, sizeof(digest));
    reset();
    return hex;
}

void Sha256Hasher::transform(const std::uint8_t block[64]) {
    std::uint32_t w[64];
    for (std::size_t i = 0; i < 16; ++i) w[i] = readBigEndian32(block + i * 4);
    for (std::size_t i = 16; i < 64; ++i) {
        w[i] = smallSigma1(w[i - 2]) + w[i - 7] + smallSigma0(w[i - 15]) + w[i - 16];
    }

    std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

    for (std::size_t i = 0; i < 64; ++i) {
        const std::uint32_t t1 = h + bigSigma1(e) + ch(e, f, g) + K[i] + w[i];
        const std::uint32_t t2 = bigSigma0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
}

std::string Sha256Hasher::toHex(const std::uint8_t* bytes, std::size_t n) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string s(n * 2, '\0');
    for (std::size_t i = 0; i < n; ++i) {
        s[2 * i] = hex[(bytes[i] >> 4) & 0x0f];
        s[2 * i + 1] = hex[bytes[i] & 0x0f];
    }
    return s;
}

} // namespace blobdb
