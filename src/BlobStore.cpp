#include "BlobStore.h"

// #include "BlobStore.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <random>
#include <stdexcept>
#include <cctype>

#include <openssl/sha.h> // OpenSSL for SHA-256

namespace blobdb {

namespace fs = std::filesystem;

// ---------- ctor ----------
BlobStore::BlobStore(fs::path root)
    : root_(std::move(root)), blobsDir_(root_ / "blobs")
{
    std::error_code ec;
    fs::create_directories(blobsDir_, ec);
    if (ec) throw std::runtime_error("BlobStore: failed to create blob directory: " + ec.message());
}

// ---------- public: putFile ----------
std::string BlobStore::putFile(const fs::path& srcPath) {
    if (!fs::exists(srcPath)) throw std::runtime_error("BlobStore::putFile: source does not exist");

    // Hash while streaming
    std::string id = sha256File(srcPath);
    fs::path dst = blobPathFor(id);

    // Fast path: already stored
    if (fs::exists(dst)) return id;

    // Ensure shard directory exists
    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (ec) throw std::runtime_error("BlobStore::putFile: failed to create shard dir: " + ec.message());

    // Atomic write: write to tmp then rename
    fs::path tmp = dst.parent_path() / (".tmp-" + randomToken(12));
    {
        std::ifstream in(srcPath, std::ios::binary);
        if (!in) throw std::runtime_error("BlobStore::putFile: failed to open source");

        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("BlobStore::putFile: failed to open temp");

        char buf[1 << 20]; // 1 MB
        while (in) {
            in.read(buf, sizeof(buf));
            std::streamsize n = in.gcount();
            if (n > 0) out.write(buf, n);
        }
        out.flush();
        if (!out) {
            std::error_code delEc;
            fs::remove(tmp, delEc);
            throw std::runtime_error("BlobStore::putFile: write error");
        }
    }

    std::error_code rnEc;
    fs::rename(tmp, dst, rnEc);
    if (rnEc) {
        // If destination now exists, another writer won; remove tmp
        if (fs::exists(dst)) {
            std::error_code delEc;
            fs::remove(tmp, delEc);
        } else {
            // Fallback to copy+remove (cross-device or Windows)
            std::error_code cpEc;
            fs::copy_file(tmp, dst, fs::copy_options::none, cpEc);
            std::error_code delEc;
            fs::remove(tmp, delEc);
            if (cpEc) throw std::runtime_error("BlobStore::putFile: finalize failed: " + cpEc.message());
        }
    }

    return id;
}

// ---------- public: putBytes ----------
std::string BlobStore::putBytes(const std::vector<std::uint8_t>& bytes) {
    std::string id = sha256Bytes(bytes);
    fs::path dst = blobPathFor(id);

    if (fs::exists(dst)) return id;

    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (ec) throw std::runtime_error("BlobStore::putBytes: failed to create shard dir: " + ec.message());

    fs::path tmp = dst.parent_path() / (".tmp-" + randomToken(12));
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("BlobStore::putBytes: failed to open temp");
        if (!bytes.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        out.flush();
        if (!out) {
            std::error_code delEc;
            fs::remove(tmp, delEc);
            throw std::runtime_error("BlobStore::putBytes: write error");
        }
    }

    std::error_code rnEc;
    fs::rename(tmp, dst, rnEc);
    if (rnEc) {
        if (fs::exists(dst)) {
            std::error_code delEc;
            fs::remove(tmp, delEc);
        } else {
            std::error_code cpEc;
            fs::copy_file(tmp, dst, fs::copy_options::none, cpEc);
            std::error_code delEc;
            fs::remove(tmp, delEc);
            if (cpEc) throw std::runtime_error("BlobStore::putBytes: finalize failed: " + cpEc.message());
        }
    }
    return id;
}

// ---------- public: getToFile ----------
bool BlobStore::getToFile(const std::string& id, const fs::path& outPath) const {
    checkId(id);
    fs::path src = blobPathFor(id);
    if (!fs::exists(src)) return false;

    if (outPath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(outPath.parent_path(), ec); // ok if already exists
    }

    fs::path tmp = (outPath.has_parent_path()
                        ? outPath.parent_path()
                        : fs::path(".")) /
                   (".tmp-out-" + randomToken(12));
    {
        std::ifstream in(src, std::ios::binary);
        if (!in) return false;
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;

        char buf[1 << 20];
        while (in) {
            in.read(buf, sizeof(buf));
            std::streamsize n = in.gcount();
            if (n > 0) out.write(buf, n);
        }
        out.flush();
        if (!out) {
            std::error_code delEc;
            fs::remove(tmp, delEc);
            return false;
        }
    }
    std::error_code rnEc;
    fs::rename(tmp, outPath, rnEc);
    if (rnEc) {
        // Overwrite if exists
        std::error_code cpEc;
        fs::copy_file(tmp, outPath, fs::copy_options::overwrite_existing, cpEc);
        std::error_code delEc;
        fs::remove(tmp, delEc);
        if (cpEc) return false;
    }
    return true;
}

// ---------- public: getBytes ----------
std::optional<std::vector<std::uint8_t>> BlobStore::getBytes(const std::string& id) const {
    checkId(id);
    fs::path src = blobPathFor(id);
    if (!fs::exists(src)) return std::nullopt;

    std::ifstream in(src, std::ios::binary);
    if (!in) return std::nullopt;

    std::vector<std::uint8_t> data;
    data.reserve(static_cast<size_t>(fs::file_size(src)));

    char buf[1 << 20];
    while (in) {
        in.read(buf, sizeof(buf));
        std::streamsize n = in.gcount();
        if (n > 0) data.insert(data.end(), buf, buf + n);
    }
    return data;
}

// ---------- public: exists ----------
bool BlobStore::exists(const std::string& id) const {
    if (!isValidId(id)) return false;
    return fs::exists(blobPathFor(id));
}

// ---------- public: size ----------
std::uintmax_t BlobStore::size(const std::string& id) const {
    checkId(id);
    std::error_code ec;
    auto s = fs::file_size(blobPathFor(id), ec);
    if (ec) throw std::runtime_error("BlobStore::size: " + ec.message());
    return s;
}

// ---------- public: verify ----------
bool BlobStore::verify(const std::string& id) const {
    checkId(id);
    fs::path src = blobPathFor(id);
    if (!fs::exists(src)) return false;
    return sha256File(src) == id;
}

// ---------- public: pathOf ----------
fs::path BlobStore::pathOf(const std::string& id) const {
    checkId(id);
    return blobPathFor(id);
}

// ---------- public: erase ----------
bool BlobStore::erase(const std::string& id) {
    checkId(id);
    std::error_code ec;
    return fs::remove(blobPathFor(id), ec);
}

// ---------- public static: isValidId ----------
bool BlobStore::isValidId(const std::string& id) {
    if (id.size() != 64) return false;
    for (unsigned char c : id) {
        if (!std::isxdigit(c)) return false;
    }
    return true;
}

// ---------- private: blobPathFor ----------
fs::path BlobStore::blobPathFor(const std::string& id) const {
    // shard by first 2 hex chars to avoid huge dirs
    std::string shard = id.substr(0, 2);
    return blobsDir_ / shard / id;
}

// ---------- private static: toHex ----------
std::string BlobStore::toHex(const unsigned char* bytes, std::size_t n) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < n; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    }
    return oss.str();
}

// ---------- private static: sha256File ----------
std::string BlobStore::sha256File(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("BlobStore::sha256File: failed to open");

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buf[1 << 20];
    while (in) {
        in.read(buf, sizeof(buf));
        std::streamsize n = in.gcount();
        if (n > 0) SHA256_Update(&ctx, buf, static_cast<size_t>(n));
    }
    unsigned char out[SHA256_DIGEST_LENGTH];
    SHA256_Final(out, &ctx);
    return toHex(out, SHA256_DIGEST_LENGTH);
}

// ---------- private static: sha256Bytes ----------
std::string BlobStore::sha256Bytes(const std::vector<std::uint8_t>& data) {
    unsigned char out[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    if (!data.empty()) SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(out, &ctx);
    return toHex(out, SHA256_DIGEST_LENGTH);
}

// ---------- private static: randomToken ----------
std::string BlobStore::randomToken(std::size_t len) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    static const char* alphabet = "0123456789abcdef";
    std::uniform_int_distribution<int> dist(0, 15);
    std::string s; s.reserve(len);
    for (std::size_t i = 0; i < len; ++i) s.push_back(alphabet[dist(rng)]);
    return s;
}

// ---------- private static: checkId ----------
void BlobStore::checkId(const std::string& id) {
    if (!isValidId(id)) throw std::invalid_argument("BlobStore: invalid blob id (expect 64 hex chars)");
}

} // namespace blobdb
