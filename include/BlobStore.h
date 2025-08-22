#ifndef BLOBSTORE_H
#define BLOBSTORE_H


#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <cstdint>

namespace blobdb {

namespace fs = std::filesystem;

class BlobStore {
public:
    explicit BlobStore(fs::path root);

    // Store operations
    std::string putFile(const fs::path& srcPath);
    std::string putBytes(const std::vector<std::uint8_t>& bytes);

    // Retrieve operations
    bool getToFile(const std::string& id, const fs::path& outPath) const;
    std::optional<std::vector<std::uint8_t>> getBytes(const std::string& id) const;

    // Queries / utilities
    bool exists(const std::string& id) const;
    std::uintmax_t size(const std::string& id) const;
    bool verify(const std::string& id) const;      // re-hash and compare to id
    fs::path pathOf(const std::string& id) const;  // location on disk for zero-copy reads
    bool erase(const std::string& id);             // remove blob (ensure unreferenced first)

    // Validate a prospective blob id (hex SHA-256)
    static bool isValidId(const std::string& id);

private:
    fs::path root_;
    fs::path blobsDir_;

    // Helpers
    fs::path blobPathFor(const std::string& id) const;
    static std::string toHex(const unsigned char* bytes, std::size_t n);
    static std::string sha256File(const fs::path& p);
    static std::string sha256Bytes(const std::vector<std::uint8_t>& data);
    static std::string randomToken(std::size_t len);
    static void checkId(const std::string& id);
};

} // namespace blobdb


#endif // BLOBSTORE_H
