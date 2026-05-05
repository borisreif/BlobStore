#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace blobdb {

class IHasher {
public:
    virtual ~IHasher() = default;
    virtual void reset() = 0;
    virtual void update(const void* data, std::size_t len) = 0;
    virtual std::string finishHex() = 0;

    std::string hashBytesHex(const std::vector<std::uint8_t>& bytes) {
        reset();
        if (!bytes.empty()) update(bytes.data(), bytes.size());
        return finishHex();
    }
};

} // namespace blobdb
