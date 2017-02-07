//
// Created by root on 2/1/17.
//

#include <stdlib.h>
#include "iothub_client.h"
#include "FirmwareUpdateHandler.h"
#include "json.hpp"


FirmwareUpdateHandler::FirmwareUpdateHandler(utility::string_t stoconnstr, utility::string_t containername, std::string storageaccname) : containerName{containername}, storageAccountName{storageaccname}
{
    InitBlobStorage(stoconnstr);
    memset((void*)stoconnstr.data(), 0, stoconnstr.size());
}

void FirmwareUpdateHandler::HandleFirmwareUpdate(std::string blobUrl, std::string fileName, std::string publicKeyUrl) {
    // Download firmware update file.
    this->DownloadFirmwareUpdate(blobUrl, fileName);

    // Verify firmware update
    std::cout << "-----------------------------------------------\n";
    std::cout << "------------ Verify firmware update -----------\n";
    std::cout << "-----------------------------------------------\n";
    int r = system("/home/debian/ism-device-scripts/verify.sh");
    // If it doesn't return with success
    if (r != 0) {
        return this->HandleFirmwareUpdateError("Error during verification of firmware update.");
    }

    // Extract firmware update
    std::cout << "-----------------------------------------------\n";
    std::cout << "----------- Extract firmware update -----------\n";
    std::cout << "-----------------------------------------------\n";
    r = system("/home/debian/ism-device-scripts/xfwupdate.sh");
    // If it doesn't return with success
    if (r != 0) {
        return this->HandleFirmwareUpdateError("Error during firmware update extraction.");
    }

    // Apply firmware update
    std::cout << "-----------------------------------------------\n";
    std::cout << "------------ Apply firmware update ------------\n";
    std::cout << "-----------------------------------------------\n";
    system("setsid /home/debian/fwupdate/data/apply.sh >/home/debian/fwupdate/logfile 2>&1 &");
    // Shut down this program
    exit(0);
}

void FirmwareUpdateHandler::DownloadFirmwareUpdate(std::string blobUrl, std::string fileName) {
    // Retrieve reference to blob
    std::cout << "Downloading " << fileName << " from " << blobUrl << std::endl;
    azure::storage::cloud_block_blob blockBlob = container.get_block_blob_reference(U(fileName));
    blockBlob.download_to_file("/home/debian/fwupdate.tar.gz");
    std::cout << "Done!\n";
}

void FirmwareUpdateHandler::HandleFirmwareUpdateError(std::string error) {
    std::cout << "Error during firmware update: " << error << std::endl;
    std::cout << "---------- Verification unsuccessful ----------\n";
}

void FirmwareUpdateHandler::InitBlobStorage(utility::string_t storageConnectionString)
{
    try
    {
        // Retrieve storage account from connection string.
        azure::storage::cloud_storage_account storage_account = azure::storage::cloud_storage_account::parse(storageConnectionString);

        // Create the blob client
        azure::storage::cloud_blob_client blob_client = storage_account.create_cloud_blob_client();

        // Retrieve a reference to a container.
        this->container = blob_client.get_container_reference(containerName);

        // Create the container if it doesn't already exist.
        container.create_if_not_exists();
    }
    catch(const std::exception& e)
    {
        std::wcout << U("Error: ") << e.what() << std::endl;
    }
}