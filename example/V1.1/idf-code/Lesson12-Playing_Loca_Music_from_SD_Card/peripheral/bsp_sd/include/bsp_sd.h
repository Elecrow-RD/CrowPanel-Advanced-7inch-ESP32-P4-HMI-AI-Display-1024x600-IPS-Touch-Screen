#ifndef _BSP_SD_H_
#define _BSP_SD_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_vfs_fat.h"              //References for vfs fat Function-related API Functions
#include "sdmmc_cmd.h"                //References for sdmmc cmd Function-related API Functions
#include "driver/sdmmc_host.h"        //References for sdmmc host Function-related API Functions
#include "esp_private/sdmmc_common.h" //References for sdmmc common Function-related API Functions
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define SD_TAG "SD_CARD"
#define SD_INFO(fmt, ...) ESP_LOGI(SD_TAG, fmt, ##__VA_ARGS__)
#define SD_DEBUG(fmt, ...) ESP_LOGD(SD_TAG, fmt, ##__VA_ARGS__)
#define SD_ERROR(fmt, ...) ESP_LOGE(SD_TAG, fmt, ##__VA_ARGS__)

#define EXAMPLE_MAX_CHAR_SIZE 64       // Maximum byte count
#define SD_MOUNT_POINT "/sdcard"       // path where partition should be registered


esp_err_t sd_init();                                                                    // SD Initialization Function
esp_err_t create_file(const char *filename);                                            // Create a binary file
esp_err_t write_string_file(const char *filename, char *data);                          // Create and write to a text file
esp_err_t read_string_file(const char *filename);                                       // Read to a text file
esp_err_t write_file(const char *filename, char *data, size_t size);                    // Create and write to a binary file
esp_err_t write_file_seek(const char *filename, void *data, size_t size, int32_t seek); // Create and write to a binary file that can offset the writing position
esp_err_t read_file(const char *filename, char *data, size_t size);                     // Read to a binary file
esp_err_t read_file_size(const char *filename);                                         // Read the number of bytes in a binary file
esp_err_t format_sd_card();                                                             // Format the SD card
void get_sd_card_info();                                                                // Obtain information about the SD card device

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
