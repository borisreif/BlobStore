#include "Sha256OpenSSLHasher.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <openssl/evp.h>

namespace hashing {

struct Sha256OpenSSLHasher::State {
    EVP_MD_CTX* ctx = nullptr;

    State()
        : ctx(EVP_MD_CTX_new())
    {
        if (!ctx) {
            throw std::runtime_error("EVP_MD_CTX_new failed");
        }
    }

    ~State() {
        EVP_MD_CTX_free(ctx);
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;
};

Sha256OpenSSLHasher::Sha256OpenSSLHasher()
    : state_(std::make_unique<State>()),
      algorithm_("sha256")
{
    reset();
}

Sha256OpenSSLHasher::~Sha256OpenSSLHasher() = default;

Sha256OpenSSLHasher::Sha256OpenSSLHasher(
    Sha256OpenSSLHasher&&
) noexcept = default;

Sha256OpenSSLHasher& Sha256OpenSSLHasher::operator=(
    Sha256OpenSSLHasher&&
) noexcept = default;

const std::string& Sha256OpenSSLHasher::algorithm() const {
    return algorithm_;
}

void Sha256OpenSSLHasher::reset() {
    if (EVP_DigestInit_ex(state_->ctx, EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }
}

void Sha256OpenSSLHasher::update(std::span<const std::uint8_t> data) {
    if (data.empty()) {
        return;
    }

    if (EVP_DigestUpdate(
            state_->ctx,
            data.data(),
            data.size()
        ) != 1
    ) {
        throw std::runtime_error("EVP_DigestUpdate failed");
    }
}

/*
std::string Sha256OpenSSLHasher::finalHex() {
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int outLen = 0;

    /*
        EVP_DigestFinal_ex finalizes the current context.
        Because our BlobStore creates fresh hashers per operation and calls
        finalHex() once, this is fine.

        If you want finalHex() to be repeatable/idempotent, copy the context
        first with EVP_MD_CTX_copy_ex and finalize the copy instead.
    *
    if (EVP_DigestFinal_ex(state_->ctx, out, &outLen) != 1) {
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    return toHex(out, outLen);
}
*/
std::string Sha256OpenSSLHasher::finalHex() {
    EVP_MD_CTX* copy = EVP_MD_CTX_new();

    if (!copy) {
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_MD_CTX_copy_ex(copy, state_->ctx) != 1) {
        EVP_MD_CTX_free(copy);
        throw std::runtime_error("EVP_MD_CTX_copy_ex failed");
    }

    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int outLen = 0;

    if (EVP_DigestFinal_ex(copy, out, &outLen) != 1) {
        EVP_MD_CTX_free(copy);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(copy);

    return toHex(out, outLen);
}

std::string Sha256OpenSSLHasher::toHex(
    const unsigned char* bytes,
    std::size_t n
) {
    std::ostringstream out;

    out << std::hex
        << std::setfill('0');

    for (std::size_t i = 0; i < n; ++i) {
        out << std::setw(2)
            << static_cast<unsigned int>(bytes[i]);
    }

    return out.str();
}

HasherFactory makeSha256OpenSSLFactory() {
    return [] {
        return std::make_unique<Sha256OpenSSLHasher>();
    };
}

} // namespace hashing