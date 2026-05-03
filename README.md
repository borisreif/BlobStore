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

## Requirements

Linux/WSL:

```bash
sudo apt install build-essential cmake gdb
```

macOS with Homebrew:

```bash
brew install cmake
```

Windows:

Use WSL, or install CMake and a C++ compiler. WSL is the simplest route for this prototype.

## Build and run from a terminal

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/blob test_file.pdf out/report_copy.pdf
```

## About VS Code "Problems"

If the terminal build succeeds but VS Code shows many red squiggles, that usually means the editor language server is not using the same include paths or C++ standard as CMake. This project now includes:

- `.vscode/settings.json` for the Microsoft C/C++ extension.
- `.vscode/c_cpp_properties.json` with C++17 and `include/` paths.
- `.clangd` for users who have the clangd extension installed.
- `CMAKE_EXPORT_COMPILE_COMMANDS ON`, so CMake creates `build/compile_commands.json` after the first configure.

After opening the folder, run **C/C++: Reset IntelliSense Database** or **clangd: Restart language server** if old diagnostics remain. Make sure the folder opened in VS Code is the project root containing `CMakeLists.txt`, not the parent folder above it.

## Build and run from VS Code

1. Open this folder in VS Code: `Blob_fixed/`.
2. Install the Microsoft C/C++ extension if you want debugging.
3. Press `Ctrl+Shift+B` to build.
4. Open **Run and Debug** and choose **Debug blob demo**.

## Current demo behavior

By default, `main.cpp` stores `test_file.pdf` into `data_root/blobs/<first-two-hex-chars>/<hash>` and writes a retrieved copy to `out/report_copy.pdf`.

You can pass another input/output pair:

```bash
./build/blob path/to/input.bin out/copy.bin
```

## About BLAKE3

The old Code::Blocks project referenced BLAKE3 from `../../../BLAKE3/c`, which means the dependency lived outside this repository. That is why the project broke after moving to VS Code.

To add BLAKE3 back cleanly, vendor it inside the repo, for example:

```text
third_party/blake3/c/blake3.c
third_party/blake3/c/blake3_dispatch.c
third_party/blake3/c/blake3_portable.c
third_party/blake3/c/blake3.h
```

Then add those C files to `CMakeLists.txt` and re-enable `Blake3Hasher` in `HashFactory.cpp`.
