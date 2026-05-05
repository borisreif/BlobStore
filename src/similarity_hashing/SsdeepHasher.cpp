#include "SsdeepHasher.hpp"

#include <stdexcept>

extern "C" {
#include <fuzzy.h>
}

namespace similarity_hashing {

struct SsdeepHasher::State {
    fuzzy_state* state = nullptr;

    State()
        : state(fuzzy_new())
    {
        if (!state) {
            throw std::runtime_error("fuzzy_new failed");
        }
    }

    ~State() {
        fuzzy_free(state);
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;
};

SsdeepHasher::SsdeepHasher()
    : state_(std::make_unique<State>()),
      algorithm_("ssdeep")
{
}

SsdeepHasher::~SsdeepHasher() = default;

SsdeepHasher::SsdeepHasher(SsdeepHasher&&) noexcept = default;

SsdeepHasher& SsdeepHasher::operator=(SsdeepHasher&&) noexcept = default;

const std::string& SsdeepHasher::algorithm() const {
    return algorithm_;
}

void SsdeepHasher::update(std::span<const std::uint8_t> data) {
    if (data.empty()) {
        return;
    }

    int rc = fuzzy_update(
        state_->state,
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size()
    );

    if (rc != 0) {
        throw std::runtime_error("fuzzy_update failed");
    }
}

std::string SsdeepHasher::finalSignature() {
    char result[FUZZY_MAX_RESULT];

    int rc = fuzzy_digest(
        state_->state,
        result,
        0
    );

    if (rc != 0) {
        throw std::runtime_error("fuzzy_digest failed");
    }

    return std::string(result);
}

void SsdeepHasher::reset() {
    state_ = std::make_unique<State>();
}

int SsdeepHasher::compare(
    const std::string& signatureA,
    const std::string& signatureB
) {
    return fuzzy_compare(
        signatureA.c_str(),
        signatureB.c_str()
    );
}

SimilarityHasherFactory makeSsdeepFactory() {
    return [] {
        return std::make_unique<SsdeepHasher>();
    };
}

} // namespace similarity_hashing
