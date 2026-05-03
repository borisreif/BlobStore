#include "include/BlobStore.h"

#include <filesystem>
#include <iostream>
#include <exception>
#include <string>

int main(int argc, char* argv[]) {
    try {
        namespace fs = std::filesystem;

        fs::path inputPath{"test_file.pdf"};
        fs::path outputPath{"out/report_copy.pdf"};
        if (argc > 1) inputPath = fs::path{std::string{argv[1]}};
        if (argc > 2) outputPath = fs::path{std::string{argv[2]}};

        blobdb::BlobStore store{fs::path{"data_root"}};

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
