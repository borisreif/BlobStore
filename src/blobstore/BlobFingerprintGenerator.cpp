#include "BlobFingerprintGenerator.hpp"
#include "BlobErrors.hpp"

#include <fstream>
#include <span>
#include <utility>

namespace blobstore {

namespace {

/**
 * @brief Normalize hexadecimal text to uppercase for stable metadata.
 *
 * This helper intentionally mirrors BlobStore's path-normalization behavior,
 * but lives locally so BlobFingerprintGenerator does not depend on BlobStore's
 * private helper functions.
 */
std::string normalizeHex(std::string hex) {
    for (char& c : hex) {
        if (c >= 'a' && c <= 'f') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return hex;
}

} // anonymous namespace

BlobFingerprintGenerator::BlobFingerprintGenerator(
    std::vector<identity_hashing::IdentityHasherFactory> identityHashers,
    std::vector<similarity_hashing::SimilarityHasherFactory> similarityHashers,
    std::size_t ioChunkSize
)
    : identityHasherFactories_(std::move(identityHashers)),
      similarityHasherFactories_(std::move(similarityHashers)),
      ioChunkSize_(ioChunkSize)
{
    if (identityHasherFactories_.empty()) {
        throw BlobStoreError("BlobFingerprintGenerator needs at least one identity hasher");
    }

    if (ioChunkSize_ == 0) {
        throw BlobStoreError("BlobFingerprintGenerator I/O chunk size must be greater than zero");
    }

    auto primaryHasher = identityHasherFactories_.front()();
    if (!primaryHasher) {
        throw BlobStoreError("Primary identity hasher factory returned nullptr");
    }

    primaryAlgorithm_ = primaryHasher->algorithm();
}

const std::string& BlobFingerprintGenerator::primaryAlgorithm() const {
    return primaryAlgorithm_;
}

BlobFingerprint BlobFingerprintGenerator::generateForFile(
    const fs::path& sourcePath
) const {
    return generate(sourcePath, std::nullopt);
}

BlobFingerprint BlobFingerprintGenerator::generateForFileAndCopyTo(
    const fs::path& sourcePath,
    const fs::path& tempPayloadPath
) const {
    return generate(sourcePath, tempPayloadPath);
}

std::vector<std::unique_ptr<identity_hashing::IIdentityHasher>>
BlobFingerprintGenerator::makeIdentityHashers() const {
    std::vector<std::unique_ptr<identity_hashing::IIdentityHasher>> result;
    result.reserve(identityHasherFactories_.size());

    for (const auto& factory : identityHasherFactories_) {
        auto hasher = factory();
        if (!hasher) {
            throw BlobStoreError("Identity hasher factory returned nullptr");
        }
        result.push_back(std::move(hasher));
    }

    return result;
}

std::vector<std::unique_ptr<similarity_hashing::ISimilarityHasher>>
BlobFingerprintGenerator::makeSimilarityHashers() const {
    std::vector<std::unique_ptr<similarity_hashing::ISimilarityHasher>> result;
    result.reserve(similarityHasherFactories_.size());

    for (const auto& factory : similarityHasherFactories_) {
        auto hasher = factory();
        if (!hasher) {
            throw BlobStoreError("Similarity hasher factory returned nullptr");
        }
        result.push_back(std::move(hasher));
    }

    return result;
}

BlobFingerprint BlobFingerprintGenerator::generate(
    const fs::path& sourcePath,
    const std::optional<fs::path>& tempPayloadPath
) const {
    if (!fs::exists(sourcePath)) {
        throw BlobStoreError("Source file does not exist: " + sourcePath.string());
    }

    if (!fs::is_regular_file(sourcePath)) {
        throw BlobStoreError("Source path is not a regular file: " + sourcePath.string());
    }

    auto identityHashers = makeIdentityHashers();
    auto similarityHashers = makeSimilarityHashers();

    std::ifstream in(sourcePath, std::ios::binary);
    if (!in) {
        throw BlobStoreError("Could not open source file: " + sourcePath.string());
    }

    std::ofstream out;
    if (tempPayloadPath.has_value()) {
        out.open(*tempPayloadPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw BlobStoreError("Could not create temporary file: " + tempPayloadPath->string());
        }
    }

    std::vector<std::uint8_t> buffer(ioChunkSize_);
    std::uintmax_t totalSize = 0;

    while (in) {
        in.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );

        const std::streamsize got = in.gcount();

        if (got > 0) {
            const auto n = static_cast<std::size_t>(got);
            const std::span<const std::uint8_t> chunk(buffer.data(), n);

            for (auto& hasher : identityHashers) {
                hasher->update(chunk);
            }

            for (auto& similarityHasher : similarityHashers) {
                similarityHasher->update(chunk);
            }

            if (out.is_open()) {
                out.write(
                    reinterpret_cast<const char*>(buffer.data()),
                    got
                );
            }

            totalSize += static_cast<std::uintmax_t>(got);
        }
    }

    if (out.is_open()) {
        out.close();
        if (!out) {
            throw BlobStoreError("Failed while writing temporary file: " + tempPayloadPath->string());
        }
    }

    BlobFingerprint fingerprint;
    fingerprint.size = totalSize;
    fingerprint.identityDigests.reserve(identityHashers.size());
    fingerprint.similarityDigests.reserve(similarityHashers.size());

    for (auto& hasher : identityHashers) {
        fingerprint.identityDigests.push_back(HashDigest{
            hasher->algorithm(),
            normalizeHex(hasher->finalHex())
        });
    }

    for (auto& similarityHasher : similarityHashers) {
        const std::string signature = similarityHasher->finalSignature();
        if (!signature.empty()) {
            fingerprint.similarityDigests.push_back(SimilarityDigest{
                similarityHasher->algorithm(),
                signature
            });
        }
    }

    if (fingerprint.identityDigests.empty()) {
        throw BlobStoreError("No identity digests were generated");
    }

    return fingerprint;
}

} // namespace blobstore
