#include "TlshHasher.hpp"

#include <stdexcept>

#include <tlsh.h>

namespace similarity_hashing {

struct TlshHasher::State {
    Tlsh tlsh;
    bool finalized = false;
};

TlshHasher::TlshHasher()
    : state_(std::make_unique<State>()),
      algorithm_("tlsh")
{
}

TlshHasher::~TlshHasher() = default;

TlshHasher::TlshHasher(TlshHasher&&) noexcept = default;

TlshHasher& TlshHasher::operator=(TlshHasher&&) noexcept = default;

const std::string& TlshHasher::algorithm() const {
    return algorithm_;
}

void TlshHasher::update(std::span<const std::uint8_t> data) {
    if (data.empty()) {
        return;
    }

    if (state_->finalized) {
        throw std::logic_error("Cannot update TLSH after finalSignature()");
    }

    /*
        TLSH's C++ API expects:
            const unsigned char*
            unsigned int length

        Our blob store uses std::span<const std::uint8_t>.
        std::uint8_t is normally an unsigned byte type, so this cast is
        appropriate for passing raw binary data to TLSH.
    */
    state_->tlsh.update(
        reinterpret_cast<const unsigned char*>(data.data()),
        static_cast<unsigned int>(data.size())
    );
}

std::string TlshHasher::finalSignature() {
    if (!state_->finalized) {
        /*
            final() tells TLSH there is no more data.

            TLSH may still be invalid after finalization if the input is
            too short or too simple.
        */
        state_->tlsh.final();
        state_->finalized = true;
    }

    const char* hash = state_->tlsh.getHash();

    if (hash == nullptr) {
        return {};
    }

    std::string signature(hash);

    /*
        Some TLSH versions return an empty/null-like signature when the input
        is too short or too simple. Avoid depending on Tlsh::isValid(), because
        older Ubuntu/libtlsh headers do not expose that member.
    */
    if (signature.empty() || signature == "TNULL") {
        return {};
    }

    return signature;
}

void TlshHasher::reset() {
    state_->tlsh.reset();
    state_->finalized = false;
}

int TlshHasher::distance(
    const std::string& lhs,
    const std::string& rhs,
    bool includeLength
) {
    Tlsh left;
    Tlsh right;

    if (left.fromTlshStr(lhs.c_str()) != 0) {
        throw std::invalid_argument("Invalid left TLSH string");
    }

    if (right.fromTlshStr(rhs.c_str()) != 0) {
        throw std::invalid_argument("Invalid right TLSH string");
    }

    /*
        fromTlshStr() returning 0 is the compatibility check we rely on here.
        Do not call Tlsh::isValid(): some packaged TLSH headers do not provide
        that member even though totalDiff() and fromTlshStr() are available.
    */
    return left.totalDiff(&right, includeLength);
}

SimilarityHasherFactory makeTlshFactory() {
    return [] {
        return std::make_unique<TlshHasher>();
    };
}

} // namespace similarity_hashing