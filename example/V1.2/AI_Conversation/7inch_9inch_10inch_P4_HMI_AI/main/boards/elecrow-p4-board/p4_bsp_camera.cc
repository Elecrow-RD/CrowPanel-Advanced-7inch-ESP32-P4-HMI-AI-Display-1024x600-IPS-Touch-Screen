#include "p4_bsp_camera.h"
#include "board.h"
#include "lvgl_display.h"   // Correct path: header file is in display/lvgl_display directory, provided by INCLUDE_DIRS
#include "system_info.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <img_converters.h>
#include <cstring>
#include <lvgl.h>
#include <esp_lvgl_port.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "P4BspCamera"

extern "C" {
    void camera_display();
    void camera_display_refresh();
    esp_err_t camera_refresh();
}

/**
 * @brief Continuously acquire frames and invalidate the LVGL camera object.
 *
 * @param param Unused FreeRTOS task argument.
 * @return None; the task runs until it is deleted.
 * @note Reserved for continuous preview and not started by the constructor.
 */
void P4BspCamera::CameraDisplayTask(void* /*param*/) {
    // Keep function for future use when continuous preview is needed, currently not creating this task at startup,
    // to avoid full screen occupation immediately after power on.
    while (1) {
        // Refresh camera data
        camera_refresh();
        // Refresh display
        if (lvgl_port_lock(0)) {
            camera_display_refresh();
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(23));  // Approximately 43 FPS (refresh every 23ms)
    }
}

/**
 * @brief Build the application camera adapter and acquire its first frame.
 *
 * @param None.
 * @return A constructed object whose initialized_ flag records readiness.
 * @note Called once from ElecrowP4Board::InitializeCamera().
 */
P4BspCamera::P4BspCamera() : initialized_(false), camera_display_task_handle_(nullptr) {
    esp_err_t err = camera_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed: %s", esp_err_to_name(err));
        return;
    }

    // Only complete low-level camera initialization, do not create full screen preview at startup,
    // to avoid covering the main interface immediately after power on. When photo/preview is actually needed,
    // Capture() actively refreshes and displays thumbnail through Display's SetPreviewImage.

    // Pre-refresh one frame to ensure valid image data in buffer (for Explain use)
    camera_refresh();

    initialized_ = true;
    ESP_LOGI(TAG, "P4 BSP Camera initialized successfully");
}

/**
 * @brief Stop the optional continuous-preview task during destruction.
 *
 * @param None.
 * @return None.
 * @note Called automatically when the board releases the camera object.
 */
P4BspCamera::~P4BspCamera() {
    if (camera_display_task_handle_ != nullptr) {
        vTaskDelete(camera_display_task_handle_);
        camera_display_task_handle_ = nullptr;
    }
}

/**
 * @brief Save the remote vision endpoint and its optional bearer token.
 *
 * @param url Endpoint that accepts the multipart image request.
 * @param token Authorization token; leave empty to omit the header.
 * @return None.
 * @note Called before Explain() is allowed to send an image.
 */
void P4BspCamera::SetExplainUrl(const std::string& url, const std::string& token) {
    explain_url_ = url;
    explain_token_ = token;
}

/**
 * @brief Refresh the RGB565 frame and hand a private copy to LVGL.
 *
 * @param None.
 * @return true when the new frame is ready for preview.
 * @return false when capture or memory allocation fails.
 * @note Called whenever the application requests a camera snapshot.
 */
bool P4BspCamera::Capture() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Camera not initialized");
        return false;
    }

    esp_err_t err = camera_refresh();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera refresh failed: %s", esp_err_to_name(err));
        return false;
    }

    // Display preview image
    auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
    if (display != nullptr) {
        // Update img_camera data pointer (my_trans.buffer already contains new data)
        img_camera.data = (uint8_t*)my_trans.buffer;
        img_camera.data_size = 1024 * 600 * 2;
        
        // Create preview image descriptor
        auto img_dsc = (lv_img_dsc_t*)heap_caps_calloc(1, sizeof(lv_img_dsc_t), MALLOC_CAP_8BIT);
        if (img_dsc == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for preview image descriptor");
            return false;
        }
        
        img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
        img_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
        img_dsc->header.flags = 0;
        img_dsc->header.w = 1024;
        img_dsc->header.h = 600;
        img_dsc->header.stride = 1024 * 2;
        img_dsc->data_size = 1024 * 600 * 2;
        
        // Allocate memory and copy data
        img_dsc->data = (uint8_t*)heap_caps_malloc(img_dsc->data_size, MALLOC_CAP_SPIRAM);
        if (img_dsc->data == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate memory for preview image");
            heap_caps_free(img_dsc);
            return false;
        }
        
        // Copy data (my_trans.buffer is already in RGB565 format)
        memcpy((void*)img_dsc->data, my_trans.buffer, img_dsc->data_size);
        
        display->SetPreviewImage(img_dsc);
    }
    
    return true;
}

/**
 * @brief Report a horizontal-mirror request without changing the sensor.
 *
 * @param enabled Requested state.
 * @return true because this compatibility operation is a no-op.
 * @note Called through the generic Camera interface.
 */
bool P4BspCamera::SetHMirror(bool enabled) {
    // Current project is mainly for preview display, here we don't directly control low-level camera mirroring, simply log and return success
    ESP_LOGI(TAG, "SetHMirror(%s) called (no-op)", enabled ? "true" : "false");
    (void)enabled;
    return true;
}

/**
 * @brief Report a vertical-flip request without changing the sensor.
 *
 * @param enabled Requested state.
 * @return true because this compatibility operation is a no-op.
 * @note Called through the generic Camera interface.
 */
bool P4BspCamera::SetVFlip(bool enabled) {
    ESP_LOGI(TAG, "SetVFlip(%s) called (no-op)", enabled ? "true" : "false");
    (void)enabled;
    return true;
}

/**
 * @brief Encode the current RGB565 frame and upload it for explanation.
 *
 * JPEG chunks cross from the encoder thread to the HTTP writer through a
 * FreeRTOS queue, avoiding the need for one large contiguous JPEG buffer.
 *
 * @param question User prompt sent beside the image.
 * @return Text returned by the remote explanation service.
 * @note Called only after SetExplainUrl() and successful camera startup.
 */
std::string P4BspCamera::Explain(const std::string& question) {
    if (explain_url_.empty()) {
        throw std::runtime_error("Image explain URL or token is not set");
    }

    if (!initialized_ || my_trans.buffer == nullptr) {
        throw std::runtime_error("Camera not initialized or buffer not available");
    }

    // Create JPEG queue
    QueueHandle_t jpeg_queue = xQueueCreate(40, sizeof(JpegChunk));
    if (jpeg_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create JPEG queue");
        throw std::runtime_error("Failed to create JPEG queue");
    }

    // Create camera_fb_t structure for JPEG encoding (avoid C++ restrictions on designated initialization order)
    camera_fb_t fb = {};
    fb.buf = (uint8_t*)my_trans.buffer;
    fb.len = my_trans.buflen;
    fb.width  = 1024;
    fb.height = 600;
    fb.format = PIXFORMAT_RGB565;
    // fb.timestamp keeps default 0 initialization (camera_fb_t is scalar in C, timeval structure in C++)

    // Encode JPEG in separate thread
    if (encoder_thread_.joinable()) {
        encoder_thread_.join();
    }

    encoder_thread_ = std::thread([this, jpeg_queue, &fb]() {
        frame2jpg_cb(&fb, 80, [](void* arg, size_t index, const void* data, size_t len) -> unsigned int {
            auto jpeg_queue = (QueueHandle_t)arg;
            JpegChunk chunk = {
                .data = (uint8_t*)heap_caps_aligned_alloc(16, len, MALLOC_CAP_SPIRAM),
                .len = len
            };
            if (chunk.data == nullptr) {
                return 0;
            }
            memcpy(chunk.data, data, len);
            xQueueSend(jpeg_queue, &chunk, portMAX_DELAY);
            return len;
        }, jpeg_queue);
        
        // Send end marker
        JpegChunk end_chunk = {.data = nullptr, .len = 0};
        xQueueSend(jpeg_queue, &end_chunk, portMAX_DELAY);
    });

    auto network = Board::GetInstance().GetNetwork();
    auto http = network->CreateHttp(3);
    std::string boundary = "----ESP32_CAMERA_BOUNDARY";

    // Configure HTTP client
    http->SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());
    http->SetHeader("Client-Id", Board::GetInstance().GetUuid().c_str());
    if (!explain_token_.empty()) {
        http->SetHeader("Authorization", "Bearer " + explain_token_);
    }
    http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http->SetHeader("Transfer-Encoding", "chunked");
    
    if (!http->Open("POST", explain_url_)) {
        ESP_LOGE(TAG, "Failed to connect to explain URL");
        encoder_thread_.join();
        JpegChunk chunk;
        while (xQueueReceive(jpeg_queue, &chunk, 0) == pdPASS) {
            if (chunk.data != nullptr) {
                heap_caps_free(chunk.data);
            } else {
                break;
            }
        }
        vQueueDelete(jpeg_queue);
        throw std::runtime_error("Failed to connect to explain URL");
    }

    // Send question field
    std::string question_field;
    question_field += "--" + boundary + "\r\n";
    question_field += "Content-Disposition: form-data; name=\"question\"\r\n";
    question_field += "\r\n";
    question_field += question + "\r\n";
    http->Write(question_field.c_str(), question_field.size());

    // Send file field header
    std::string file_header;
    file_header += "--" + boundary + "\r\n";
    file_header += "Content-Disposition: form-data; name=\"file\"; filename=\"camera.jpg\"\r\n";
    file_header += "Content-Type: image/jpeg\r\n";
    file_header += "\r\n";
    http->Write(file_header.c_str(), file_header.size());

    // Send JPEG data
    size_t total_sent = 0;
    while (true) {
        JpegChunk chunk;
        if (xQueueReceive(jpeg_queue, &chunk, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "Failed to receive JPEG chunk");
            break;
        }
        if (chunk.data == nullptr) {
            break; // End marker
        }
        http->Write((const char*)chunk.data, chunk.len);
        total_sent += chunk.len;
        heap_caps_free(chunk.data);
    }
    
    encoder_thread_.join();
    vQueueDelete(jpeg_queue);

    // Send multipart footer
    std::string multipart_footer;
    multipart_footer += "\r\n--" + boundary + "--\r\n";
    http->Write(multipart_footer.c_str(), multipart_footer.size());
    http->Write("", 0);

    if (http->GetStatusCode() != 200) {
        ESP_LOGE(TAG, "Failed to upload photo, status code: %d", http->GetStatusCode());
        throw std::runtime_error("Failed to upload photo");
    }

    std::string result = http->ReadAll();
    http->Close();

    ESP_LOGI(TAG, "Explain image size=1024x600, compressed size=%d, question=%s\n%s",
        total_sent, question.c_str(), result.c_str());
    
    return result;
}
