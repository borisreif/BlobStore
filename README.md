# BlobStore prototype

This is a cleaned VS Code/CMake version of the blob store prototype.

## What was fixed

- Removed hardcoded `/home/boris-a-reif/...` paths from the VS Code launcher and `main.cpp`.
- Removed the broken dependency on `../../../BLAKE3/c`, which pointed outside the project folder.
- Restored the missing `BlobStore` retrieval/query methods. The previous `getToFile()` implementation was only a placeholder that always returned `true`.
- Fixed the `std::unique_ptr<IHasher>` incomplete-type problem by making `IHasher` complete in `BlobStore.h`.
- Made the build reproducible with `CMakeLists.txt` and VS Code `tasks.json`.
- Switched the default hasher to a self-contained SHA-256 implementation, matching the existing stored blob id for `test_file.pdf`.
- Removed generated binaries/objects from the source project layout.

## Current cleanup pass

The project had two architectures side by side: an older `include/` + `src/BlobStore.cpp` version and a newer `src/blobstore` + `src/hashing` version. The current CMake build now uses the newer architecture:

```text
src/blobstore/
    BlobStore.hpp
    BlobStore.cpp
    BlobTypes.hpp

src/hashing/
    IHasher.hpp
    FnvHasher.hpp
    FnvHasher.tpp
    FnvHasher.cpp
    Sha256OpenSSLHasher.hpp
    Sha256OpenSSLHasher.cpp
    Blake3Hasher.hpp
    Blake3Hasher.cpp

src/meta_hashing/
    IFuzzyHasher.hpp
    SsdeepHasher.hpp
    SsdeepHasher.cpp
```

The older files under `include/` and the old `src/BlobStore.cpp`, `src/HashFactory.cpp`, and `src/Sha256Hasher.cpp` are still present as reference material, but they are no longer part of the default build.

Additional fixes in this pass:

- Updated `CMakeLists.txt` to build the newer `src/blobstore` / `src/hashing` layout.
- Kept BLAKE3 optional because the vendored BLAKE3 files in this archive are GitHub HTML pages, not raw C source files.
- Added a CMake sanity check that explains the BLAKE3 problem if `BLOBSTORE_WITH_BLAKE3=ON` is used before replacing those files.
- Added Doxygen-style comments to the active blob-store API and hasher interfaces.
- Fixed the unfinished `src/hashing/FuzzyHasher.hpp` so it no longer contains invalid template code.
- Added optional fuzzy-hasher support to `BlobStore` metadata without making fuzzy hashes canonical IDs.
- Added `BlobStore::copyToFile()` so the demo can store and retrieve a copy.

## Requirements

Linux/WSL:

```bash
sudo apt install build-essential cmake gdb libssl-dev
```

macOS with Homebrew:

```bash
brew install cmake openssl
```

Windows:

Use WSL, or install CMake and a C++ compiler. WSL is the simplest route for this prototype.

## Build and run from a terminal

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/blob test_file.pdf out/report_copy.pdf
```

The default demo currently uses the dependency-free FNV-1a 64-bit hasher so the storage pipeline can be tested easily. FNV is not cryptographic. It is only a development/test hasher.

## Optional build switches

```bash
cmake -S . -B build -DBLOBSTORE_WITH_OPENSSL=ON
cmake -S . -B build -DBLOBSTORE_WITH_BLAKE3=OFF
cmake -S . -B build -DBLOBSTORE_WITH_SSDEEP=OFF
```

`BLOBSTORE_WITH_OPENSSL` is on by default and builds the OpenSSL EVP SHA-256 wrapper.

`BLOBSTORE_WITH_BLAKE3` is off by default because the current `src/third_party/blake3` files are HTML pages accidentally saved from GitHub, not raw source files. To enable it, replace these files with their raw versions from the BLAKE3 `c/` directory:

```text
src/third_party/blake3/blake3.h
src/third_party/blake3/blake3_impl.h
src/third_party/blake3/blake3.c
src/third_party/blake3/blake3_dispatch.c
src/third_party/blake3/blake3_portable.c
```

Then configure with:

```bash
cmake -S . -B build -DBLOBSTORE_WITH_BLAKE3=ON
```

`BLOBSTORE_WITH_SSDEEP` is off by default because it requires libfuzzy/ssdeep development headers.

## About VS Code "Problems"

If the terminal build succeeds but VS Code shows many red squiggles, that usually means the editor language server is not using the same include paths or C++ standard as CMake. This project now includes:

- `.vscode/settings.json` for the Microsoft C/C++ extension.
- `.vscode/c_cpp_properties.json` with C++20/C++23-compatible settings and `src/` paths.
- `.clangd` for users who have the clangd extension installed.
- `CMAKE_EXPORT_COMPILE_COMMANDS ON`, so CMake creates `build/compile_commands.json` after the first configure.

After opening the folder, run **C/C++: Reset IntelliSense Database** or **clangd: Restart language server** if old diagnostics remain. Make sure the folder opened in VS Code is the project root containing `CMakeLists.txt`, not the parent folder above it.

## Build and run from VS Code

1. Open this folder in VS Code: `Blob/`.
2. Install the Microsoft C/C++ extension if you want debugging.
3. Press `Ctrl+Shift+B` to build.
4. Open **Run and Debug** and choose **Debug blob demo**.

## Current demo behavior

By default, `main.cpp` stores `test_file.pdf` into `MY_STORE/OBJECTS/<digest chunks>/DATA.BLB`, writes metadata to `META.TXT`, verifies the blob, and writes a retrieved copy to `out/report_copy.pdf`.

You can pass another input/output pair:

```bash
./build/blob path/to/input.bin out/copy.bin
```

## About BLAKE3

The old Code::Blocks project referenced BLAKE3 from `../../../BLAKE3/c`, which means the dependency lived outside this repository. That is why the project broke after moving to VS Code.

To add BLAKE3 back cleanly, vendor the raw source files inside the repo, for example:

```text
src/third_party/blake3/blake3.c
src/third_party/blake3/blake3_dispatch.c
src/third_party/blake3/blake3_portable.c
src/third_party/blake3/blake3.h
src/third_party/blake3/blake3_impl.h
```

Then configure CMake with `-DBLOBSTORE_WITH_BLAKE3=ON` and use `hashing::makeBlake3Factory()` in `main.cpp` or your own application code.
