#include "../include/BlobStore.h"

#include <cctype>
#include <fstream>
#include <random>
#include <stdexcept>
#include <utility>

namespace blobdb {

namespace fs = std::filesystem;

BlobStore::BlobStore(fs::path root, HashAlgo algo)
    : root_(std::move(root)), blobsDir_(root_ / "blobs"), hasher_(makeHasher(algo)) {
    std::error_code ec;
    fs::create_directories(blobsDir_, ec);
    if (ec) throw std::runtime_error("BlobStore: failed to create blob directory: " + ec.message());
}

BlobStore::BlobStore(fs::path root, std::unique_ptr<IHasher> hasher)
    : root_(std::move(root)), blobsDir_(root_ / "blobs"), hasher_(std::move(hasher)) {
    if (!hasher_) throw std::invalid_argument("BlobStore: hasher is null");
    std::error_code ec;
    fs::create_directories(blobsDir_, ec);
    if (ec) throw std::runtime_error("BlobStore: failed to create blob directory: " + ec.message());
}

std::string BlobStore::putFile(const fs::path& srcPath) {
    if (!fs::exists(srcPath)) {
        throw std::runtime_error("BlobStore::putFile: source does not exist: " + srcPath.string());
    }

    const std::string id = hashFileHex(srcPath);
    const fs::path dst = blobPathFor(id);
    if (fs::exists(dst)) return id;

    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (ec) throw std::runtime_error("BlobStore::putFile: failed to create shard dir: " + ec.message());

    const fs::path tmp = dst.parent_path() / (".tmp-" + randomToken(12));
    {
        std::ifstream in(srcPath, std::ios::binary);
        if (!in) throw std::runtime_error("BlobStore::putFile: failed to open source");
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("BlobStore::putFile: failed to open temp file");

        char buf[1 << 20];
        while (in) {
            in.read(buf, sizeof(buf));
            const std::streamsize n = in.gcount();
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
        if (fs::exists(dst)) {
            std::error_code delEc;
            fs::remove(tmp, delEc);
        } else {
            std::error_code cpEc;
            fs::copy_file(tmp, dst, fs::copy_options::none, cpEc);
            std::error_code delEc;
            fs::remove(tmp, delEc);
            if (cpEc) throw std::runtime_error("BlobStore::putFile: finalize failed: " + cpEc.message());
        }
    }
    return id;
}

std::string BlobStore::putBytes(const std::vector<std::uint8_t>& bytes) {
    const std::string id = hashBytesHex(bytes);
    const fs::path dst = blobPathFor(id);
    if (fs::exists(dst)) return id;

    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (ec) throw std::runtime_error("BlobStore::putBytes: failed to create shard dir: " + ec.message());

    const fs::path tmp = dst.parent_path() / (".tmp-" + randomToken(12));
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("BlobStore::putBytes: failed to open temp file");
        if (!bytes.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
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

bool BlobStore::getToFile(const std::string& id, const fs::path& outPath) const {
    checkId(id);
    const fs::path src = blobPathFor(id);
    if (!fs::exists(src)) return false;

    if (outPath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(outPath.parent_path(), ec);
        if (ec) return false;
    }

    const fs::path tmpDir = outPath.has_parent_path() ? outPath.parent_path() : fs::path(".");
    const fs::path tmp = tmpDir / (".tmp-out-" + randomToken(12));
    {
        std::ifstream in(src, std::ios::binary);
        if (!in) return false;
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;

        char buf[1 << 20];
        while (in) {
            in.read(buf, sizeof(buf));
            const std::streamsize n = in.gcount();
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
        std::error_code cpEc;
        fs::copy_file(tmp, outPath, fs::copy_options::overwrite_existing, cpEc);
        std::error_code delEc;
        fs::remove(tmp, delEc);
        if (cpEc) return false;
    }
    return true;
}

std::optional<std::vector<std::uint8_t>> BlobStore::getBytes(const std::string& id) const {
    checkId(id);
    const fs::path src = blobPathFor(id);
    if (!fs::exists(src)) return std::nullopt;

    std::ifstream in(src, std::ios::binary);
    if (!in) return std::nullopt;

    std::vector<std::uint8_t> data;
    std::error_code ec;
    const auto bytes = fs::file_size(src, ec);
    if (!ec) data.reserve(static_cast<std::size_t>(bytes));

    char buf[1 << 20];
    while (in) {
        in.read(buf, sizeof(buf));
        const std::streamsize n = in.gcount();
        if (n > 0) data.insert(data.end(), buf, buf + n);
    }
    return data;
}

bool BlobStore::exists(const std::string& id) const {
    if (!isValidId(id)) return false;
    return fs::exists(blobPathFor(id));
}

std::uintmax_t BlobStore::size(const std::string& id) const {
    checkId(id);
    std::error_code ec;
    const auto s = fs::file_size(blobPathFor(id), ec);
    if (ec) throw std::runtime_error("BlobStore::size: " + ec.message());
    return s;
}

bool BlobStore::verify(const std::string& id) const {
    checkId(id);
    const fs::path src = blobPathFor(id);
    if (!fs::exists(src)) return false;
    return hashFileHex(src) == id;
}

fs::path BlobStore::pathOf(const std::string& id) const {
    checkId(id);
    return blobPathFor(id);
}

bool BlobStore::erase(const std::string& id) {
    checkId(id);
    std::error_code ec;
    return fs::remove(blobPathFor(id), ec);
}

bool BlobStore::isValidId(const std::string& id) {
    if (id.size() != 64) return false;
    for (unsigned char c : id) {
        if (!std::isxdigit(c)) return false;
    }
    return true;
}

fs::path BlobStore::blobPathFor(const std::string& id) const {
    return blobsDir_ / id.substr(0, 2) / id;
}

std::string BlobStore::hashFileHex(const fs::path& p) const {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("BlobStore::hashFileHex: failed to open: " + p.string());

    hasher_->reset();
    char buf[1 << 20];
    while (in) {
        in.read(buf, sizeof(buf));
        const std::streamsize n = in.gcount();
        if (n > 0) hasher_->update(buf, static_cast<std::size_t>(n));
    }
    return hasher_->finishHex();
}

std::string BlobStore::hashBytesHex(const std::vector<std::uint8_t>& data) const {
    return hasher_->hashBytesHex(data);
}

std::string BlobStore::randomToken(std::size_t len) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    static constexpr char alphabet[] = "0123456789abcdef";
    std::uniform_int_distribution<int> dist(0, 15);
    std::string s;
    s.reserve(len);
    for (std::size_t i = 0; i < len; ++i) s.push_back(alphabet[dist(rng)]);
    return s;
}

void BlobStore::checkId(const std::string& id) {
    if (!isValidId(id)) throw std::invalid_argument("BlobStore: invalid blob id; expected 64 hex chars");
}

} // namespace blobdb
