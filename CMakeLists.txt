cmake_minimum_required(VERSION 3.16)

project(BlobStorePrototype VERSION 0.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BLOBSTORE_WITH_OPENSSL "Build the OpenSSL SHA-256 identity hasher" ON)
option(BLOBSTORE_WITH_BLAKE3 "Build the vendored portable BLAKE3 identity hasher" ON)
option(BLOBSTORE_WITH_SSDEEP "Build the optional ssdeep similarity hasher" OFF)
option(BLOBSTORE_WITH_TLSH "Build the optional TLSH similarity hasher" OFF)

add_library(blobstore
    src/blobstore/BlobStore.cpp
    src/identity_hashing/FnvHasher.cpp
)

target_include_directories(blobstore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(blobstore PUBLIC cxx_std_20)

if(BLOBSTORE_WITH_BLAKE3)
    foreach(BLAKE3_FILE
        blake3.h
        blake3_impl.h
        blake3.c
        blake3_dispatch.c
        blake3_portable.c
    )
        set(BLAKE3_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/blake3/${BLAKE3_FILE})
        if(NOT EXISTS ${BLAKE3_PATH})
            message(FATAL_ERROR "Missing vendored BLAKE3 file: ${BLAKE3_PATH}")
        endif()
        file(READ ${BLAKE3_PATH} BLAKE3_PREVIEW LIMIT 512)
        if(BLAKE3_PREVIEW MATCHES "<!DOCTYPE html")
            message(FATAL_ERROR "${BLAKE3_PATH} looks like a GitHub HTML page, not raw source. Download the raw BLAKE3 C file instead.")
        endif()
    endforeach()

    target_sources(blobstore PRIVATE
        src/identity_hashing/Blake3Hasher.cpp
        src/thirdparty/blake3/blake3.c
        src/thirdparty/blake3/blake3_dispatch.c
        src/thirdparty/blake3/blake3_portable.c
    )

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
        src/identity_hashing/Sha256OpenSSLHasher.cpp
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
        src/similarity_hashing/SsdeepHasher.cpp
    )
    target_include_directories(blobstore PRIVATE ${FUZZY_INCLUDE_DIR})
    target_link_libraries(blobstore PUBLIC ${FUZZY_LIBRARY})
endif()

if(BLOBSTORE_WITH_TLSH)
    find_path(TLSH_INCLUDE_DIR tlsh.h)
    find_library(TLSH_LIBRARY tlsh)

    if(NOT TLSH_INCLUDE_DIR OR NOT TLSH_LIBRARY)
        message(FATAL_ERROR
            "BLOBSTORE_WITH_TLSH=ON but libtlsh/tlsh.h was not found. "
            "Install it with: sudo apt install libtlsh-dev"
        )
    endif()

    target_sources(blobstore PRIVATE
        src/similarity_hashing/TlshHasher.cpp
    )
    target_include_directories(blobstore PRIVATE ${TLSH_INCLUDE_DIR})
    target_link_libraries(blobstore PUBLIC ${TLSH_LIBRARY})
endif()

add_executable(blob src/main.cpp)
target_link_libraries(blob PRIVATE blobstore)

if(BLOBSTORE_WITH_BLAKE3)
    target_compile_definitions(blob PRIVATE BLOBSTORE_WITH_BLAKE3)
endif()

if(BLOBSTORE_WITH_OPENSSL)
    target_compile_definitions(blob PRIVATE BLOBSTORE_WITH_OPENSSL)
endif()

if(BLOBSTORE_WITH_SSDEEP)
    target_compile_definitions(blob PRIVATE BLOBSTORE_WITH_SSDEEP)
endif()

if(BLOBSTORE_WITH_TLSH)
    target_compile_definitions(blob PRIVATE BLOBSTORE_WITH_TLSH)
endif()

if(MSVC)
    target_compile_options(blobstore PRIVATE /W4)
    target_compile_options(blob PRIVATE /W4)
else()
    target_compile_options(blobstore PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(blob PRIVATE -Wall -Wextra -Wpedantic)
endif()
