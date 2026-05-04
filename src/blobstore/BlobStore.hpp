#pragma once

#include "../hashing/IHasher.hpp"
#include "BlobTypes.hpp"

#include <cstdint>     // std::uint8_t, std::uint64_t, etc.
#include <filesystem>  // std::filesystem::path
#include <functional>  // std::function
#include <memory>      // std::unique_ptr
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <vector>      // std::vector
#include <ctime>       // for timestamp

namespace blobstore {

namespace fs = std::filesystem;

class BlobStoreError : public std::runtime_error {
public:
    explicit BlobStoreError(const std::string& message)
        : std::runtime_error(message) {}
};

class HashCollisionError : public BlobStoreError {
public:
    explicit HashCollisionError(const std::string& message)
        : BlobStoreError(message) {}
};

class BlobStore {
public:
    BlobStore(
        fs::path root,
        std::vector<hashing::HasherFactory> hashers,
        std::size_t chunkChars = 8
    );

    PutResult putFile(const fs::path& sourcePath);

    fs::path pathFor(const BlobId& id) const;
    bool contains(const BlobId& id) const;
    bool verify(const BlobId& id) const;

    std::vector<HashDigest> readMetaFor(const BlobId& id) const;

private:
    fs::path root_;
    fs::path objectsDir_;
    fs::path tmpDir_;

    std::vector<hashing::HasherFactory> hasherFactories_;
    std::string primaryAlgorithm_;
    std::size_t chunkChars_;

    std::vector<std::unique_ptr<hashing::IHasher>> makeHashers() const;

    fs::path objectDirForDigest(const std::string& hexDigest) const;
    fs::path dataPathForDigest(const std::string& hexDigest) const;
    fs::path metaPathForDigest(const std::string& hexDigest) const;

    fs::path makeTempPath() const;

    void writeMeta(
        const std::string& primaryDigest,
        std::uintmax_t size,
        const std::vector<HashDigest>& digests
    ) const;

    static bool equalFiles(const fs::path& a, const fs::path& b);
    static std::string normalizeHex(std::string hex);
    static void requireCanonicalId(
        const BlobId& id,
        const std::string& primaryAlgorithm
    );
};

} // namespace blobstore