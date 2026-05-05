#pragma once

#include "IIdentityHasher.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace identity_hashing {

/**
 * @brief Streaming BLAKE3 hasher adapted to the IHasher strategy interface.
 *
 * The implementation wraps the official BLAKE3 C API while keeping BLAKE3's C
 * data structures out of this public header. The default digest length is 32
 * bytes, which corresponds to a 256-bit identifier.
 */
class Blake3Hasher final : public IIdentityHasher {
public:
    /**
     * @brief Construct a new BLAKE3 hasher.
     * @param digestLength Number of output bytes to request from BLAKE3.
     *
     * @throws std::invalid_argument if digestLength is zero.
     */
    explicit Blake3Hasher(std::size_t digestLength = 32);

    /**
     * @brief Destroy the hidden BLAKE3 state.
     */
    ~Blake3Hasher() override;

    /** @brief Copying is disabled because the hidden C state is owned uniquely. */
    Blake3Hasher(const Blake3Hasher&) = delete;

    /** @brief Copy assignment is disabled because the hidden C state is owned uniquely. */
    Blake3Hasher& operator=(const Blake3Hasher&) = delete;

    /** @brief Move construction transfers ownership of the hidden state. */
    Blake3Hasher(Blake3Hasher&&) noexcept;

    /** @brief Move assignment transfers ownership of the hidden state. */
    Blake3Hasher& operator=(Blake3Hasher&&) noexcept;

    /**
     * @brief Return the algorithm label, for example `blake3-256`.
     */
    const std::string& algorithm() const override;

    /**
     * @brief Feed a chunk of bytes into the BLAKE3 context.
     * @param data Read-only bytes to hash.
     */
    void update(std::span<const std::uint8_t> data) override;

    /**
     * @brief Finalize the current BLAKE3 state and return hexadecimal output.
     *
     * BLAKE3 finalization is non-destructive, so repeated calls return the same
     * digest until more data is added or reset() is called.
     */
    std::string finalHex() override;

    /**
     * @brief Reset the hasher to an empty BLAKE3 state.
     */
    void reset();

private:
    /** @brief Forward-declared wrapper around the C `blake3_hasher` state. */
    struct State;

    /** @brief Owned BLAKE3 state hidden from the header. */
    std::unique_ptr<State> state_;

    /** @brief Number of output bytes requested from BLAKE3. */
    std::size_t digestLength_;

    /** @brief Cached algorithm label. */
    std::string algorithm_;

    /**
     * @brief Convert raw digest bytes to lowercase hexadecimal text.
     * @param bytes Pointer to digest bytes.
     * @param n Number of bytes to convert.
     * @return Hexadecimal string with two characters per byte.
     */
    static std::string toHex(const std::uint8_t* bytes, std::size_t n);
};

/**
 * @brief Create a factory that returns fresh Blake3Hasher instances.
 * @param digestLength Number of output bytes for each created hasher.
 * @return Factory compatible with BlobStore.
 */
IdentityHasherFactory makeBlake3Factory(std::size_t digestLength = 32);

} // namespace identity_hashing
