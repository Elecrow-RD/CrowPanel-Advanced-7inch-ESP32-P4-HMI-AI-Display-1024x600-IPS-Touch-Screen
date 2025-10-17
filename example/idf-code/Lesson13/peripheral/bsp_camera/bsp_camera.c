/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_camera.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
// Add the missing macro definitions.
#ifndef BITS_PER_PIXEL
#define BITS_PER_PIXEL 16
#endif

static i2c_master_bus_handle_t sccb_bus_handle = NULL;
static esp_sccb_io_handle_t sccb_io_handle = NULL;
static esp_cam_sensor_device_t *cam = NULL;
static isp_proc_handle_t isp_proc = NULL;
static isp_ae_ctlr_t ae_ctlr = NULL;
static isp_awb_ctlr_t awb_ctlr = NULL;
esp_cam_ctlr_trans_t my_trans;
esp_cam_ctlr_handle_t cam_handle = NULL;
void *camera_buffer = NULL;

static lv_obj_t *camera_obj;
lv_img_dsc_t img_camera;

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

bool example_isp_awb_on_statistics_done_cb(isp_awb_ctlr_t awb_ctlr, const esp_isp_awb_evt_data_t *edata, void *user_data)
{
    return true;
}

IRAM_ATTR bool camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    esp_cam_ctlr_trans_t new_trans = *(esp_cam_ctlr_trans_t *)user_data;
    trans->buffer = new_trans.buffer;
    trans->buflen = new_trans.buflen;
    return false;
}

IRAM_ATTR bool camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    return false;
}

static esp_err_t camera_sensor_init()
{
    esp_err_t err = ESP_OK;
    int enable_flag = 1;
    uint32_t exposure_us = 4000;
    uint32_t exposure_val = 700;
    esp_cam_sensor_format_t *cam_cur_fmt = NULL;
    i2c_master_bus_config_t sccb_conf = {
        .i2c_port = SCCB_MASTER_PORT,
        .sda_io_num = SCCB_GPIO_SDA,
        .scl_io_num = SCCB_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
    };
    err = i2c_new_master_bus(&sccb_conf, &sccb_bus_handle);
    if (err != ESP_OK)
        return err;
    esp_cam_sensor_config_t cam_config = {
        .sccb_handle = sccb_io_handle,
        .reset_pin = -1,
        .pwdn_pin = -1,
        .xclk_pin = -1,
        .sensor_port = ESP_CAM_SENSOR_MIPI_CSI,
    };
    for (esp_cam_sensor_detect_fn_t *p = &__esp_cam_sensor_detect_fn_array_start; p < &__esp_cam_sensor_detect_fn_array_end; ++p)
    {
        sccb_i2c_config_t i2c_config = {
            .scl_speed_hz = 100000,
            .device_address = p->sccb_addr,
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        };
        ESP_ERROR_CHECK(sccb_new_i2c_io(sccb_bus_handle, &i2c_config, &cam_config.sccb_handle));
        cam = (*(p->detect))(&cam_config);
        if (cam)
        {
            if (p->port != ESP_CAM_SENSOR_MIPI_CSI)
            {
                err = ESP_FAIL;
                CAMERA_ERROR("detect a camera sensor with mismatched interface");
                return err;
            }
            break;
        }
        err = esp_sccb_del_i2c_io(cam_config.sccb_handle);
        if (err != ESP_OK)
            return err;
    }
    if (cam == NULL)
    {
        err = ESP_FAIL;
        CAMERA_ERROR("failed to detect camera sensor");
        return err;
    }
    esp_cam_sensor_format_array_t cam_fmt_array = {0};
    err = esp_cam_sensor_query_format(cam, &cam_fmt_array);
    if (err != ESP_OK)
        return err;
    const esp_cam_sensor_format_t *parray = cam_fmt_array.format_array;

    for (int i = 0; i < cam_fmt_array.count; i++)
    {
        if (!strcmp(parray[i].name, "MIPI_2lane_24Minput_RAW8_1024x600_30fps"))
        {
            cam_cur_fmt = (esp_cam_sensor_format_t *)&(parray[i].name);
        }
    }
    err = esp_cam_sensor_set_format(cam, (const esp_cam_sensor_format_t *)cam_cur_fmt);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("Format set fail");
        return err;
    }

    err = esp_cam_sensor_set_para_value(cam, ESP_CAM_SENSOR_HMIRROR, &enable_flag, 1);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("camera mirror fail");
        return err;
    }
    err = esp_cam_sensor_set_para_value(cam, ESP_CAM_SENSOR_EXPOSURE_US, &exposure_us, 1);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("camera set exposure time fail");
        return err;
    }

    err = esp_cam_sensor_set_para_value(cam, ESP_CAM_SENSOR_EXPOSURE_VAL, &exposure_val, 1);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("camera set exposure val fail");
        return err;
    }
    err = esp_cam_sensor_ioctl(cam, ESP_CAM_SENSOR_IOC_S_STREAM, &enable_flag); // Set sensor output stream
    if (err != ESP_OK)
    {
        CAMERA_ERROR("Start stream fail");
        return err;
    }
    return err;
}

static esp_err_t camera_csi_init()
{
    esp_err_t err = ESP_OK;
    esp_cam_ctlr_csi_config_t csi_config = {
        .ctlr_id = 1,
        .h_res = 1024,
        .v_res = 600,
        .lane_bit_rate_mbps = 200,
        .input_data_color_type = CAM_CTLR_COLOR_RAW8,
        .output_data_color_type = CAM_CTLR_COLOR_RGB565,
        .data_lane_num = 2,
        .byte_swap_en = false,
        .queue_items = 5,
    };
    err = esp_cam_new_csi_ctlr(&csi_config, &cam_handle);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("csi init fail[%d]", err);
        return err;
    }
    esp_cam_ctlr_evt_cbs_t cbs = {
        .on_get_new_trans = camera_get_new_vb,
        .on_trans_finished = camera_get_finished_trans,
    };
    err = esp_cam_ctlr_register_event_callbacks(cam_handle, &cbs, &my_trans);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("ops register fail");
        return err;
    }
    err = esp_cam_ctlr_enable(cam_handle);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable cam controller fail");
        return err;
    }
    return err;
}

static esp_err_t isp_init()
{
    esp_err_t err = ESP_OK;
    esp_isp_processor_cfg_t isp_config = {
        .clk_hz = 80 * 1000 * 1000,
        .input_data_source = ISP_INPUT_DATA_SOURCE_CSI,
        .input_data_color_type = ISP_COLOR_RAW8,
        .output_data_color_type = ISP_COLOR_RGB565,
        .bayer_order = COLOR_RAW_ELEMENT_ORDER_BGGR,
        .has_line_start_packet = false,
        .has_line_end_packet = false,
        .h_res = 1024,
        .v_res = 600,
    };
    err = esp_isp_new_processor(&isp_config, &isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp register fail");
        return err;
    }
    err = esp_isp_enable(isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp fail");
        return err;
    }
    esp_isp_color_config_t color_config = {
        .color_contrast = {
            .integer = 0,
            .decimal = 88,
        },
        .color_saturation = {
            .integer = 1,
            .decimal = 0,
        },
        .color_hue = 0,
        .color_brightness = 40,
    };
    err = esp_isp_color_configure(isp_proc, &color_config);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp color configure fail");
        return err;
    }
    err = esp_isp_color_enable(isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp color fail");
        return err;
    }
    esp_isp_awb_config_t awb_config = {
        .sample_point = ISP_AWB_SAMPLE_POINT_AFTER_CCM,
        .white_patch.luminance.min = 0,
        .white_patch.luminance.max = 255,
        .white_patch.red_green_ratio.min = 0.7,
        .white_patch.red_green_ratio.max = 1.0,
        .white_patch.blue_green_ratio.min = 0.7,
        .white_patch.blue_green_ratio.max = 1.0,
    };
    err = esp_isp_new_awb_controller(isp_proc, &awb_config, &awb_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp awb controller fail");
        return err;
    }
    esp_isp_awb_cbs_t awb_cb = {
        .on_statistics_done = example_isp_awb_on_statistics_done_cb,
    };
    err = esp_isp_awb_register_event_callbacks(awb_ctlr, &awb_cb, NULL);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("register isp awb callback fail");
        return err;
    }
    err = esp_isp_awb_controller_enable(awb_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("enable isp awb fail");
        return err;
    }
    err = esp_isp_awb_controller_start_continuous_statistics(awb_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("start isp awb fail");
        return err;
    }
    esp_isp_ae_config_t ae_config = {
        .sample_point = ISP_AE_SAMPLE_POINT_AFTER_GAMMA,
    };
    err = esp_isp_new_ae_controller(isp_proc, &ae_config, &ae_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ae configure fail");
        return err;
    }
    err = esp_isp_ae_controller_enable(ae_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ae enable fail");
        return err;
    }
    err = esp_isp_ae_controller_start_continuous_statistics(ae_ctlr);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ae start fail");
        return err;
    }
    esp_isp_ccm_config_t ccm_cfg = {
        .matrix = {
            {1.0, 0.0, 0.0},
            {0.0, 0.5, 0.0},
            {0.0, 0.0, 1.0},
        },
        .saturation = false,
    };
    err = esp_isp_ccm_configure(isp_proc, &ccm_cfg);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ccm configure fail");
        return err;
    }
    err = esp_isp_ccm_enable(isp_proc);
    if (err != ESP_OK)
    {
        CAMERA_ERROR("isp ccm enable fail");
        return err;
    }
    return err;
}

esp_err_t camera_init()
{
    esp_err_t err = ESP_OK;
    uint32_t cache_line_size = cache_hal_get_cache_line_size(CACHE_LL_LEVEL_EXT_MEM, CACHE_TYPE_DATA);
    size_t camera_buffer_size = 0;
    camera_buffer_size = 1024 * 600 * ((BITS_PER_PIXEL + 7) / 8);
    camera_buffer = heap_caps_aligned_calloc(cache_line_size, 1, camera_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (camera_buffer == NULL)
        return ESP_FAIL;
    my_trans.buffer = camera_buffer;
    my_trans.buflen = camera_buffer_size;
    err = camera_sensor_init();
    if (err != ESP_OK)
        return err;
    err = camera_csi_init();
    if (err != ESP_OK)
        return err;
    err = isp_init();
    if (err != ESP_OK)
        return err;
    memset(my_trans.buffer, 0xFF, my_trans.buflen);
    esp_cache_msync((void *)my_trans.buffer, my_trans.buflen, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    err = esp_cam_ctlr_start(cam_handle);
    if (err != ESP_OK)
    {
        CAMERA_ERROR(" Camera Driver start fail");
        return err;
    }
    return err;
}

esp_err_t camera_refresh()
{
    esp_err_t err = ESP_OK;
    err = esp_cam_ctlr_receive(cam_handle, &my_trans, ESP_CAM_CTLR_MAX_DELAY);
    if (err != ESP_OK)
    {
        CAMERA_INFO("Receive failed: %s", esp_err_to_name(err));
        return err;
    }
    return err;
}

void camera_display_refresh()
{
    lv_obj_invalidate(camera_obj);
}

void camera_display()
{
    if (!lvgl_port_lock(0)) {
        return;
    }
    camera_obj = lv_img_create(lv_scr_act());
    lv_obj_align(camera_obj, LV_ALIGN_CENTER, 0, 0);
    img_camera = (lv_img_dsc_t){
        .header = {.always_zero = 0, .w = 1024, .h = 600, .cf = LV_IMG_CF_TRUE_COLOR},
        .data = my_trans.buffer,
        .data_size = 1024 * 600 * 2,
    };
    lv_img_set_src(camera_obj, &img_camera);
    lvgl_port_unlock();
}
