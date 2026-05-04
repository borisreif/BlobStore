#pragma once

#include "IHasher.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>

// https://www.zedwood.com/article/cpp-sha256-function
// https://stackoverflow.com/questions/50489951/openssl-convert-binary-bytes-to-sha256-c
//
// This wrapper uses OpenSSL's EVP interface rather than the older low-level
// SHA256_* functions. EVP keeps the implementation flexible and avoids APIs
// that are deprecated in modern OpenSSL versions.

namespace hashing {

/**
 * @brief Streaming SHA-256 hasher implemented with OpenSSL EVP.
 *
 * The class hides the OpenSSL `EVP_MD_CTX` behind a private State object so the
 * public header does not expose OpenSSL implementation details. The algorithm
 * label is always `sha256`.
 */
class Sha256OpenSSLHasher final : public IHasher {
public:
    /**
     * @brief Allocate and initialize a new SHA-256 EVP context.
     * @throws std::runtime_error if OpenSSL context allocation or initialization fails.
     */
    Sha256OpenSSLHasher();

    /**
     * @brief Free the hidden EVP context.
     */
    ~Sha256OpenSSLHasher() override;

    /** @brief Copying is disabled because the OpenSSL context is owned uniquely. */
    Sha256OpenSSLHasher(const Sha256OpenSSLHasher&) = delete;

    /** @brief Copy assignment is disabled because the OpenSSL context is owned uniquely. */
    Sha256OpenSSLHasher& operator=(const Sha256OpenSSLHasher&) = delete;

    /** @brief Move construction transfers ownership of the hidden context. */
    Sha256OpenSSLHasher(Sha256OpenSSLHasher&&) noexcept;

    /** @brief Move assignment transfers ownership of the hidden context. */
    Sha256OpenSSLHasher& operator=(Sha256OpenSSLHasher&&) noexcept;

    /**
     * @brief Return the stable algorithm label `sha256`.
     */
    const std::string& algorithm() const override;

    /**
     * @brief Feed another byte chunk into the SHA-256 context.
     * @param data Read-only bytes to hash.
     * @throws std::runtime_error if OpenSSL rejects the update operation.
     */
    void update(std::span<const std::uint8_t> data) override;

    /**
     * @brief Return the current SHA-256 digest as hexadecimal text.
     *
     * The implementation copies the EVP context before finalization, so calling
     * finalHex() does not destroy the current state. This matches BLAKE3's
     * non-destructive finalize behavior and makes the interface easier to use.
     */
    std::string finalHex() override;

    /**
     * @brief Reset the OpenSSL context to an empty SHA-256 state.
     * @throws std::runtime_error if OpenSSL initialization fails.
     */
    void reset();

private:
    /** @brief Forward-declared wrapper around `EVP_MD_CTX*`. */
    struct State;

    /** @brief Owned OpenSSL EVP context. */
    std::unique_ptr<State> state_;

    /** @brief Cached algorithm label. */
    std::string algorithm_;

    /**
     * @brief Convert raw digest bytes to lowercase hexadecimal text.
     * @param bytes Pointer to raw digest bytes.
     * @param n Number of bytes to convert.
     * @return Hexadecimal string with two characters per byte.
     */
    static std::string toHex(const unsigned char* bytes, std::size_t n);
};

/**
 * @brief Create a factory that returns fresh SHA-256 OpenSSL hashers.
 * @return Factory compatible with BlobStore.
 */
HasherFactory makeSha256OpenSSLFactory();

} // namespace hashing
