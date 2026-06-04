/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_camera.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
static i2c_master_bus_handle_t sccb_bus_handle = NULL;
static lv_obj_t *camera_obj;
static camera_video_t camera_video;
uint8_t *cam_buffer[2];
size_t cam_buffer_size[2];
int camera_video_id = 0;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
esp_err_t camera_video_init()
{
    esp_err_t err = ESP_OK;
    i2c_master_bus_config_t sccb_conf = {
        .i2c_port = SCCB_MASTER_PORT,
        .sda_io_num = SCCB_GPIO_SDA,
        .scl_io_num = SCCB_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
    };
    CAMERA_INFO("Initializing SCCB Bus.......");
    err = i2c_new_master_bus(&sccb_conf, &sccb_bus_handle);
    esp_video_init_csi_config_t csi_config = {
        .sccb_config = {
            .init_sccb = true,
            .i2c_config = {
                .port = SCCB_MASTER_PORT,
                .scl_pin = SCCB_GPIO_SCL,
                .sda_pin = SCCB_GPIO_SDA,
            },
            .freq = 100000,
        },
        .reset_pin = -1,
        .pwdn_pin = -1,
    };
    csi_config.sccb_config.init_sccb = false;
    csi_config.sccb_config.i2c_handle = sccb_bus_handle;
    esp_video_init_config_t cam_config_ptr = {
        .csi = &csi_config,
    };
    err = esp_video_init(&cam_config_ptr);
    if (err != ESP_OK)
        return err;
    return err;
}

int video_open()
{
    struct v4l2_format camera_format;
    struct v4l2_capability capability;

#if CONFIG_ENABLE_CAM_SENSOR_PIC_VFLIP || CONFIG_ENABLE_CAM_SENSOR_PIC_HFLIP
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control control[1];
#endif

    int fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY | O_NONBLOCK, 0);
    if (fd < 0)
    {
        CAMERA_ERROR("Open video failed");
        return -1;
    }
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability))
    {
        CAMERA_ERROR("failed to get capability");
        goto exit_0;
    }
    CAMERA_INFO("version: %d.%d.%d", (uint16_t)(capability.version >> 16), (uint8_t)(capability.version >> 8), (uint8_t)capability.version);
    CAMERA_INFO("driver:  %s", capability.driver);
    CAMERA_INFO("card:    %s", capability.card);
    CAMERA_INFO("bus:     %s", capability.bus_info);
    memset(&camera_format, 0, sizeof(struct v4l2_format));
    camera_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &camera_format) != 0)
    {
        CAMERA_ERROR("failed to get format");
        goto exit_0;
    }
    CAMERA_INFO("width=%" PRIu32 " height=%" PRIu32, camera_format.fmt.pix.width, camera_format.fmt.pix.height);
    camera_video.camera_buf_hes = camera_format.fmt.pix.width;
    camera_video.camera_buf_ves = camera_format.fmt.pix.height;
    if (camera_format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565)
    {
        struct v4l2_format format = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .fmt.pix.width = camera_format.fmt.pix.width,
            .fmt.pix.height = camera_format.fmt.pix.height,
            .fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565,
        };
        if (ioctl(fd, VIDIOC_S_FMT, &format) != 0)
        {
            CAMERA_ERROR("failed to set format");
            goto exit_0;
        }
    }
    CAMERA_INFO("app_video_open successful");

#if CONFIG_ENABLE_CAM_SENSOR_PIC_VFLIP
    controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    controls.count = 1;
    controls.controls = control;
    control[0].id = V4L2_CID_VFLIP;
    control[0].value = 1;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0)
    {
        CAMERA_ERROR("failed to mirror the frame horizontally and skip this step");
    }
#endif

#if CONFIG_ENABLE_CAM_SENSOR_PIC_HFLIP
    controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    controls.count = 1;
    controls.controls = control;
    control[0].id = V4L2_CID_HFLIP;
    control[0].value = 1;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0)
    {
        CAMERA_ERROR("failed to mirror the frame horizontally and skip this step");
    }
#endif
    CAMERA_INFO("fd = %d", fd);
    return fd;
exit_0:
    close(fd);
    return -1;
}

esp_err_t camera_video_set_bufs(int video_fd, uint32_t fb_num, const void **fb)
{
    struct v4l2_requestbuffers req;
    if (fb_num > MAX_BUFFER_COUNT)
    {
        CAMERA_ERROR("buffer num is too large");
        return ESP_FAIL;
    }
    else if (fb_num < 2)
    {
        CAMERA_ERROR("At least two buffers are required");
        return ESP_FAIL;
    }
    memset(&req, 0, sizeof(req));
    req.count = fb_num;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_video.camera_mem_mode = req.memory = fb ? V4L2_MEMORY_USERPTR : V4L2_MEMORY_MMAP;
    if (ioctl(video_fd, VIDIOC_REQBUFS, &req) != 0)
    {
        CAMERA_ERROR("req bufs failed");
        goto errout_req_bufs;
    }
    for (int i = 0; i < fb_num; i++)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = req.memory;
        buf.index = i;
        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) != 0)
        {
            CAMERA_ERROR("query buf failed");
            goto errout_req_bufs;
        }
        if (req.memory == V4L2_MEMORY_MMAP)
        {
            camera_video.camera_buffer[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buf.m.offset);
            if (camera_video.camera_buffer[i] == NULL)
            {
                CAMERA_ERROR("mmap failed");
                goto errout_req_bufs;
            }
        }
        else
        {
            if (!fb[i])
            {
                CAMERA_ERROR("frame buffer is NULL");
                goto errout_req_bufs;
            }
            buf.m.userptr = (unsigned long)fb[i];
            camera_video.camera_buffer[i] = (uint8_t *)fb[i];
        }
        camera_video.camera_buf_size = buf.length;
        if (ioctl(video_fd, VIDIOC_QBUF, &buf) != 0)
        {
            CAMERA_ERROR("queue frame buffer failed");
            goto errout_req_bufs;
        }
    }
    return ESP_OK;
errout_req_bufs:
    close(video_fd);
    return ESP_FAIL;
}

esp_err_t camera_video_get_bufs(int fb_num, void **fb)
{
    if (fb_num > MAX_BUFFER_COUNT)
    {
        CAMERA_ERROR("buffer num is too large");
        return ESP_FAIL;
    }
    else if (fb_num < 2)
    {
        CAMERA_ERROR("At least two buffers are required");
        return ESP_FAIL;
    }
    for (int i = 0; i < fb_num; i++)
    {
        if (camera_video.camera_buffer[i] != NULL)
        {
            fb[i] = camera_video.camera_buffer[i];
        }
        else
        {
            CAMERA_ERROR("frame buffer is NULL");
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

uint32_t app_video_get_buf_size(void)
{
    uint32_t buf_size = camera_video.camera_buf_hes * camera_video.camera_buf_ves * 2;
    return buf_size;
}

static inline esp_err_t video_receive_video_frame(int video_fd)
{
    memset(&camera_video.v4l2_buf, 0, sizeof(camera_video.v4l2_buf));
    camera_video.v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_video.v4l2_buf.memory = camera_video.camera_mem_mode;
    int res = ioctl(video_fd, VIDIOC_DQBUF, &(camera_video.v4l2_buf));
    if (res != 0)
    {
        CAMERA_ERROR("failed to receive video frame");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static inline void video_operation_video_frame(int video_fd)
{
    camera_video.v4l2_buf.m.userptr = (unsigned long)camera_video.camera_buffer[camera_video.v4l2_buf.index];
    camera_video.v4l2_buf.length = camera_video.camera_buf_size;
    uint8_t buf_index = camera_video.v4l2_buf.index;
    camera_video.user_camera_video_frame_operation_cb(
        camera_video.camera_buffer[buf_index],
        buf_index,
        camera_video.camera_buf_hes,
        camera_video.camera_buf_ves,
        camera_video.camera_buf_size);
}

static inline esp_err_t video_free_video_frame(int video_fd)
{
    if (ioctl(video_fd, VIDIOC_QBUF, &(camera_video.v4l2_buf)) != 0)
    {
        CAMERA_ERROR("failed to free video frame");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static inline esp_err_t video_stream_start(int video_fd)
{
    CAMERA_INFO("Video Stream Start");
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type))
    {
        CAMERA_ERROR("failed to start stream");
        return ESP_FAIL;
    }
    struct v4l2_format format = {0};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_G_FMT, &format) != 0)
    {
        CAMERA_ERROR("get fmt failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static inline esp_err_t video_stream_stop(int video_fd)
{
    CAMERA_INFO("Video Stream Stop");
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMOFF, &type))
    {
        CAMERA_ERROR("failed to stop stream");
        return ESP_FAIL;
    }
    xEventGroupSetBits(camera_video.video_event_group, VIDEO_TASK_DELETE_DONE);
    return ESP_OK;
}

static void video_stream_task(void *arg)
{
    int video_fd = *((int *)arg);
    esp_err_t err = ESP_OK;
    while (1)
    {
        EventBits_t bits = xEventGroupGetBits(camera_video.video_event_group);
        if (bits & VIDEO_TASK_DELETE)
        {
            xEventGroupClearBits(camera_video.video_event_group, VIDEO_TASK_DELETE);
            CAMERA_INFO("stop stream");
            err = video_stream_stop(video_fd);
            if (err != ESP_OK)
            {
                CAMERA_ERROR("stop stream: [ %s ]", esp_err_to_name(err));
            }
            break;
        }
        if (bits & VIDEO_TASK_DISPLAY_EN)
        {
            err = video_receive_video_frame(video_fd);
            if (err != ESP_OK)
            {
                CAMERA_ERROR("receive video frame: [ %s ]", esp_err_to_name(err));
                continue;
            }
            video_operation_video_frame(video_fd);
            err = video_free_video_frame(video_fd);
            if (err != ESP_OK)
            {
                CAMERA_ERROR("free video frame: [ %s ]", esp_err_to_name(err));
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(33));
        }
    }
    vTaskDelete(NULL);
}

esp_err_t video_stream_task_start(int video_fd, int core_id)
{
    if (camera_video.video_event_group == NULL)
    {
        camera_video.video_event_group = xEventGroupCreate();
    }
    xEventGroupClearBits(camera_video.video_event_group, VIDEO_TASK_DELETE_DONE | VIDEO_TASK_DISPLAY_EN);
    video_stream_start(video_fd);
    BaseType_t result = xTaskCreatePinnedToCore(video_stream_task, "video stream task", 4096, &video_fd, 3, &camera_video.video_stream_task_handle, core_id);
    if (result != pdPASS)
    {
        CAMERA_ERROR("failed to create video stream task");
        goto errout;
    }
    return ESP_OK;
errout:
    video_stream_stop(video_fd);
    return ESP_FAIL;
}

esp_err_t video_stream_task_stop(int video_fd)
{
    xEventGroupSetBits(camera_video.video_event_group, VIDEO_TASK_DELETE);
    return ESP_OK;
}

esp_err_t video_register_frame_operation_cb(camera_video_frame_operation_cb_t operation_cb)
{
    camera_video.user_camera_video_frame_operation_cb = operation_cb;
    return ESP_OK;
}

esp_err_t video_stream_wait_stop(void)
{
    xEventGroupWaitBits(camera_video.video_event_group, VIDEO_TASK_DELETE_DONE, pdTRUE, pdTRUE, portMAX_DELAY);
    CAMERA_INFO("Video Stream Task Stopped Done");
    return ESP_OK;
}

static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index, uint32_t camera_buf_hes, uint32_t camera_buf_ves, size_t camera_buf_len)
{
    EventBits_t bits = xEventGroupGetBits(camera_video.video_event_group);
    if (!(bits & VIDEO_TASK_DISPLAY_EN))
        return;
    if (lvgl_port_lock(100))
    {
        lv_canvas_set_buffer(camera_obj, camera_buf, camera_buf_hes, camera_buf_ves, LV_IMG_CF_TRUE_COLOR);
        lv_refr_now(NULL);
        lvgl_port_unlock();
    }
}

int camera_work()
{
    esp_err_t err = ESP_OK;
    size_t cache_line_size = 0;
    camera_video_id = video_open();
    CAMERA_INFO("camera_video_id = %d", camera_video_id);
    esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &cache_line_size);
    for (int i = 0; i < 2; i++)
    {
        cam_buffer[i] = (uint8_t *)heap_caps_aligned_alloc(cache_line_size, H_size * V_size * ((BITS_PER_PIXEL + 7) / 8), MALLOC_CAP_SPIRAM);
        cam_buffer_size[i] = H_size * V_size * ((BITS_PER_PIXEL + 7) / 8);
    }
    err = video_register_frame_operation_cb(camera_video_frame_operation);
    if (err != ESP_OK)
    {
        CAMERA_INFO("register frame operation ERROR");
    }
    if (lvgl_port_lock(0))
    {
        camera_obj = lv_canvas_create(lv_scr_act());
        lv_obj_set_size(camera_obj, 1024, 600);
        memset(cam_buffer[0], 0xFF, cam_buffer_size[0]);
        lv_canvas_set_buffer(camera_obj, cam_buffer[0], 1024, 600, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_align(camera_obj, LV_ALIGN_CENTER);
        lvgl_port_unlock();
    }
    camera_video_set_bufs(camera_video_id, 2, (const void **)cam_buffer);
    video_stream_task_start(camera_video_id, 0);
    return camera_video_id;
}

void set_camera_img_display(bool state)
{
    if (state)
    {
        if (camera_obj != NULL)
            lv_obj_clear_flag(camera_obj, LV_OBJ_FLAG_HIDDEN);

        if (camera_video.video_event_group)
            xEventGroupSetBits(camera_video.video_event_group, VIDEO_TASK_DISPLAY_EN);
    }
    else
    {
        if (camera_obj != NULL)
            lv_obj_add_flag(camera_obj, LV_OBJ_FLAG_HIDDEN);

        if (camera_video.video_event_group)
            xEventGroupClearBits(camera_video.video_event_group, VIDEO_TASK_DISPLAY_EN);
    }
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/