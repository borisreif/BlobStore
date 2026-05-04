#include "Blake3Hasher.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
#include "../third_party/blake3/blake3.h"
}

namespace hashing {

struct Blake3Hasher::State {
    blake3_hasher ctx;
};

Blake3Hasher::Blake3Hasher(std::size_t digestLength)
    : state_(std::make_unique<State>()),
      digestLength_(digestLength),
      algorithm_("blake3-" + std::to_string(digestLength * 8))
{
    if (digestLength_ == 0) {
        throw std::invalid_argument("BLAKE3 digest length must be greater than zero");
    }

    blake3_hasher_init(&state_->ctx);
}

Blake3Hasher::~Blake3Hasher() = default;

Blake3Hasher::Blake3Hasher(Blake3Hasher&&) noexcept = default;

Blake3Hasher& Blake3Hasher::operator=(Blake3Hasher&&) noexcept = default;

const std::string& Blake3Hasher::algorithm() const {
    return algorithm_;
}

void Blake3Hasher::update(std::span<const std::uint8_t> data) {
    if (data.empty()) {
        return;
    }

    blake3_hasher_update(
        &state_->ctx,
        data.data(),
        data.size()
    );
}

std::string Blake3Hasher::finalHex() {
    std::vector<std::uint8_t> output(digestLength_);

    blake3_hasher_finalize(
        &state_->ctx,
        output.data(),
        output.size()
    );

    return toHex(output.data(), output.size());
}

void Blake3Hasher::reset() {
    blake3_hasher_init(&state_->ctx);
}

std::string Blake3Hasher::toHex(const std::uint8_t* bytes, std::size_t n) {
    std::ostringstream out;

    out << std::hex
        << std::setfill('0');

    for (std::size_t i = 0; i < n; ++i) {
        out << std::setw(2)
            << static_cast<unsigned int>(bytes[i]);
    }

    return out.str();
}

HasherFactory makeBlake3Factory(std::size_t digestLength) {
    return [digestLength] {
        return std::make_unique<Blake3Hasher>(digestLength);
    };
}

} // namespace hashing