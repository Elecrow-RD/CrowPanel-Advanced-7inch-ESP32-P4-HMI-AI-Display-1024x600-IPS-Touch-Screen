/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_sd.h"

/* Detected card structure (populated by sd_init). */
static sdmmc_card_t *card;
/* VFS mount point used in file paths, e.g. "/sdcard/huahai.wav". */
const char sd_mount_point[] = SD_MOUNT_POINT;

/*---------------------------------------------------------------
 * File helpers
 *--------------------------------------------------------------*/

/**
 * @brief Create an empty file (truncates if it already exists).
 */
esp_err_t create_file(const char *filename)
{
    SD_INFO("Creating file %s", filename);
    FILE *file = fopen(filename, "wb");
    if (!file) {
        SD_ERROR("Failed to create file");
        return ESP_FAIL;
    }
    fclose(file);
    SD_INFO("File created");
    return ESP_OK;
}

/**
 * @brief Write a NUL-terminated string to a file (text mode).
 */
esp_err_t write_string_file(const char *filename, char *data)
{
    SD_INFO("Opening file %s", filename);
    FILE *file = fopen(filename, "w");
    if (!file) {
        SD_ERROR("Failed to open file for writing string");
        return ESP_FAIL;
    }
    fputs(data, file);
    fclose(file);
    SD_INFO("File written");
    return ESP_OK;
}

/**
 * @brief Read the first line of a file and log it.
 */
esp_err_t read_string_file(const char *filename)
{
    SD_INFO("Reading file %s", filename);
    FILE *file = fopen(filename, "r");
    if (!file) {
        SD_ERROR("Failed to open file for reading string");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), file);
    fclose(file);

    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
        SD_INFO("Read a line from file: '%s'", line);
    } else {
        SD_INFO("Read from file: '%s'", line);
    }
    return ESP_OK;
}

/**
 * @brief Write a fixed number of raw bytes to a file (binary mode).
 */
esp_err_t write_file(const char *filename, char *data, size_t size)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "wb");
    if (!file) {
        SD_ERROR("Failed to open file for writing");
        return ESP_FAIL;
    }
    success_size = fwrite(data, 1, size, file);
    if (success_size != size) {
        fclose(file);
        SD_ERROR("Failed to write file");
        return ESP_FAIL;
    }
    fclose(file);
    SD_INFO("File written");
    return ESP_OK;
}

/**
 * @brief Write bytes at a specific offset, creating the file if needed.
 */
esp_err_t write_file_seek(const char *filename, void *data, size_t size, int32_t seek)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "r+b");
    if (!file) {
        /* File may not exist yet; create then reopen for read+write. */
        file = fopen(filename, "wb");
        if (!file) {
            return ESP_FAIL;
        }
        fclose(file);
        file = fopen(filename, "r+b");
        if (!file) {
            return ESP_FAIL;
        }
    }

    if (fseek(file, seek, SEEK_SET) != 0) {
        fclose(file);
        return ESP_FAIL;
    }
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    return (written == size) ? ESP_OK : ESP_FAIL;
}

/**
 * @brief Read a fixed number of raw bytes from a file.
 */
esp_err_t read_file(const char *filename, char *data, size_t size)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "rb");
    if (!file) {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    success_size = fread(data, 1, size, file);
    if (success_size != size) {
        fclose(file);
        SD_ERROR("Failed to read file");
        return ESP_FAIL;
    }
    fclose(file);
    SD_INFO("File read success");
    return ESP_OK;
}

/**
 * @brief Read a file end to end and report its total size.
 */
esp_err_t read_file_size(const char *read_filename)
{
    size_t read_success_size = 0;
    size_t size = 0;
    FILE *read_file = fopen(read_filename, "rb");
    if (!read_file) {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    uint8_t buffer[1024];
    while ((read_success_size = fread(buffer, 1, sizeof(buffer), read_file)) > 0) {
        size += read_success_size;
    }
    fclose(read_file);
    SD_INFO("File read success,success size =%d", size);
    return ESP_OK;
}

/**
 * @brief Copy one file to another in 1 KB chunks.
 */
esp_err_t read_write_file(const char *read_filename, char *write_filename)
{
    size_t read_success_size = 0;
    size_t write_success_size = 0;
    size_t size = 0;
    FILE *read_file = fopen(read_filename, "rb");
    FILE *write_file = fopen(write_filename, "wb");
    if (!read_file) {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    if (!write_file) {
        SD_ERROR("Failed to open file for writing");
        return ESP_FAIL;
    }
    uint8_t buffer[1024];
    while ((read_success_size = fread(buffer, 1, sizeof(buffer), read_file)) > 0) {
        write_success_size = fwrite(buffer, 1, read_success_size, write_file);
        if (write_success_size != read_success_size) {
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

/*---------------------------------------------------------------
 * SD card mount / info / format
 *--------------------------------------------------------------*/

/**
 * @brief Mount the SD card as a FAT filesystem under /sdcard.
 *
 * Uses the SDMMC host driver on slot 0 with 1-bit data. The card is
 * initialised at a conservative 10 MHz clock and internal pull-ups
 * are enabled.
 *
 * @return ESP_OK on success.
 */
esp_err_t sd_init(void)
{
    esp_err_t err = ESP_OK;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = 10000;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = GPIO_NUM_43;
    slot_config.cmd = GPIO_NUM_44;
    slot_config.d0 = GPIO_NUM_39;
    slot_config.width = 1;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    SD_INFO("Mounting filesystem");
    err = esp_vfs_fat_sdmmc_mount(sd_mount_point, &host, &slot_config,
                                  &mount_config, &card);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(SD_TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the "
                     "EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(SD_TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(err));
        }
        return err;
    }
    SD_INFO("Filesystem mounted");
    sdmmc_card_print_info(stdout, card);
    return err;
}

/**
 * @brief Print a human-readable summary of the detected card.
 *
 * Reports name, type (SDSC/SDHC/SDXC/MMC/SDIO), speed, capacity
 * and, where applicable, the CSD/SCR register fields.
 */
void get_sd_card_info(void)
{
    bool print_scr = false;
    bool print_csd = false;
    const char *type;

    SD_INFO("Name: %s\n", card->cid.name);

    /* Classify the card and decide which registers to print. */
    if (card->is_sdio) {
        type = "SDIO";
        print_scr = true;
        print_csd = true;
    } else if (card->is_mmc) {
        type = "MMC";
        print_csd = true;
    } else {
        if ((card->ocr & SD_OCR_SDHC_CAP) == 0) {
            type = "SDSC";
        } else {
            if (card->ocr & SD_OCR_S18_RA) {
                type = "SDHC/SDXC (UHS-I)";
            } else {
                type = "SDHC";
            }
        }
        print_csd = true;
    }
    SD_INFO("Type: %s\n", type);

    /* Current and maximum operating frequency. */
    if (card->real_freq_khz == 0) {
        SD_INFO("Speed: N/A\n");
    } else {
        const char *freq_unit = card->real_freq_khz < 1000 ? "kHz" : "MHz";
        const float freq = card->real_freq_khz < 1000 ? card->real_freq_khz : card->real_freq_khz / 1000.0;
        const char *max_freq_unit = card->max_freq_khz < 1000 ? "kHz" : "MHz";
        const float max_freq = card->max_freq_khz < 1000 ? card->max_freq_khz : card->max_freq_khz / 1000.0;
        SD_INFO("Speed: %.2f %s (limit: %.2f %s)%s\n",
                freq, freq_unit,
                max_freq, max_freq_unit,
                card->is_ddr ? ", DDR" : "");
    }

    /* Total capacity in megabytes. */
    SD_INFO("Size: %lluMB\n", ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));

    if (print_csd) {
        SD_INFO("CSD: ver=%d, sector_size=%d, capacity=%d read_bl_len=%d\n",
                (int)(card->is_mmc ? card->csd.csd_ver : card->csd.csd_ver + 1),
                card->csd.sector_size,
                card->csd.capacity,
                card->csd.read_block_len);
        if (card->is_mmc) {
            SD_INFO("EXT CSD: bus_width=%" PRIu32 "\n",
                    (uint32_t)(1 << card->log_bus_width));
        } else if (!card->is_sdio) {
            SD_INFO("SSR: bus_width=%" PRIu32 "\n",
                    (uint32_t)(card->ssr.cur_bus_width ? 4 : 1));
        }
    }

    if (print_scr) {
        SD_INFO("SCR: sd_spec=%d, bus_width=%d\n",
                card->scr.sd_spec,
                card->scr.bus_width);
    }
}

/**
 * @brief Format the mounted FAT filesystem.
 */
esp_err_t format_sd_card(void)
{
    esp_err_t err = ESP_OK;
    err = esp_vfs_fat_sdcard_format(sd_mount_point, card);
    if (err != ESP_OK) {
        SD_ERROR("Failed to format FATFS (%s)", esp_err_to_name(err));
        return err;
    }
    return err;
}
