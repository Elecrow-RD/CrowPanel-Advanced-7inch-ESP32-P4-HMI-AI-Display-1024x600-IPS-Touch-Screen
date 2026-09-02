/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_camera.h"
#include "esp_cache.h"
#include "esp_timer.h"

/* SCCB (camera control) I2C bus handle. */
static i2c_master_bus_handle_t sccb_bus_handle = NULL;

/* LVGL canvas object that renders the camera frame. */
static lv_obj_t *camera_obj;

/* Camera video state: buffers, geometry, V4L2 buffer, callbacks. */
static camera_video_t camera_video;

/* Two SPIRAM frame buffers for double-buffered capture. */
uint8_t *cam_buffer[2];
size_t cam_buffer_size[2];
int camera_video_id = 0;

/* Dedicated LVGL display buffers.  Camera USERPTR buffers are returned to
 * the CSI driver immediately after the callback, so LVGL must never render
 * directly from them. */
#define DISPLAY_BUFFER_COUNT 3
static uint8_t *display_buffer[DISPLAY_BUFFER_COUNT];
static size_t display_buffer_size;
static uint8_t display_buffer_index;
static int64_t last_display_time_us;

static const uint8_t red_gain_lut[32] __attribute__((unused)) = {
    0,1,3,4,5,7,8,9,11,12,14,15,16,18,19,20,
    22,23,24,26,27,28,30,31,31,31,31,31,31,31,31,31
};
static const uint8_t green_gain_lut[64] __attribute__((unused)) = {
    0,1,2,3,4,5,6,7,8,8,9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
    30,31,32,33,34,35,36,37,38,39,39,40,41,42,43,44,
    45,46,47,48,49,50,51,52,53,54,55,55,56,57,58,59
};
static const uint8_t blue_gain_lut[32] __attribute__((unused)) = {
    0,1,1,2,3,3,4,5,5,6,7,7,8,9,9,10,
    11,12,12,13,14,14,15,16,16,17,18,18,19,20,20,21
};

/* The ESP32-P4 ISP delivers RGB565 pixels in MSB-first byte order, while
 * LVGL stores RGB565 pixels in the CPU's native (little-endian) order.  This
 * panel's SC2336 module also has a cool/green bias after ISP conversion, so
 * normalise its RGB channels as the frame enters the LVGL buffer. */
static void camera_rgb565_to_lvgl(uint8_t *dst, const uint8_t *src, size_t size)
{
    size_t i;

    for (i = 0; i + 1 < size; i += 2) {
        dst[i] = src[i + 1];
        dst[i + 1] = src[i];
    }
}

/*---------------------------------------------------------------
 * Camera / SCCB initialisation
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the camera SCCB bus and CSI video device.
 *
 * Creates the SCCB I2C master bus (SDA=12, SCL=13) and calls
 * esp_video_init to bring up the MIPI CSI receiver and detected
 * sensor.
 *
 * @return ESP_OK on success.
 */
esp_err_t camera_video_init(void)
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
    if (err != ESP_OK) {
        CAMERA_ERROR("failed to initialize SCCB bus: %s", esp_err_to_name(err));
        return err;
    }

    /* CSI init: reuse the SCCB bus handle for sensor register access. */
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
    if (err != ESP_OK) {
        return err;
    }
    return err;
}

/*---------------------------------------------------------------
 * V4L2 video device open and format negotiation
 *--------------------------------------------------------------*/

/**
 * @brief Open the CSI video device and negotiate RGB565 format.
 *
 * Queries capabilities and current format, then forces RGB565 if
 * the sensor does not report it. Optionally applies vertical/
 * horizontal flip via V4L2 controls.
 *
 * @return File descriptor on success, -1 on failure.
 */
int video_open(void)
{
    struct v4l2_format camera_format;
    struct v4l2_capability capability;

#if CONFIG_ENABLE_CAM_SENSOR_PIC_VFLIP || CONFIG_ENABLE_CAM_SENSOR_PIC_HFLIP
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control control[1];
#endif

    int fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY | O_NONBLOCK, 0);
    if (fd < 0) {
        CAMERA_ERROR("Open video failed");
        return -1;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &capability)) {
        CAMERA_ERROR("failed to get capability");
        goto exit_0;
    }
    CAMERA_INFO("version: %d.%d.%d", (uint16_t)(capability.version >> 16),
               (uint8_t)(capability.version >> 8), (uint8_t)capability.version);
    CAMERA_INFO("driver:  %s", capability.driver);
    CAMERA_INFO("card:    %s", capability.card);
    CAMERA_INFO("bus:     %s", capability.bus_info);

    /* Query the current capture format and remember the geometry. */
    memset(&camera_format, 0, sizeof(struct v4l2_format));
    camera_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &camera_format) != 0) {
        CAMERA_ERROR("failed to get format");
        goto exit_0;
    }
    CAMERA_INFO("initial format: %" PRIu32 "x%" PRIu32 " fourcc=0x%08" PRIx32
                " stride=%" PRIu32 " size=%" PRIu32,
                camera_format.fmt.pix.width, camera_format.fmt.pix.height,
                camera_format.fmt.pix.pixelformat, camera_format.fmt.pix.bytesperline,
                camera_format.fmt.pix.sizeimage);
    camera_video.camera_buf_hes = camera_format.fmt.pix.width;
    camera_video.camera_buf_ves = camera_format.fmt.pix.height;

    /* Force RGB565 if the sensor is not already in that format. */
    if (camera_format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) {
        struct v4l2_format format = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .fmt.pix.width = camera_format.fmt.pix.width,
            .fmt.pix.height = camera_format.fmt.pix.height,
            .fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565,
        };
        if (ioctl(fd, VIDIOC_S_FMT, &format) != 0) {
            CAMERA_ERROR("failed to set format");
            goto exit_0;
        }
        camera_format = format;
    }
    if (camera_format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) {
        CAMERA_ERROR("driver did not accept RGB565 format");
        goto exit_0;
    }
    camera_video.camera_buf_hes = camera_format.fmt.pix.width;
    camera_video.camera_buf_ves = camera_format.fmt.pix.height;
    CAMERA_INFO("capture format: %" PRIu32 "x%" PRIu32 " RGB565"
                " stride=%" PRIu32 " size=%" PRIu32,
                camera_video.camera_buf_hes, camera_video.camera_buf_ves,
                camera_format.fmt.pix.bytesperline, camera_format.fmt.pix.sizeimage);
    CAMERA_INFO("app_video_open successful");

    /* Optional vertical flip. */
#if CONFIG_ENABLE_CAM_SENSOR_PIC_VFLIP
    controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    controls.count = 1;
    controls.controls = control;
    control[0].id = V4L2_CID_VFLIP;
    control[0].value = 1;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        CAMERA_ERROR("failed to mirror the frame horizontally and skip this step");
    }
#endif

    /* Optional horizontal flip. */
#if CONFIG_ENABLE_CAM_SENSOR_PIC_HFLIP
    controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    controls.count = 1;
    controls.controls = control;
    control[0].id = V4L2_CID_HFLIP;
    control[0].value = 1;
    if (ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) != 0) {
        CAMERA_ERROR("failed to mirror the frame horizontally and skip this step");
    }
#endif

    CAMERA_INFO("fd = %d", fd);
    return fd;

exit_0:
    close(fd);
    return -1;
}

/*---------------------------------------------------------------
 * Buffer management
 *--------------------------------------------------------------*/

/**
 * @brief Request and queue V4L2 capture buffers.
 *
 * @param video_fd Open video device fd.
 * @param fb_num   Number of buffers (2..MAX_BUFFER_COUNT).
 * @return ESP_OK on success.
 */
esp_err_t camera_video_set_bufs(int video_fd, uint32_t fb_num)
{
    struct v4l2_requestbuffers req = {0};
    size_t cache_line_size = 0;
    size_t minimum_frame_size = app_video_get_buf_size();
    esp_err_t err;

    if (fb_num > MAX_BUFFER_COUNT) {
        CAMERA_ERROR("buffer num is too large");
        return ESP_FAIL;
    } else if (fb_num < 2) {
        CAMERA_ERROR("At least two buffers are required");
        return ESP_FAIL;
    }

    req.count = fb_num;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_video.camera_mem_mode = req.memory = V4L2_MEMORY_USERPTR;

    if (ioctl(video_fd, VIDIOC_REQBUFS, &req) != 0) {
        CAMERA_ERROR("req bufs failed");
        return ESP_FAIL;
    }
    if (req.count < fb_num) {
        CAMERA_ERROR("driver provided only %" PRIu32 " buffers", req.count);
        goto errout_req_bufs;
    }

    err = esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &cache_line_size);
    if (err != ESP_OK || cache_line_size == 0) {
        CAMERA_ERROR("failed to get cache alignment: %s", esp_err_to_name(err));
        goto errout_req_bufs;
    }

    /* The V4L2 driver owns the required frame length. Never derive it from LCD size. */
    for (int i = 0; i < fb_num; i++) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = req.memory;
        buf.index = i;
        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) != 0) {
            CAMERA_ERROR("query buf failed");
            goto errout_req_bufs;
        }
        if (buf.length < minimum_frame_size) {
            CAMERA_ERROR("frame buffer %d is too small: %" PRIu32 " < %u bytes",
                         i, buf.length, (unsigned)minimum_frame_size);
            goto errout_req_bufs;
        }

        camera_video.camera_buffer[i] = heap_caps_aligned_alloc(
                                            cache_line_size, buf.length,
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED);
        if (camera_video.camera_buffer[i] == NULL) {
            CAMERA_ERROR("failed to allocate frame buffer %d (%" PRIu32 " bytes)",
                         i, buf.length);
            goto errout_req_bufs;
        }
        camera_video.camera_buf_size[i] = buf.length;
        buf.m.userptr = (unsigned long)camera_video.camera_buffer[i];

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) != 0) {
            CAMERA_ERROR("queue frame buffer failed");
            goto errout_req_bufs;
        }
        CAMERA_INFO("frame buffer %d: %" PRIu32 " bytes", i, buf.length);
    }
    camera_video.camera_buffer_count = fb_num;
    return ESP_OK;

errout_req_bufs:
    for (int i = 0; i < MAX_BUFFER_COUNT; i++) {
        if (camera_video.camera_buffer[i] != NULL) {
            heap_caps_free(camera_video.camera_buffer[i]);
            camera_video.camera_buffer[i] = NULL;
            camera_video.camera_buf_size[i] = 0;
        }
    }
    camera_video.camera_buffer_count = 0;
    req.count = 0;
    ioctl(video_fd, VIDIOC_REQBUFS, &req);
    return ESP_FAIL;
}

/**
 * @brief Retrieve the capture buffer pointers into the caller's array.
 */
esp_err_t camera_video_get_bufs(int fb_num, void **fb)
{
    if (fb_num > MAX_BUFFER_COUNT) {
        CAMERA_ERROR("buffer num is too large");
        return ESP_FAIL;
    } else if (fb_num < 2) {
        CAMERA_ERROR("At least two buffers are required");
        return ESP_FAIL;
    }
    for (int i = 0; i < fb_num; i++) {
        if (camera_video.camera_buffer[i] != NULL) {
            fb[i] = camera_video.camera_buffer[i];
        } else {
            CAMERA_ERROR("frame buffer is NULL");
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

/**
 * @brief Return the byte size of one frame (width * height * 2 for RGB565).
 */
uint32_t app_video_get_buf_size(void)
{
    uint32_t buf_size = camera_video.camera_buf_hes * camera_video.camera_buf_ves * 2;
    return buf_size;
}

/*---------------------------------------------------------------
 * Frame receive / operate / free (stream loop helpers)
 *--------------------------------------------------------------*/

/**
 * @brief Dequeue a captured frame from the V4L2 device.
 */
static inline esp_err_t video_receive_video_frame(int video_fd)
{
    memset(&camera_video.v4l2_buf, 0, sizeof(camera_video.v4l2_buf));
    camera_video.v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_video.v4l2_buf.memory = camera_video.camera_mem_mode;
    int res = ioctl(video_fd, VIDIOC_DQBUF, &(camera_video.v4l2_buf));
    if (res != 0) {
        CAMERA_ERROR("failed to receive video frame");
        return ESP_FAIL;
    }
    if (camera_video.v4l2_buf.index >= camera_video.camera_buffer_count) {
        CAMERA_ERROR("driver returned invalid buffer index %" PRIu32,
                     camera_video.v4l2_buf.index);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief Invoke the user frame-operation callback with the dequeued buffer.
 */
static inline void video_operation_video_frame(void)
{
    uint8_t buf_index = camera_video.v4l2_buf.index;
    camera_video.v4l2_buf.m.userptr = (unsigned long)camera_video.camera_buffer[buf_index];
    camera_video.v4l2_buf.length = camera_video.camera_buf_size[buf_index];
    if (camera_video.user_camera_video_frame_operation_cb != NULL) {
        camera_video.user_camera_video_frame_operation_cb(
            camera_video.camera_buffer[buf_index],
            buf_index,
            camera_video.camera_buf_hes,
            camera_video.camera_buf_ves,
            camera_video.camera_buf_size[buf_index]);
    }
}

/**
 * @brief Re-queue a processed buffer so the device can refill it.
 */
static inline esp_err_t video_free_video_frame(int video_fd)
{
    if (ioctl(video_fd, VIDIOC_QBUF, &(camera_video.v4l2_buf)) != 0) {
        CAMERA_ERROR("failed to free video frame");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/*---------------------------------------------------------------
 * Stream start / stop
 *--------------------------------------------------------------*/

/**
 * @brief Start the V4L2 capture stream (VIDIOC_STREAMON).
 */
static inline esp_err_t video_stream_start(int video_fd)
{
    CAMERA_INFO("Video Stream Start");
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type)) {
        CAMERA_ERROR("failed to start stream");
        return ESP_FAIL;
    }
    struct v4l2_format format = {0};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_G_FMT, &format) != 0) {
        CAMERA_ERROR("get fmt failed");
        ioctl(video_fd, VIDIOC_STREAMOFF, &type);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief Stop the V4L2 capture stream and signal the task-done event.
 */
static inline esp_err_t video_stream_stop(int video_fd)
{
    CAMERA_INFO("Video Stream Stop");
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMOFF, &type)) {
        CAMERA_ERROR("failed to stop stream");
        return ESP_FAIL;
    }
    xEventGroupSetBits(camera_video.video_event_group, VIDEO_TASK_DELETE_DONE);
    return ESP_OK;
}

/*---------------------------------------------------------------
 * Video stream task
 *--------------------------------------------------------------*/

/**
 * @brief Capture loop: dequeue -> operate -> requeue, until asked to stop.
 *
 * When display is disabled the task sleeps 33 ms to avoid busy-waiting.
 * When the DELETE bit is set it stops the stream and deletes itself.
 *
 * @param arg Video file descriptor encoded as an integer value.
 */
static void video_stream_task(void *arg)
{
    int video_fd = (int)(intptr_t)arg;
    esp_err_t err = ESP_OK;

    while (1) {
        EventBits_t bits = xEventGroupGetBits(camera_video.video_event_group);

        /* Stop request: stop the stream and exit. */
        if (bits & VIDEO_TASK_DELETE) {
            xEventGroupClearBits(camera_video.video_event_group, VIDEO_TASK_DELETE);
            CAMERA_INFO("stop stream");
            err = video_stream_stop(video_fd);
            if (err != ESP_OK) {
                CAMERA_ERROR("stop stream: [ %s ]", esp_err_to_name(err));
            }
            break;
        }

        /* Display enabled: capture, operate, recycle one frame. */
        if (bits & VIDEO_TASK_DISPLAY_EN) {
            err = video_receive_video_frame(video_fd);
            if (err != ESP_OK) {
                CAMERA_ERROR("receive video frame: [ %s ]", esp_err_to_name(err));
                continue;
            }
            video_operation_video_frame();
            err = video_free_video_frame(video_fd);
            if (err != ESP_OK) {
                CAMERA_ERROR("free video frame: [ %s ]", esp_err_to_name(err));
            }
        } else {
            /* Display off: yield so other tasks can run. */
            vTaskDelay(pdMS_TO_TICKS(33));
        }
    }
    vTaskDelete(NULL);
}

/**
 * @brief Create the video stream task pinned to a core and start streaming.
 * @param video_fd Open video device fd.
 * @param core_id  Core to pin the task to.
 * @return ESP_OK on success.
 */
esp_err_t video_stream_task_start(int video_fd, int core_id)
{
    if (camera_video.video_event_group == NULL) {
        camera_video.video_event_group = xEventGroupCreate();
        if (camera_video.video_event_group == NULL) {
            CAMERA_ERROR("failed to create video event group");
            return ESP_ERR_NO_MEM;
        }
    }
    xEventGroupClearBits(camera_video.video_event_group,
                        VIDEO_TASK_DELETE_DONE | VIDEO_TASK_DISPLAY_EN);
    esp_err_t err = video_stream_start(video_fd);
    if (err != ESP_OK) {
        return err;
    }

    BaseType_t result = xTaskCreatePinnedToCore(video_stream_task, "video stream task",
                                                4096, (void *)(intptr_t)video_fd, 3,
                                                &camera_video.video_stream_task_handle, core_id);
    if (result != pdPASS) {
        CAMERA_ERROR("failed to create video stream task");
        goto errout;
    }
    return ESP_OK;

errout:
    video_stream_stop(video_fd);
    return ESP_FAIL;
}

/**
 * @brief Request the video stream task to stop (async).
 */
esp_err_t video_stream_task_stop(int video_fd)
{
    if (camera_video.video_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    xEventGroupSetBits(camera_video.video_event_group, VIDEO_TASK_DELETE);
    return ESP_OK;
}

/**
 * @brief Register the per-frame callback used by the stream task.
 */
esp_err_t video_register_frame_operation_cb(camera_video_frame_operation_cb_t operation_cb)
{
    if (operation_cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    camera_video.user_camera_video_frame_operation_cb = operation_cb;
    return ESP_OK;
}

/**
 * @brief Block until the video stream task has stopped.
 */
esp_err_t video_stream_wait_stop(void)
{
    xEventGroupWaitBits(camera_video.video_event_group, VIDEO_TASK_DELETE_DONE,
                        pdTRUE, pdTRUE, portMAX_DELAY);
    CAMERA_INFO("Video Stream Task Stopped Done");
    return ESP_OK;
}

/*---------------------------------------------------------------
 * LVGL canvas drawing callback
 *--------------------------------------------------------------*/

/**
 * @brief Per-frame callback: draw the camera buffer onto the LVGL canvas.
 *
 * Called by the stream task for every captured frame. Locks LVGL,
 * swaps the canvas buffer to the just-captured frame and forces a
 * refresh so the new frame appears on screen.
 */
static void camera_video_frame_operation(uint8_t *camera_buf, uint8_t camera_buf_index,
                                         uint32_t camera_buf_hes, uint32_t camera_buf_ves,
                                         size_t camera_buf_len)
{
    EventBits_t bits = xEventGroupGetBits(camera_video.video_event_group);
    if (!(bits & VIDEO_TASK_DISPLAY_EN)) {
        return;
    }

    /* Keep LVGL refresh work bounded.  CSI may deliver 30 frames/s, while
     * the DSI panel and LVGL task do not need a forced full refresh for every
     * frame.  The capture buffer is still recycled immediately. */
    int64_t now_us = esp_timer_get_time();
    if (last_display_time_us != 0 && (now_us - last_display_time_us) < 42000) {
        return;
    }
    last_display_time_us = now_us;

    /*
     * CSI writes the frame directly to PSRAM through DMA.  The V4L2 CSI
     * device does not invalidate the CPU cache when a USERPTR buffer is
     * dequeued, so LVGL can otherwise blend stale cache lines with the new
     * frame.  This looks like coloured noise while the scene outline remains
     * visible.  Synchronise the complete RGB565 frame before LVGL reads it.
     */
    const uint32_t out_w = (camera_buf_hes > H_size) ? H_size : camera_buf_hes;
    const uint32_t out_h = (camera_buf_ves > V_size) ? V_size : camera_buf_ves;
    const uint32_t src_x = (camera_buf_hes - out_w) / 2;
    const uint32_t src_y = (camera_buf_ves - out_h) / 2;
    const size_t src_frame_size = (size_t)camera_buf_hes * camera_buf_ves * sizeof(uint16_t);
    const size_t frame_size = (size_t)out_w * out_h * sizeof(uint16_t);
    if (camera_buf_len < src_frame_size) {
        CAMERA_ERROR("captured frame is too small: %u < %u bytes",
                     (unsigned)camera_buf_len, (unsigned)src_frame_size);
        return;
    }
    esp_err_t cache_err = esp_cache_msync(camera_buf, frame_size,
                                          ESP_CACHE_MSYNC_FLAG_DIR_M2C |
                                          ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    if (cache_err != ESP_OK) {
        CAMERA_ERROR("failed to invalidate camera frame cache: %s",
                     esp_err_to_name(cache_err));
        return;
    }

    uint8_t next_index = (uint8_t)((display_buffer_index + 1) % DISPLAY_BUFFER_COUNT);
    if (display_buffer[next_index] == NULL || display_buffer_size < frame_size) {
        CAMERA_ERROR("display frame buffer is not allocated");
        return;
    }

    /* ISP RGB565 is MSB-first.  Convert it to LVGL's native RGB565 layout;
     * the display driver will perform the required final panel byte swap. */
    for (uint32_t y = 0; y < out_h; y++) {
        const uint8_t *src = camera_buf + ((size_t)(src_y + y) * camera_buf_hes + src_x) * 2;
        uint8_t *dst = display_buffer[next_index] + (size_t)y * out_w * 2;
        camera_rgb565_to_lvgl(dst, src, (size_t)out_w * 2);
    }
    /* Make the copied frame visible to the LCD DMA before LVGL refreshes. */
    cache_err = esp_cache_msync(display_buffer[next_index], frame_size,
                                ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    if (cache_err != ESP_OK) {
        CAMERA_ERROR("failed to flush display frame cache: %s",
                     esp_err_to_name(cache_err));
        return;
    }

    display_buffer_index = next_index;
    if (lvgl_port_lock(100)) {
        lv_canvas_set_buffer(camera_obj, display_buffer[next_index], out_w, out_h,
                             LV_COLOR_FORMAT_RGB565);
        lv_obj_invalidate(camera_obj);
        lvgl_port_unlock();
    }
}

/*---------------------------------------------------------------
 * Camera bring-up entry point
 *--------------------------------------------------------------*/

/**
 * @brief Open the video device, allocate frame buffers and start streaming.
 *
 * Allocates two cache-aligned SPIRAM buffers, registers the LVGL
 * draw callback, creates the LVGL canvas, queues the buffers and
 * starts the stream task.
 *
 * @return Video file descriptor on success, -1 on failure.
 */
int camera_work(void)
{
    esp_err_t err = ESP_OK;

    camera_video_id = video_open();
    CAMERA_INFO("camera_video_id = %d", camera_video_id);
    if (camera_video_id < 0) {
        return -1;
    }

    err = camera_video_set_bufs(camera_video_id, 2);
    if (err != ESP_OK) {
        CAMERA_ERROR("failed to prepare frame buffers: %s", esp_err_to_name(err));
        goto errout;
    }
    for (int i = 0; i < 2; i++) {
        cam_buffer[i] = camera_video.camera_buffer[i];
        cam_buffer_size[i] = camera_video.camera_buf_size[i];
    }

    display_buffer_size = (size_t)((camera_video.camera_buf_hes > H_size) ? H_size : camera_video.camera_buf_hes) *
                          ((camera_video.camera_buf_ves > V_size) ? V_size : camera_video.camera_buf_ves) * sizeof(uint16_t);
    display_buffer_index = 0;
    for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
        display_buffer[i] = heap_caps_aligned_alloc(64, display_buffer_size,
                                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED);
        if (display_buffer[i] == NULL) {
            CAMERA_ERROR("failed to allocate display buffer %d", i);
            for (int j = 0; j < DISPLAY_BUFFER_COUNT; j++) {
                if (display_buffer[j]) {
                    heap_caps_free(display_buffer[j]);
                    display_buffer[j] = NULL;
                }
            }
            display_buffer_size = 0;
            goto errout;
        }
        memset(display_buffer[i], 0xFF, display_buffer_size);
    }

    err = video_register_frame_operation_cb(camera_video_frame_operation);
    if (err != ESP_OK) {
        CAMERA_ERROR("register frame operation failed: %s", esp_err_to_name(err));
        goto errout;
    }

    /* Create the LVGL canvas and bind the first buffer. */
    if (!lvgl_port_lock(1000)) {
        CAMERA_ERROR("timed out while locking LVGL");
        goto errout;
    }
    camera_obj = lv_canvas_create(lv_screen_active());
    if (camera_obj == NULL) {
        lvgl_port_unlock();
        CAMERA_ERROR("failed to create camera canvas");
        goto errout;
    }
    const uint32_t canvas_w = (camera_video.camera_buf_hes > H_size) ? H_size : camera_video.camera_buf_hes;
    const uint32_t canvas_h = (camera_video.camera_buf_ves > V_size) ? V_size : camera_video.camera_buf_ves;
    lv_canvas_set_buffer(camera_obj, display_buffer[display_buffer_index],
                         canvas_w, canvas_h,
                         LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(camera_obj, canvas_w, canvas_h);
    lv_obj_set_align(camera_obj, LV_ALIGN_CENTER);
    lvgl_port_unlock();

    err = video_stream_task_start(camera_video_id, 0);
    if (err != ESP_OK) {
        CAMERA_ERROR("failed to start video stream task: %s", esp_err_to_name(err));
        goto errout;
    }
    return camera_video_id;

errout:
    if (camera_obj != NULL && lvgl_port_lock(1000)) {
        lv_obj_delete(camera_obj);
        camera_obj = NULL;
        lvgl_port_unlock();
    }
    for (int i = 0; i < MAX_BUFFER_COUNT; i++) {
        if (camera_video.camera_buffer[i] != NULL) {
            heap_caps_free(camera_video.camera_buffer[i]);
            camera_video.camera_buffer[i] = NULL;
            camera_video.camera_buf_size[i] = 0;
        }
    }
    for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
        if (display_buffer[i] != NULL) {
            heap_caps_free(display_buffer[i]);
            display_buffer[i] = NULL;
        }
    }
    display_buffer_size = 0;
    camera_video.camera_buffer_count = 0;
    close(camera_video_id);
    camera_video_id = -1;
    return -1;
}

/**
 * @brief Show or hide the live camera image.
 * @param state true = show, false = hide.
 */
void set_camera_img_display(bool state)
{
    if (!lvgl_port_lock(1000)) {
        CAMERA_ERROR("timed out while locking LVGL");
        return;
    }
    if (state) {
        if (camera_obj != NULL) {
            lv_obj_clear_flag(camera_obj, LV_OBJ_FLAG_HIDDEN);
        }
        if (camera_video.video_event_group) {
            xEventGroupSetBits(camera_video.video_event_group, VIDEO_TASK_DISPLAY_EN);
        }
    } else {
        if (camera_obj != NULL) {
            lv_obj_add_flag(camera_obj, LV_OBJ_FLAG_HIDDEN);
        }
        if (camera_video.video_event_group) {
            xEventGroupClearBits(camera_video.video_event_group, VIDEO_TASK_DISPLAY_EN);
        }
    }
    lvgl_port_unlock();
}
