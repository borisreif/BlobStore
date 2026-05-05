#pragma once

#include "ISimilarityHasher.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace similarity_hashing {

/**
 * @brief TLSH similarity hasher wrapper.
 *
 * TLSH is a locality-sensitive / fuzzy hash. It is useful for detecting
 * similarity between blobs, but it is not suitable as a canonical blob ID.
 *
 * This wrapper uses the C++ TLSH API:
 *
 *     Tlsh::update(...)
 *     Tlsh::final(...)
 *     Tlsh::getHash(...)
 *     Tlsh::totalDiff(...)
 */
class TlshHasher final : public ISimilarityHasher {
public:
    /**
     * @brief Constructs a fresh TLSH hasher.
     */
    TlshHasher();

    /**
     * @brief Destroys the TLSH hasher.
     */
    ~TlshHasher() override;

    TlshHasher(const TlshHasher&) = delete;
    TlshHasher& operator=(const TlshHasher&) = delete;

    TlshHasher(TlshHasher&&) noexcept;
    TlshHasher& operator=(TlshHasher&&) noexcept;

    /**
     * @brief Returns the algorithm name, "tlsh".
     */
    const std::string& algorithm() const override;

    /**
     * @brief Feeds another byte chunk into the TLSH state.
     *
     * @param data Read-only chunk of blob bytes.
     */
    void update(std::span<const std::uint8_t> data) override;

    /**
     * @brief Finalizes and returns the TLSH digest string.
     *
     * If TLSH cannot produce a digest, for example because the input is too
     * short or too simple, this returns an empty string.
     *
     * @return TLSH digest string, or empty string if invalid.
     */
    std::string finalSignature() override;

    /**
     * @brief Resets the TLSH state.
     */
    void reset() override;

    /**
     * @brief Computes the TLSH distance between two TLSH digest strings.
     *
     * A lower score means more similar. A score of 0 means almost identical.
     * This is the opposite intuition from ssdeep, where higher is usually
     * more similar.
     *
     * @param lhs First TLSH digest string.
     * @param rhs Second TLSH digest string.
     * @param includeLength Whether TLSH should include length difference
     *        in the distance calculation.
     * @return TLSH distance score.
     */
    static int distance(
        const std::string& lhs,
        const std::string& rhs,
        bool includeLength = true
    );

private:
    struct State;

    std::unique_ptr<State> state_;
    std::string algorithm_;
};

/**
 * @brief Creates a factory for TLSH similarity hashers.
 */
SimilarityHasherFactory makeTlshFactory();

} // namespace similarity_hashing