#include "BlobStore.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    try {
        namespace fs = std::filesystem;

        const fs::path inputPath = argc > 1 ? fs::path(argv[1]) : fs::path("test_file.pdf");
        const fs::path outputPath = argc > 2 ? fs::path(argv[2]) : fs::path("out/report_copy.pdf");

        blobdb::BlobStore store("data_root");

        std::cout << "Current working directory: " << fs::current_path() << '\n';
        std::cout << "Input file: " << inputPath << '\n';

        const std::string id = store.putFile(inputPath);
        std::cout << "Blob id: " << id << '\n';
        std::cout << "Stored at: " << store.pathOf(id) << '\n';
        std::cout << "Exists? " << std::boolalpha << store.exists(id) << '\n';
        std::cout << "Size: " << store.size(id) << " bytes\n";
        std::cout << "Integrity OK? " << store.verify(id) << '\n';

        if (store.getToFile(id, outputPath)) {
            std::cout << "Copied blob to: " << outputPath << '\n';
        } else {
            std::cout << "Failed to copy blob to: " << outputPath << '\n';
            return 2;
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
