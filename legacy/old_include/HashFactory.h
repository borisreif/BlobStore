#pragma once

#include <memory>
#include <string>

namespace blobdb {

class IHasher;

enum class HashAlgo {
    SHA256,
    BLAKE3
};

std::unique_ptr<IHasher> makeHasher(HashAlgo algo = HashAlgo::SHA256);
HashAlgo parseHashAlgo(const std::string& s);

} // namespace blobdb
