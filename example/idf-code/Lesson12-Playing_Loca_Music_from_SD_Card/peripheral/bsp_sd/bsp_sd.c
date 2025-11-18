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
    FILE *file = fopen(filename, "wb"); /*Open file in binary write mode*/
    if (!file)
    {
        SD_ERROR("Failed to create file");
        return ESP_FAIL;
    }
    fclose(file); /*close file*/
    SD_INFO("File created");
    return ESP_OK;
}

esp_err_t write_string_file(const char *filename, char *data)
{
    SD_INFO("Opening file %s", filename);
    FILE *file = fopen(filename, "w"); /*Open file in write mode*/
    if (!file)
    {
        SD_ERROR("Failed to open file for writing string");
        return ESP_FAIL;
    }
    fputs(data, file); /*Write data to the file stream(str)*/
    fclose(file);      /*close file*/
    SD_INFO("File written");
    return ESP_OK;
}

esp_err_t read_string_file(const char *filename)
{
    SD_INFO("Reading file %s", filename);
    FILE *file = fopen(filename, "r"); /*Open file in read mode*/
    if (!file)
    {
        SD_ERROR("Failed to open file for reading string");

        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), file); /*Read a line of text from the file stream*/
    fclose(file);                    /*close file*/

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
    FILE *file = fopen(filename, "wb"); /*Open file in binary write mode*/
    if (!file)
    {
        SD_ERROR("Failed to open file for writing");
        return ESP_FAIL;
    }
    success_size = fwrite(data, 1, size, file); /*Write data to the file at the seeked position*/
    if (success_size != size)
    {
        fclose(file); /*close file*/
        SD_ERROR("Failed to write file");
        return ESP_FAIL;
    }
    else
    {
        fclose(file); /*close file*/
        SD_INFO("File written");
    }
    return ESP_OK;
}

esp_err_t write_file_seek(const char *filename, void *data, size_t size, int32_t seek)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "r+b"); /*Open file in binary read and write mode*/
    if (!file)
    {
        file = fopen(filename, "wb"); /*Open file in binary write mode*/
        if (!file)
        {
            return ESP_FAIL;
        }
        fclose(file);
        file = fopen(filename, "r+b"); /*Open file in binary read and write mode again*/
        if (!file)
        {
            return ESP_FAIL;
        }
    }

    if (fseek(file, seek, SEEK_SET) != 0) /*Seek to the specified position from the beginning of the file*/
    {
        fclose(file);
        return ESP_FAIL;
    }
    size_t written = fwrite(data, 1, size, file); /*Write data to the file at the seeked position*/
    fclose(file);
    return (written == size) ? ESP_OK : ESP_FAIL;
}

esp_err_t read_file(const char *filename, char *data, size_t size)
{
    size_t success_size = 0;
    FILE *file = fopen(filename, "rb"); /*read binary mode*/
    if (!file)
    {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    success_size = fread(data, 1, size, file); /*Read data from the file*/
    if (success_size != size)
    {
        fclose(file); /*close file*/
        SD_ERROR("Failed to read file");
        return ESP_FAIL;
    }
    else
    {
        fclose(file); /*close file*/
        SD_INFO("File read success");
    }
    return ESP_OK;
}

esp_err_t read_file_size(const char *read_filename)
{
    size_t read_success_size = 0;
    size_t size = 0;
    FILE *read_file = fopen(read_filename, "rb"); /*read binary mode*/
    if (!read_file)
    {
        SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    uint8_t buffer[1024];
    while ((read_success_size = fread(buffer, 1, sizeof(buffer), read_file)) > 0) /*Read data from the file*/
    {
        size += read_success_size;
    }
    fclose(read_file); /*close file*/
    SD_INFO("File read success,success size =%d", size);
    return ESP_OK;
}

esp_err_t read_write_file(const char *read_filename, char *write_filename)
{
    size_t read_success_size = 0;
    size_t write_success_size = 0;
    size_t size = 0;
    FILE *read_file = fopen(read_filename, "rb");   /*read binary mode*/
    FILE *write_file = fopen(write_filename, "wb"); /*Open file in binary write mode*/
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
    while ((read_success_size = fread(buffer, 1, sizeof(buffer), read_file)) > 0) /*Read data from the file*/
    {
        write_success_size = fwrite(buffer, 1, read_success_size, write_file); /*Write data to the file at the seeked position*/
        if (write_success_size != read_success_size)
        {
            SD_ERROR("inconsistent reading and writing of data");
            return ESP_FAIL;
        }
        size += write_success_size;
    }
    fclose(read_file);  /*close file*/
    fclose(write_file); /*close file*/
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
    bool print_scr = false;                /*Flag to control SCR register printing*/
    bool print_csd = false;                /*Flag to control CSD register printing*/
    const char *type;                      /*String describing card type*/
    SD_INFO("Name: %s\n", card->cid.name); /*Display card name from Card Identification register*/
    if (card->is_sdio)                     /*Determine card type and set appropriate print flags*/
    {
        type = "SDIO";    /*SD Input/Output card*/
        print_scr = true; /*SDIO cards have SCR register*/
        print_csd = true; /*SDIO cards have CSD register*/
    }
    else if (card->is_mmc)
    {
        type = "MMC";     /*MultiMediaCard*/
        print_csd = true; /*MMC cards have CSD register*/
    }
    else
    {
        if ((card->ocr & SD_OCR_SDHC_CAP) == 0) /*Standard SD card type detection*/
            type = "SDSC";                      /*Standard Capacity SD card (up to 2GB)*/
        else
        {
            if (card->ocr & SD_OCR_S18_RA)
                type = "SDHC/SDXC (UHS-I)"; /*High/Extended Capacity with UHS-I support*/
            else
                type = "SDHC"; /*High Capacity SD card (4GB-32GB)*/
        }
        print_csd = true; /*All SD cards have CSD register*/
    }
    SD_INFO("Type: %s\n", type);
    if (card->real_freq_khz == 0) /*Display current and maximum operating frequencies*/
        SD_INFO("Speed: N/A\n");  /*Frequency information not available*/
    else
    {
        /*Convert frequency to appropriate units (kHz or MHz)*/
        const char *freq_unit = card->real_freq_khz < 1000 ? "kHz" : "MHz";
        const float freq = card->real_freq_khz < 1000 ? card->real_freq_khz : card->real_freq_khz / 1000.0;
        const char *max_freq_unit = card->max_freq_khz < 1000 ? "kHz" : "MHz";
        const float max_freq = card->max_freq_khz < 1000 ? card->max_freq_khz : card->max_freq_khz / 1000.0;
        /*Display current speed, maximum capability, and DDR mode if applicable*/
        SD_INFO("Speed: %.2f %s (limit: %.2f %s)%s\n",
                freq, freq_unit,
                max_freq, max_freq_unit,
                card->is_ddr ? ", DDR" : ""); /*Double Data Rate indicator*/
    }
    /*Calculate and display total card capacity in Megabytes*/
    SD_INFO("Size: %lluMB\n", ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
    /*Display Card Specific Data register information if enabled*/
    if (print_csd)
    {
        SD_INFO("CSD: ver=%d, sector_size=%d, capacity=%d read_bl_len=%d\n",
                (int)(card->is_mmc ? card->csd.csd_ver : card->csd.csd_ver + 1),
                card->csd.sector_size,     /*Size of each sector in bytes*/
                card->csd.capacity,        /*Total number of sectors*/
                card->csd.read_block_len); /*Maximum read block length*/
        /*Display extended information based on card type*/
        if (card->is_mmc)
        {
            SD_INFO("EXT CSD: bus_width=%" PRIu32 "\n",
                    (uint32_t)(1 << card->log_bus_width)); /*MMC bus width (1,4,8 bits)*/
        }
        else if (!card->is_sdio)
        {
            /*Display SD Card Status Register bus width information*/
            SD_INFO("SSR: bus_width=%" PRIu32 "\n",
                    (uint32_t)(card->ssr.cur_bus_width ? 4 : 1)); /*4-bit or 1-bit mode*/
        }
    }
    /*Display SD Configuration Register information if enabled*/
    if (print_scr)
    {
        SD_INFO("SCR: sd_spec=%d, bus_width=%d\n",
                card->scr.sd_spec,    /*SD physical layer specification version*/
                card->scr.bus_width); /*Supported bus widths*/
    }
}

esp_err_t format_sd_card()
{
    esp_err_t err = ESP_OK;
    err = esp_vfs_fat_sdcard_format(sd_mount_point, card); /*Format FAT filesystem*/
    if (err != ESP_OK)
    {
        SD_ERROR("Failed to format FATFS (%s)", esp_err_to_name(err));
        return err;
    }
    return err;
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/