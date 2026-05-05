#include "FnvHasher.hpp"

#include <memory>

namespace identity_hashing {

IdentityHasherFactory makeFnv1_32Factory() {
    return [] {
        return std::make_unique<Fnv1_32Hasher>();
    };
}

IdentityHasherFactory makeFnv1_64Factory() {
    return [] {
        return std::make_unique<Fnv1_64Hasher>();
    };
}

IdentityHasherFactory makeFnv1a_32Factory() {
    return [] {
        return std::make_unique<Fnv1a_32Hasher>();
    };
}

IdentityHasherFactory makeFnv1a_64Factory() {
    return [] {
        return std::make_unique<Fnv1a_64Hasher>();
    };
}

} // namespace identity_hashing
