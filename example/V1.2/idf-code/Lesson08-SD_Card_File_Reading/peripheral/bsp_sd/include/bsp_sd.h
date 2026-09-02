#ifndef _BSP_SD_H_
#define _BSP_SD_H_

/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define SD_TAG "SD_CARD"

#define SD_INFO(fmt, ...)   ESP_LOGI(SD_TAG, fmt, ##__VA_ARGS__)
#define SD_DEBUG(fmt, ...)  ESP_LOGD(SD_TAG, fmt, ##__VA_ARGS__)
#define SD_ERROR(fmt, ...)  ESP_LOGE(SD_TAG, fmt, ##__VA_ARGS__)

/* Read buffer cap used by read_string_file(). */
#define EXAMPLE_MAX_CHAR_SIZE 64

/* VFS path where the FAT filesystem is mounted. */
#define SD_MOUNT_POINT "/sdcard"

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/
esp_err_t create_file(const char *filename);
esp_err_t write_string_file(const char *filename, char *data);
esp_err_t read_string_file(const char *filename);
esp_err_t write_file(const char *filename, char *data, size_t size);
esp_err_t write_file_seek(const char *filename, void *data, size_t size, int32_t seek);
esp_err_t read_file(const char *filename, char *data, size_t size);
esp_err_t read_file_size(const char *filename);
esp_err_t read_write_file(const char *read_filename, char *write_filename);
void get_sd_card_info(void);
esp_err_t format_sd_card(void);
esp_err_t sd_init(void);

#endif
