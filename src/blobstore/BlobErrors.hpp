#pragma once

#include <stdexcept>
#include <string>

namespace blobstore {

/**
 * @brief Base exception type for BlobStore-specific runtime errors.
 *
 * Catch this type when you want to handle any error raised by the blob-store
 * layer without also catching unrelated standard-library exceptions.
 */
class BlobStoreError : public std::runtime_error {
public:
    /**
     * @brief Construct an error with a human-readable diagnostic message.
     * @param message Explanation of the failing blob-store operation.
     */
    explicit BlobStoreError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Exception raised when a canonical hash path already exists for different bytes.
 *
 * With a strong canonical hash this should be extraordinarily unlikely. In a
 * testing setup using toy hashers such as FNV or multiplicative hashes, this is
 * useful because collisions are much easier to trigger.
 */
class HashCollisionError : public BlobStoreError {
public:
    /**
     * @brief Construct a collision/integrity error.
     * @param message Explanation of the detected collision or store inconsistency.
     */
    explicit HashCollisionError(const std::string& message)
        : BlobStoreError(message) {}
};

} // namespace blobstore
