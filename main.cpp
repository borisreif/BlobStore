#include "BlobStore.h"
#include <iostream>

int main() {
    blobdb::BlobStore store("data_root"); // will create data_root/blobs

    // std::cout << "Current working directory: " << fs::current_path() << "\n";

    // Add a file
    std::string id = store.putFile("/home/boris-a-reif/test_file.pdf");
    std::cout << "Blob id: " << id << "\n";

    // Check / verify
    std::cout << "Exists? " << store.exists(id) << "\n";
    std::cout << "Size: " << store.size(id) << " bytes\n";
    std::cout << "Integrity OK? " << store.verify(id) << "\n";

    // Retrieve
    store.getToFile(id, "out/report_copy.pdf");

    return 0;
}
