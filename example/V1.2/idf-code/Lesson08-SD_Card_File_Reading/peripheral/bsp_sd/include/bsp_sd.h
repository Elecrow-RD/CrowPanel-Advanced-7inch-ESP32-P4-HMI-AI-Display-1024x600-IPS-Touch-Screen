#ifndef _BSP_SD_H_   // Prevent multiple inclusions of this header file
#define _BSP_SD_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>           // Include standard string manipulation functions
#include <sys/unistd.h>       // Include system calls for file handling
#include <sys/stat.h>         // Include functions for file status and permissions
#include "esp_vfs_fat.h"      // Include ESP-IDF FAT filesystem support for SD card
#include "sdmmc_cmd.h"        // Include SDMMC card command definitions and helpers
#include "driver/sdmmc_host.h"// Include SDMMC host driver for SD card communication
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define SD_TAG "SD_CARD"   // Tag used for logging messages related to SD card operations

#define SD_INFO(fmt, ...) ESP_LOGI(SD_TAG, fmt, ##__VA_ARGS__)   // Macro for info-level SD log output
#define SD_DEBUG(fmt, ...) ESP_LOGD(SD_TAG, fmt, ##__VA_ARGS__)  // Macro for debug-level SD log output
#define SD_ERROR(fmt, ...) ESP_LOGE(SD_TAG, fmt, ##__VA_ARGS__)  // Macro for error-level SD log output

#define EXAMPLE_MAX_CHAR_SIZE 64   // Maximum character buffer size for file read/write operations
#define SD_MOUNT_POINT "/sdcard"   // Default SD card mount point path

esp_err_t create_file(const char *filename);                     // Function to create a new file on SD card
esp_err_t write_string_file(const char *filename, char *data);   // Function to write a string to a file
esp_err_t read_string_file(const char *filename);                // Function to read a string from a file
esp_err_t write_file(const char *filename, char *data, size_t size);   // Function to write raw data to a file
esp_err_t write_file_seek(const char *filename, void *data, size_t size, int32_t seek);  // Function to write data to a specific file offset
esp_err_t read_file(const char *filename, char *data, size_t size);    // Function to read raw data from a file
esp_err_t read_file_size(const char *filename);                  // Function to read file and return its size
void get_sd_card_info(void);                                     // Function to print SD card information
esp_err_t format_sd_card();                                      // Function to format SD card (FAT filesystem)
esp_err_t sd_init();                                             // Function to initialize and mount SD card
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif   // End of include guard
