/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_sd.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
static sdmmc_card_t *card;
const char sd_mount_point[] = SD_MOUNT_POINT;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
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
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host =  SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = 10000;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = GPIO_NUM_43;
    slot_config.cmd = GPIO_NUM_44;
    slot_config.d0 = GPIO_NUM_39;
    slot_config.width = 1;  // 1线SDIO
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    SD_INFO("Mounting filesystem");
    err = esp_vfs_fat_sdmmc_mount(sd_mount_point, &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(SD_TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(SD_TAG, "Failed to initialize the card (%s). "
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
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/