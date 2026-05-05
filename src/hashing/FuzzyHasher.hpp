#pragma once

#include "../metahashing/IFuzzyHasher.hpp"

// #include <string>

/**
 * The Fuzzy hash function
 * 
 * @link https://en.wikipedia.org/wiki/Fuzzy_hashing
 * @link https://docs.rspamd.com/tutorials/fuzzy_storage/
 * @link https://www.microsoft.com/en-us/security/blog/2021/07/27/combing-through-the-fuzz-using-fuzzy-hashing-and-deep-learning-to-counter-malware-detection-evasion-techniques/
 *
 * This header used to contain an experimental, unfinished FuzzyHasher class.
 * The project now keeps fuzzy hashing separate from exact hashing through the
 * IFuzzyHasher interface in `src/metahashing/IFuzzyHasher.hpp`.
 *
 * Exact hashers implement `IHasher` and return hexadecimal digests.
 * Fuzzy hashers implement `IFuzzyHasher` and return algorithm-specific
 * similarity signatures. Keeping the two concepts separate prevents fuzzy
 * signatures from accidentally being used as canonical blob identities.
 *
 * @author Boris A. Reif
 * @version 0.0.2
 */

namespace hashing {

/**
 * @brief Compatibility alias for the fuzzy hasher strategy interface.
 *
 * Include this file if you want the conceptual name `FuzzyHasher` while the
 * implementation continues to use the clearer IFuzzyHasher interface.
 */
using FuzzyHasher = IFuzzyHasher;

} // namespace hashing
