cmake_minimum_required(VERSION 3.16)

project(BlobStorePrototype VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BLOBSTORE_WITH_OPENSSL "Build the OpenSSL SHA-256 hasher" ON)
option(BLOBSTORE_WITH_BLAKE3 "Build the vendored portable BLAKE3 hasher" OFF)
option(BLOBSTORE_WITH_SSDEEP "Build the optional ssdeep fuzzy hasher" OFF)

add_library(blobstore
    src/blobstore/BlobStore.cpp
    src/hashing/FnvHasher.cpp
)

target_include_directories(blobstore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(blobstore PUBLIC cxx_std_20)

if(BLOBSTORE_WITH_BLAKE3)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/blake3/blake3.h BLAKE3_HEADER_PREVIEW LIMIT 4096)
    if(BLAKE3_HEADER_PREVIEW MATCHES "<!DOCTYPE html")
        message(FATAL_ERROR "The vendored BLAKE3 files look like GitHub HTML pages, not raw source files. Re-download the raw files from raw.githubusercontent.com or leave BLOBSTORE_WITH_BLAKE3=OFF.")
    endif()
    target_sources(blobstore PRIVATE
        src/hashing/Blake3Hasher.cpp
        src/third_party/blake3/blake3.c
        src/third_party/blake3/blake3_dispatch.c
        src/third_party/blake3/blake3_portable.c
    )

    # Keep the first BLAKE3 integration portable and simple. SIMD files can be
    # added later when the storage design is stable.
    target_compile_definitions(blobstore PRIVATE
        BLAKE3_NO_SSE2
        BLAKE3_NO_SSE41
        BLAKE3_NO_AVX2
        BLAKE3_NO_AVX512
    )
endif()

if(BLOBSTORE_WITH_OPENSSL)
    find_package(OpenSSL REQUIRED)
    target_sources(blobstore PRIVATE
        src/hashing/Sha256OpenSSLHasher.cpp
    )
    target_link_libraries(blobstore PUBLIC OpenSSL::Crypto)
endif()

if(BLOBSTORE_WITH_SSDEEP)
    find_path(FUZZY_INCLUDE_DIR fuzzy.h)
    find_library(FUZZY_LIBRARY fuzzy)

    if(NOT FUZZY_INCLUDE_DIR OR NOT FUZZY_LIBRARY)
        message(FATAL_ERROR "BLOBSTORE_WITH_SSDEEP=ON but libfuzzy/fuzzy.h was not found")
    endif()

    target_sources(blobstore PRIVATE
        src/meta_hashing/SsdeepHasher.cpp
    )
    target_include_directories(blobstore PRIVATE ${FUZZY_INCLUDE_DIR})
    target_link_libraries(blobstore PUBLIC ${FUZZY_LIBRARY})
endif()

add_executable(blob main.cpp)
target_link_libraries(blob PRIVATE blobstore)

if(MSVC)
    target_compile_options(blobstore PRIVATE /W4)
    target_compile_options(blob PRIVATE /W4)
else()
    target_compile_options(blobstore PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(blob PRIVATE -Wall -Wextra -Wpedantic)
endif()
