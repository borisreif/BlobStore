#pragma once

#include "IFuzzyHasher.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace hashing {

class SsdeepHasher final : public IFuzzyHasher {
public:
    SsdeepHasher();

    ~SsdeepHasher() override;

    SsdeepHasher(const SsdeepHasher&) = delete;
    SsdeepHasher& operator=(const SsdeepHasher&) = delete;

    SsdeepHasher(SsdeepHasher&&) noexcept;
    SsdeepHasher& operator=(SsdeepHasher&&) noexcept;

    const std::string& algorithm() const override;

    void update(std::span<const std::uint8_t> data) override;

    std::string finalSignature() override;

    void reset() override;

    static int compare(
        const std::string& signatureA,
        const std::string& signatureB
    );

private:
    struct State;

    std::unique_ptr<State> state_;
    std::string algorithm_;
};

FuzzyHasherFactory makeSsdeepFactory();

} // namespace hashing