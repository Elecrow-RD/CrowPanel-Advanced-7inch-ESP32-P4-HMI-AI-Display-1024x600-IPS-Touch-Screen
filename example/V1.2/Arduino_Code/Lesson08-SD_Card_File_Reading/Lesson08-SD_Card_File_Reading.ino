/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"   // board pin define
#include <Arduino.h>        // Arduino core library. Must be placed at the very top to ensure recognition of Arduino APIs

#include <string.h>         // Include standard string manipulation functions
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes
#include <sys/unistd.h>         // Include system calls for file handling
#include <sys/stat.h>           // Include functions for file status and permissions
#include <esp_vfs_fat.h>        // Include ESP-IDF FAT filesystem support for SD card
#include <sdmmc_cmd.h>          // Include SDMMC card command definitions and helpers
#include <driver/sdmmc_host.h>  // Include SDMMC host driver for SD card communication
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define PRINTF_ORIGINAL(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__);
#define PRINTF_PRINT(fmt, ...)    Serial.print(fmt);
#define PRINTF_LN(fmt, ...)       Serial.println(fmt);

#define PRINTF_ERROR(fmt, ...)      do { \
                                        Serial.print("[ERROR] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_WARN(fmt, ...)       do { \
                                        Serial.print("[WARN] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_INFO(fmt, ...)       do { \
                                        Serial.print("[INFO] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_DEBUG(fmt, ...)      do { \
                                        Serial.print("[DEBUG] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)

#define MAIN_INFO(fmt, ...)         PRINTF_INFO(fmt, ##__VA_ARGS__)   // Info level log macro
#define MAIN_ERROR(fmt, ...)        PRINTF_ERROR(fmt, ##__VA_ARGS__)  // Error level log macro

#define SD_INFO(fmt, ...)           PRINTF_INFO(fmt, ##__VA_ARGS__) 
#define SD_ERROR(fmt, ...)          PRINTF_ERROR(fmt, ##__VA_ARGS__)

#define EXAMPLE_MAX_CHAR_SIZE 64   // Maximum character buffer size for file read/write operations
#define SD_MOUNT_POINT "/sdcard"   // Default SD card mount point path
/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
static sdmmc_card_t *card;
const char sd_mount_point[] = SD_MOUNT_POINT;

TaskHandle_t sd_task_handle;   // Task handle for the SD card test task
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

esp_err_t create_file(const char *filename)
{
    SD_INFO("Creating file %s", filename);
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        SD_ERROR("Failed to create file");
        return ESP_FAIL;
    }
    fclose(file);
    SD_INFO("File created");
    return ESP_OK;
}

esp_err_t write_string_file(const char *filename, char *data)
{
    SD_INFO("Opening file %s", filename);
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        SD_ERROR("Failed to open file for writing string");
        return ESP_FAIL;
    }
    fprintf(file, "%s", data);
    fclose(file);
    SD_INFO("File written");
    return ESP_OK;
}

esp_err_t read_string_file(const char *filename)
{
    SD_INFO("Reading file %s", filename);
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        SD_ERROR("Failed to open file for reading string");

        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), file);
    fclose(file);

    char *pos = strchr(line, '\n');
    if (pos)
    {
        *pos = '\0';
        SD_INFO("Read a line from file: '%s'", line);
    }
    else
        SD_INFO("Read from file: '%s'", line);
    return ESP_OK;
}

esp_err_t write_file(const char *filename, char *data, size_t size)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        SD_ERROR("Failed to open file for writing");
        return ESP_FAIL;
    }
    success_size = fwrite(data, 1, size, file);
    if (success_size != size)
    {
        fclose(file);
        SD_ERROR("Failed to write file");
        return ESP_FAIL;
    }
    else
    {
        fclose(file);
        SD_INFO("File written");
    }
    return ESP_OK;
}

esp_err_t write_file_seek(const char *filename, void *data, size_t size, int32_t seek)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        SD_ERROR("Failed to open file for writing");
        return ESP_FAIL;
    }
    if (fseek(file, seek, SEEK_SET) != 0)
    {
        SD_ERROR("Failed to seek file");
        return ESP_FAIL;
    }
    success_size = fwrite(data, 1, size, file);
    if (success_size != size)
    {
        fclose(file);
        SD_ERROR("Failed to write file");
        return ESP_FAIL;
    }
    else
    {
        fclose(file);
        SD_INFO("File written");
    }
    return ESP_OK;
}

esp_err_t read_file(const char *filename, char *data, size_t size)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    success_size = fread(data, 1, size, file);
    if (success_size != size)
    {
        fclose(file);
        SD_ERROR("Failed to read file");
        return ESP_FAIL;
    }
    else
    {
        fclose(file);
        SD_INFO("File read success");
    }
    return ESP_OK;
}

esp_err_t read_file_size(const char *read_filename)
{
    size_t read_success_size = 0;
    size_t size = 0;
    FILE *read_file = fopen(read_filename, "rb");
    if (!read_file)
    {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    uint8_t buffer[1024];
    while ((read_success_size = fread(buffer, 1, sizeof(buffer), read_file)) > 0)
    {
        size += read_success_size;
    }
    fclose(read_file);
    SD_INFO("File read success,success size =%d", size);
    return ESP_OK;
}

esp_err_t read_write_file(const char *read_filename, char *write_filename)
{
    size_t read_success_size = 0;
    size_t write_success_size = 0;
    size_t size = 0;
    FILE *read_file = fopen(read_filename, "rb");
    FILE *write_file = fopen(write_filename, "wb");
    if (!read_file)
    {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    if (!write_file)
    {
        SD_ERROR("Failed to open file for writing");
        return ESP_FAIL;
    }
    uint8_t buffer[1024];
    while ((read_success_size = fread(buffer, 1, sizeof(buffer), read_file)) > 0)
    {
        write_success_size = fwrite(buffer, 1, read_success_size, write_file);
        if (write_success_size != read_success_size)
        {
            SD_ERROR("inconsistent reading and writing of data");
            return ESP_FAIL;
        }
        size += write_success_size;
    }
    fclose(read_file);
    fclose(write_file);
    SD_INFO("File read and write success,success size =%d", size);
    return ESP_OK;
}

esp_err_t sd_init()
{
    esp_err_t err = ESP_OK;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        // if SD card file system format is not fat32, mount failed unless ".format_if_mount_failed = true".
        .format_if_mount_failed = false,    
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host =  SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = 10000;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = (gpio_num_t)SD_GPIO_MMC_CLK;
    slot_config.cmd = (gpio_num_t)SD_GPIO_MMC_CMD;
    slot_config.d0 = (gpio_num_t)SD_GPIO_MMC_D0;
    slot_config.width = 1; // Single-line SDIO
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    SD_INFO("Mounting filesystem");
    err = esp_vfs_fat_sdmmc_mount(sd_mount_point, &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            MAIN_INFO("Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            MAIN_INFO("Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(err));
        }
        return err;
    }
    SD_INFO("Filesystem mounted");
    sdmmc_card_print_info(stdout, card);
    return err;
}

void get_sd_card_info()
{
    sdmmc_card_print_info(stdout, card);
}

esp_err_t format_sd_card()
{
    esp_err_t err = ESP_OK;
    err = esp_vfs_fat_sdcard_format(sd_mount_point, card);
    if (err != ESP_OK)
    {
        SD_ERROR("Failed to format FATFS (%s)", esp_err_to_name(err));
        return err;
    }
    return err;
}

void sd_task(void *param)   // SD card test task function
{
    esp_err_t err = ESP_OK;   // Variable to store function return values (error codes)

    const char *file_hello = SD_MOUNT_POINT "/hello.txt";   // File path for SD card test file
    char *data = "hello world!";   // Data to be written into the file
    
    // Get SD card information
    get_sd_card_info();   // Print SD card info such as size, type, and speed
    
    while (1)   // Infinite loop to perform read/write test
    {
        // Write data to file
        err = write_string_file(file_hello, data);   // Write the "hello world!" string to the file
        if (err != ESP_OK)   // Check if writing failed
        {
            MAIN_ERROR("Write file failed");   // Print error message if writing fails
            continue;   // Continue to next iteration of loop
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);   // Delay 200ms to allow SD card to complete internal operations

        // Read data from file
        err = read_string_file(file_hello);   // Read the content from the written file
        if (err != ESP_OK)   // Check if reading failed
        {
            MAIN_ERROR("Read file failed");   // Print error message if reading fails
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);   // Delay 1 second before repeating the test
        MAIN_INFO("SD card test completed");   // Log message indicating test finished successfully
        vTaskDelete(NULL);   // Delete this task after finishing the test
    }
}

void Init(void)   // System initialization function
{
    esp_err_t err = ESP_OK;   // Variable to store error codes
    
    // Initialize SD card
    err = sd_init();   // Call SD card initialization function
    // Check if initialization failed
    while (ESP_OK != err) { 
        MAIN_ERROR("%s initialization failed [ %s ]", "SD card", esp_err_to_name(err));   // Print module name and error description
        delay (1000);
        err = sd_init();   // Call SD card initialization function
    }
}

void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

    MAIN_INFO("----------SD card test program start----------\r\n");   // Print program start message
    
    // Initialize system
    Init();   // Call initialization function to set up SD card and other components
    
    MAIN_INFO("----------SD card test begin----------\r\n");   // Print message indicating test task has started
    // Create SD card test task
    xTaskCreatePinnedToCore(sd_task, "sd_task", 4096, NULL, 5, &sd_task_handle, 1);   // Create a FreeRTOS task to test SD card (core 1)
}

void loop() {
    // put your main code here, to run repeatedly:
  
    delay(1000);
}
