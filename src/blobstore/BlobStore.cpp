#include "BlobStore.hpp"

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <system_error>
#include <unordered_map>
#include <utility>

namespace blobstore {

namespace {

/**
 * @brief Default file I/O buffer size used while streaming payload bytes.
 *
 * This is unrelated to BlobStore::pathChunkChars_. The I/O chunk is the number
 * of bytes read from disk at once; the path chunk is the number of hex
 * characters used in each path component.
 */
constexpr std::size_t DefaultIoChunkSize = 64 * 1024;

} // anonymous namespace

BlobStore::BlobStore(
    fs::path root,
    std::vector<identity_hashing::IdentityHasherFactory> identityHashers,
    std::vector<similarity_hashing::SimilarityHasherFactory> similarityHashers,
    std::size_t pathChunkChars
)
    : root_(std::move(root)),
      objectsDir_(root_ / "OBJECTS"),
      tmpDir_(root_ / "TMP"),
      fingerprintGenerator_(
          std::move(identityHashers),
          std::move(similarityHashers)
      ),
      primaryAlgorithm_(fingerprintGenerator_.primaryAlgorithm()),
      pathChunkChars_(pathChunkChars)
{
    if (pathChunkChars_ == 0 || pathChunkChars_ > 8) {
        throw BlobStoreError("pathChunkChars should be between 1 and 8");
    }

    fs::create_directories(objectsDir_);
    fs::create_directories(tmpDir_);
}

PutResult BlobStore::putFile(const fs::path& sourcePath) {
    fs::path tmpPath = makeTempPath();

    BlobFingerprint fingerprint =
        fingerprintGenerator_.generateForFileAndCopyTo(sourcePath, tmpPath);

    const HashDigest& primary = fingerprint.primaryDigest();

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
                fingerprint.identityDigests,
                fingerprint.similarityDigests
            };
        }

        throw HashCollisionError(
            "Hash collision or corrupted store detected for digest: " + primary.hex
        );
    }

    fs::rename(tmpPath, dataPath);

    writeMeta(primary.hex, fingerprint.size, fingerprint.identityDigests, fingerprint.similarityDigests);

    return PutResult{
        BlobId{primary.algorithm, primary.hex},
        false,
        dataPath,
        fingerprint.identityDigests,
        fingerprint.similarityDigests
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

    const BlobFingerprint fingerprint = fingerprintGenerator_.generateForFile(dataPath);

    for (const auto& digest : fingerprint.identityDigests) {
        std::string algorithm = digest.algorithm;
        std::string actual = normalizeHex(digest.hex);

        auto it = expected.find(algorithm);

        if (it == expected.end()) {
            return false;
        }

        if (it->second != actual) {
            return false;
        }
    }

    return true;
}

bool BlobStore::copyToFile(const BlobId& id, const fs::path& outputPath) const {
    fs::path inputPath = pathFor(id);

    if (!fs::exists(inputPath)) {
        return false;
    }

    if (outputPath.has_parent_path()) {
        fs::create_directories(outputPath.parent_path());
    }

    std::error_code ec;
    fs::copy_file(inputPath, outputPath, fs::copy_options::overwrite_existing, ec);

    return !ec;
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

std::vector<SimilarityDigest> BlobStore::readSimilarityMetaFor(const BlobId& id) const {
    requireCanonicalId(id, primaryAlgorithm_);

    fs::path metaPath = metaPathForDigest(normalizeHex(id.hex));

    std::ifstream in(metaPath);
    if (!in) {
        throw BlobStoreError("Could not open metadata file: " + metaPath.string());
    }

    std::vector<SimilarityDigest> result;
    std::string line;

    while (std::getline(in, line)) {
        const std::string prefix = "similarity.";

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

        std::string signature = line.substr(equalsPos + 1);

        result.push_back(SimilarityDigest{
            algorithm,
            signature
        });
    }

    return result;
}

fs::path BlobStore::objectDirForDigest(const std::string& hexDigestRaw) const {
    std::string hexDigest = normalizeHex(hexDigestRaw);

    fs::path path = objectsDir_;

    for (std::size_t i = 0; i < hexDigest.size(); i += pathChunkChars_) {
        path /= hexDigest.substr(i, pathChunkChars_);
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
    const std::vector<HashDigest>& digests,
    const std::vector<SimilarityDigest>& similarityDigests
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

    for (const auto& similarityDigest : similarityDigests) {
        out << "similarity."
            << similarityDigest.algorithm
            << "="
            << similarityDigest.signature
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
