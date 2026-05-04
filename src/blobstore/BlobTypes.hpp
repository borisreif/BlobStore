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

struct FuzzyDigest {
    std::string algorithm;
    std::string signature;
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
    std::vector<FuzzyDigest> fuzzyDigests;
};

} // namespace blobstore