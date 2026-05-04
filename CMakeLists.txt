cmake_minimum_required(VERSION 3.16)

project(BlobStorePrototype VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(blobstore
    src/BlobStore.cpp
    src/HashFactory.cpp
    src/Sha256Hasher.cpp
)

target_include_directories(blobstore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(blob main.cpp)
target_link_libraries(blob PRIVATE blobstore)

if(MSVC)
    target_compile_options(blobstore PRIVATE /W4)
    target_compile_options(blob PRIVATE /W4)
else()
    target_compile_options(blobstore PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(blob PRIVATE -Wall -Wextra -Wpedantic)
endif()
