#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace blobstore {

namespace fs = std::filesystem;

struct HashDigest {
    std::string algorithm;
    std::string hex;
};

struct BlobId {
    std::string algorithm;
    std::string hex;
};

struct PutResult {
    BlobId id;
    bool alreadyExisted;
    fs::path dataPath;
    std::vector<HashDigest> digests;
};

} // namespace blobstore