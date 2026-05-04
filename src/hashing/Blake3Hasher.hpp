#pragma once

#include "IHasher.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace hashing {

class Blake3Hasher final : public IHasher {
public:
    explicit Blake3Hasher(std::size_t digestLength = 32);

    ~Blake3Hasher() override;

    Blake3Hasher(const Blake3Hasher&) = delete;
    Blake3Hasher& operator=(const Blake3Hasher&) = delete;

    Blake3Hasher(Blake3Hasher&&) noexcept;
    Blake3Hasher& operator=(Blake3Hasher&&) noexcept;

    const std::string& algorithm() const override;

    void update(std::span<const std::uint8_t> data) override;

    std::string finalHex() override;

    void reset();

private:
    struct State;

    std::unique_ptr<State> state_;

    std::size_t digestLength_;
    std::string algorithm_;

    static std::string toHex(const std::uint8_t* bytes, std::size_t n);
};

HasherFactory makeBlake3Factory(std::size_t digestLength = 32);

} // namespace hashing