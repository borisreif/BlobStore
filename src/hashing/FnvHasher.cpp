#include "FnvHasher.hpp"

#include <memory>

namespace hashing {

HasherFactory makeFnv1_32Factory() {
    return [] {
        return std::make_unique<Fnv1_32Hasher>();
    };
}

HasherFactory makeFnv1_64Factory() {
    return [] {
        return std::make_unique<Fnv1_64Hasher>();
    };
}

HasherFactory makeFnv1a_32Factory() {
    return [] {
        return std::make_unique<Fnv1a_32Hasher>();
    };
}

HasherFactory makeFnv1a_64Factory() {
    return [] {
        return std::make_unique<Fnv1a_64Hasher>();
    };
}

} // namespace hashing