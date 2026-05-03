#pragma once

#include "HashFactory.h"
#include "IHasher.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace blobdb {

namespace fs = std::filesystem;

class BlobStore {
public:
    // Default to self-contained SHA-256. The earlier BLAKE3 setup depended on
    // ../../../BLAKE3/c, which was outside the project and broke portability.
    explicit BlobStore(fs::path root, HashAlgo algo = HashAlgo::SHA256);

    // Or inject your own hasher, e.g. for tests.
    BlobStore(fs::path root, std::unique_ptr<IHasher> hasher);

    std::string putFile(const fs::path& srcPath);
    std::string putBytes(const std::vector<std::uint8_t>& bytes);

    bool getToFile(const std::string& id, const fs::path& outPath) const;
    std::optional<std::vector<std::uint8_t>> getBytes(const std::string& id) const;

    bool exists(const std::string& id) const;
    std::uintmax_t size(const std::string& id) const;
    bool verify(const std::string& id) const;
    fs::path pathOf(const std::string& id) const;
    bool erase(const std::string& id);

    static bool isValidId(const std::string& id);

private:
    fs::path root_;
    fs::path blobsDir_;
    std::unique_ptr<IHasher> hasher_;

    fs::path blobPathFor(const std::string& id) const;
    std::string hashFileHex(const fs::path& p) const;
    std::string hashBytesHex(const std::vector<std::uint8_t>& data) const;

    static std::string randomToken(std::size_t len);
    static void checkId(const std::string& id);
};

} // namespace blobdb
