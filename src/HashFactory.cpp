#include "../include/HashFactory.h"

#include "../include/IHasher.h"
#include "../include/Sha256Hasher.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace blobdb {

std::unique_ptr<IHasher> makeHasher(HashAlgo algo) {
    switch (algo) {
        case HashAlgo::SHA256:
            return std::make_unique<Sha256Hasher>();
        case HashAlgo::BLAKE3:
            throw std::runtime_error(
                "BLAKE3 is not available in this cleaned project. "
                "Vendor BLAKE3 under third_party/blake3/c and add it to CMake first."
            );
    }
    return std::make_unique<Sha256Hasher>();
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

HashAlgo parseHashAlgo(const std::string& s) {
    const auto v = lower(s);
    if (v == "blake3" || v == "b3") return HashAlgo::BLAKE3;
    return HashAlgo::SHA256;
}

} // namespace blobdb
