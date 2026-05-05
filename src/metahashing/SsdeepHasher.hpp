#pragma once

#include "IFuzzyHasher.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace hashing {

/**
 * @brief ssdeep fuzzy hasher adapter.
 *
 * This class is optional and requires libfuzzy/ssdeep development headers. It
 * implements IFuzzyHasher so BlobStore can store an ssdeep signature in
 * META.TXT without using that signature as the canonical blob id.
 */
class SsdeepHasher final : public IFuzzyHasher {
public:
    /** @brief Allocate a new ssdeep fuzzy state. */
    SsdeepHasher();

    /** @brief Free the ssdeep fuzzy state. */
    ~SsdeepHasher() override;

    /** @brief Copying is disabled because the fuzzy state is owned uniquely. */
    SsdeepHasher(const SsdeepHasher&) = delete;

    /** @brief Copy assignment is disabled because the fuzzy state is owned uniquely. */
    SsdeepHasher& operator=(const SsdeepHasher&) = delete;

    /** @brief Move construction transfers ownership of the fuzzy state. */
    SsdeepHasher(SsdeepHasher&&) noexcept;

    /** @brief Move assignment transfers ownership of the fuzzy state. */
    SsdeepHasher& operator=(SsdeepHasher&&) noexcept;

    /** @brief Return the stable algorithm label `ssdeep`. */
    const std::string& algorithm() const override;

    /** @brief Feed bytes into the ssdeep fuzzy state. */
    void update(std::span<const std::uint8_t> data) override;

    /** @brief Return the final ssdeep signature string. */
    std::string finalSignature() override;

    /** @brief Reset to a new empty ssdeep state. */
    void reset() override;

    /**
     * @brief Compare two ssdeep signatures.
     * @param signatureA First ssdeep signature.
     * @param signatureB Second ssdeep signature.
     * @return ssdeep similarity score, usually 0 to 100, or a negative error code.
     */
    static int compare(
        const std::string& signatureA,
        const std::string& signatureB
    );

private:
    /** @brief Forward-declared wrapper around ssdeep's C state. */
    struct State;

    /** @brief Owned ssdeep state. */
    std::unique_ptr<State> state_;

    /** @brief Cached algorithm label. */
    std::string algorithm_;
};

/** @brief Create a factory that returns fresh ssdeep fuzzy hashers. */
FuzzyHasherFactory makeSsdeepFactory();

} // namespace hashing
