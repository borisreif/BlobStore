#include "BlobStore.hpp"

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <random>
#include <span>
#include <sstream>
#include <unordered_map>

namespace blobstore {

namespace {

constexpr std::size_t DefaultIoChunkSize = 64 * 1024;

} // anonymous namespace

BlobStore::BlobStore(
    fs::path root,
    std::vector<hashing::HasherFactory> hashers,
    std::size_t chunkChars
)
    : root_(std::move(root)),
      objectsDir_(root_ / "OBJECTS"),
      tmpDir_(root_ / "TMP"),
      hasherFactories_(std::move(hashers)),
      chunkChars_(chunkChars)
{
    if (hasherFactories_.empty()) {
        throw BlobStoreError("BlobStore needs at least one hasher");
    }

    if (chunkChars_ == 0 || chunkChars_ > 8) {
        throw BlobStoreError("chunkChars should be between 1 and 8");
    }

    auto primaryHasher = hasherFactories_.front()();
    primaryAlgorithm_ = primaryHasher->algorithm();

    fs::create_directories(objectsDir_);
    fs::create_directories(tmpDir_);
}

std::vector<std::unique_ptr<hashing::IHasher>> BlobStore::makeHashers() const {
    std::vector<std::unique_ptr<hashing::IHasher>> result;

    for (const auto& factory : hasherFactories_) {
        result.push_back(factory());
    }

    return result;
}

PutResult BlobStore::putFile(const fs::path& sourcePath) {
    if (!fs::exists(sourcePath)) {
        throw BlobStoreError("Source file does not exist: " + sourcePath.string());
    }

    if (!fs::is_regular_file(sourcePath)) {
        throw BlobStoreError("Source path is not a regular file: " + sourcePath.string());
    }

    auto hashers = makeHashers();

    fs::path tmpPath = makeTempPath();

    std::ifstream in(sourcePath, std::ios::binary);
    if (!in) {
        throw BlobStoreError("Could not open source file: " + sourcePath.string());
    }

    std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw BlobStoreError("Could not create temporary file: " + tmpPath.string());
    }

    std::vector<std::uint8_t> buffer(DefaultIoChunkSize);

    std::uintmax_t totalSize = 0;

    while (in) {
        in.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );

        std::streamsize got = in.gcount();

        if (got > 0) {
            auto n = static_cast<std::size_t>(got);

            std::span<const std::uint8_t> chunk(buffer.data(), n);

            for (auto& hasher : hashers) {
                hasher->update(chunk);
            }

            out.write(
                reinterpret_cast<const char*>(buffer.data()),
                got
            );

            totalSize += static_cast<std::uintmax_t>(got);
        }
    }

    out.close();

    if (!out) {
        fs::remove(tmpPath);
        throw BlobStoreError("Failed while writing temporary file");
    }

    std::vector<HashDigest> digests;

    for (auto& hasher : hashers) {
        digests.push_back(HashDigest{
            hasher->algorithm(),
            normalizeHex(hasher->finalHex())
        });
    }

    const HashDigest& primary = digests.front();

    fs::path objectDir = objectDirForDigest(primary.hex);
    fs::path dataPath = objectDir / "DATA.BLB";

    fs::create_directories(objectDir);

    if (fs::exists(dataPath)) {
        bool sameBytes = equalFiles(tmpPath, dataPath);

        fs::remove(tmpPath);

        if (sameBytes) {
            return PutResult{
                BlobId{primary.algorithm, primary.hex},
                true,
                dataPath,
                digests
            };
        }

        throw HashCollisionError(
            "Hash collision or corrupted store detected for digest: " + primary.hex
        );
    }

    fs::rename(tmpPath, dataPath);

    writeMeta(primary.hex, totalSize, digests);

    return PutResult{
        BlobId{primary.algorithm, primary.hex},
        false,
        dataPath,
        digests
    };
}

fs::path BlobStore::pathFor(const BlobId& id) const {
    requireCanonicalId(id, primaryAlgorithm_);

    return dataPathForDigest(normalizeHex(id.hex));
}

bool BlobStore::contains(const BlobId& id) const {
    return fs::exists(pathFor(id));
}

bool BlobStore::verify(const BlobId& id) const {
    requireCanonicalId(id, primaryAlgorithm_);

    fs::path dataPath = pathFor(id);

    if (!fs::exists(dataPath)) {
        return false;
    }

    std::vector<HashDigest> storedDigests = readMetaFor(id);

    std::unordered_map<std::string, std::string> expected;

    for (const auto& digest : storedDigests) {
        expected[digest.algorithm] = normalizeHex(digest.hex);
    }

    auto hashers = makeHashers();

    std::ifstream in(dataPath, std::ios::binary);
    if (!in) {
        return false;
    }

    std::vector<std::uint8_t> buffer(DefaultIoChunkSize);

    while (in) {
        in.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );

        std::streamsize got = in.gcount();

        if (got > 0) {
            auto n = static_cast<std::size_t>(got);

            std::span<const std::uint8_t> chunk(buffer.data(), n);

            for (auto& hasher : hashers) {
                hasher->update(chunk);
            }
        }
    }

    for (auto& hasher : hashers) {
        std::string algorithm = hasher->algorithm();
        std::string actual = normalizeHex(hasher->finalHex());

        auto it = expected.find(algorithm);

        if (it == expected.end()) {
            continue;
        }

        if (it->second != actual) {
            return false;
        }
    }

    return true;
}

std::vector<HashDigest> BlobStore::readMetaFor(const BlobId& id) const {
    requireCanonicalId(id, primaryAlgorithm_);

    fs::path metaPath = metaPathForDigest(normalizeHex(id.hex));

    std::ifstream in(metaPath);
    if (!in) {
        throw BlobStoreError("Could not open metadata file: " + metaPath.string());
    }

    std::vector<HashDigest> result;
    std::string line;

    while (std::getline(in, line)) {
        const std::string prefix = "hash.";

        if (line.rfind(prefix, 0) != 0) {
            continue;
        }

        auto equalsPos = line.find('=');

        if (equalsPos == std::string::npos) {
            continue;
        }

        std::string algorithm = line.substr(
            prefix.size(),
            equalsPos - prefix.size()
        );

        std::string hex = line.substr(equalsPos + 1);

        result.push_back(HashDigest{
            algorithm,
            normalizeHex(hex)
        });
    }

    return result;
}

fs::path BlobStore::objectDirForDigest(const std::string& hexDigestRaw) const {
    std::string hexDigest = normalizeHex(hexDigestRaw);

    fs::path path = objectsDir_;

    for (std::size_t i = 0; i < hexDigest.size(); i += chunkChars_) {
        path /= hexDigest.substr(i, chunkChars_);
    }

    return path;
}

fs::path BlobStore::dataPathForDigest(const std::string& hexDigest) const {
    return objectDirForDigest(hexDigest) / "DATA.BLB";
}

fs::path BlobStore::metaPathForDigest(const std::string& hexDigest) const {
    return objectDirForDigest(hexDigest) / "META.TXT";
}

fs::path BlobStore::makeTempPath() const {
    static thread_local std::mt19937_64 rng{
        static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch()
                .count()
        )
    };

    for (int attempt = 0; attempt < 100; ++attempt) {
        std::uint64_t value = rng() & 0x0FFFFFFFULL;

        std::ostringstream name;

        name << 'T'
             << std::uppercase
             << std::hex
             << std::setw(7)
             << std::setfill('0')
             << value
             << ".TMP";

        fs::path candidate = tmpDir_ / name.str();

        if (!fs::exists(candidate)) {
            return candidate;
        }
    }

    throw BlobStoreError("Could not create unique temporary filename");
}

void BlobStore::writeMeta(
    const std::string& primaryDigest,
    std::uintmax_t size,
    const std::vector<HashDigest>& digests
) const {
    fs::path objectDir = objectDirForDigest(primaryDigest);

    fs::create_directories(objectDir);

    fs::path tmpMeta = objectDir / "META.TMP";
    fs::path finalMeta = objectDir / "META.TXT";

    std::ofstream out(tmpMeta, std::ios::trunc);

    if (!out) {
        throw BlobStoreError("Could not create metadata file: " + tmpMeta.string());
    }

    out << "version=1\n";
    out << "size=" << size << "\n";
    out << "primary=" << primaryAlgorithm_ << "\n";

    for (const auto& digest : digests) {
        out << "hash."
            << digest.algorithm
            << "="
            << normalizeHex(digest.hex)
            << "\n";
    }

    out.close();

    if (!out) {
        throw BlobStoreError("Failed while writing metadata file");
    }

    fs::rename(tmpMeta, finalMeta);
}

bool BlobStore::equalFiles(const fs::path& a, const fs::path& b) {
    if (!fs::exists(a) || !fs::exists(b)) {
        return false;
    }

    if (fs::file_size(a) != fs::file_size(b)) {
        return false;
    }

    std::ifstream fa(a, std::ios::binary);
    std::ifstream fb(b, std::ios::binary);

    if (!fa || !fb) {
        return false;
    }

    std::vector<char> ba(DefaultIoChunkSize);
    std::vector<char> bb(DefaultIoChunkSize);

    while (true) {
        fa.read(ba.data(), static_cast<std::streamsize>(ba.size()));
        fb.read(bb.data(), static_cast<std::streamsize>(bb.size()));

        std::streamsize na = fa.gcount();
        std::streamsize nb = fb.gcount();

        if (na != nb) {
            return false;
        }

        if (na == 0) {
            return true;
        }

        if (std::memcmp(
                ba.data(),
                bb.data(),
                static_cast<std::size_t>(na)
            ) != 0
        ) {
            return false;
        }
    }
}

std::string BlobStore::normalizeHex(std::string hex) {
    for (char& c : hex) {
        if (c >= 'a' && c <= 'f') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }

    return hex;
}

void BlobStore::requireCanonicalId(
    const BlobId& id,
    const std::string& primaryAlgorithm
) {
    if (id.algorithm != primaryAlgorithm) {
        throw BlobStoreError(
            "This database-free layout can locate blobs only by canonical hash. "
            "Expected algorithm: " + primaryAlgorithm +
            ", got: " + id.algorithm
        );
    }
}

} // namespace blobstore