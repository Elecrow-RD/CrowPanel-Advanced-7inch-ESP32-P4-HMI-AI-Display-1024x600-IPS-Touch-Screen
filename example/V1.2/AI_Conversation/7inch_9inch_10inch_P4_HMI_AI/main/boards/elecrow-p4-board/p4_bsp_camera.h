#pragma once

#include "camera.h"
#include "bsp_camera.h"
#include <string>
#include <thread>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct JpegChunk {
    // Points to one temporary block produced by the streaming JPEG encoder.
    uint8_t* data;
    // Number of valid bytes in data; zero with a null pointer marks the end.
    size_t len;
};

/**
 * @brief Adapts the ESP32-P4 camera pipeline to the application Camera API.
 *
 * The class captures RGB565 frames for the LVGL preview and can encode a
 * frame as JPEG before sending it to the configured image-explanation service.
 */
class P4BspCamera : public Camera {
public:
    /**
     * @brief Initialize the sensor, CSI controller, ISP, and frame buffer.
     *
     * @param None.
     * @return A constructed camera object; inspect later calls for readiness.
     * @note Called once while the board object is being constructed.
     */
    P4BspCamera();

    /**
     * @brief Stop the optional preview task before releasing the object.
     *
     * @param None.
     * @return None.
     * @note Called automatically when the camera object is destroyed.
     */
    ~P4BspCamera() override;

    /**
     * @brief Store the endpoint and bearer token used for image explanation.
     *
     * @param url HTTP endpoint that accepts the multipart image request.
     * @param token Bearer token; an empty value omits authorization.
     * @return None.
     * @note Called when the application configures the vision service.
     */
    void SetExplainUrl(const std::string& url, const std::string& token) override;

    /**
     * @brief Capture a fresh frame and publish a copied LVGL preview image.
     *
     * @param None.
     * @return true when capture and preview allocation succeed.
     * @return false when the camera is unavailable or memory allocation fails.
     * @note Called when the application requests a photo or preview update.
     */
    bool Capture() override;

    /**
     * @brief Accept the horizontal-mirror setting for API compatibility.
     *
     * @param enabled Requested horizontal-mirror state.
     * @return true because the current implementation is intentionally a no-op.
     * @note Called when the application changes camera orientation settings.
     */
    bool SetHMirror(bool enabled) override;

    /**
     * @brief Accept the vertical-flip setting for API compatibility.
     *
     * @param enabled Requested vertical-flip state.
     * @return true because the current implementation is intentionally a no-op.
     * @note Called when the application changes camera orientation settings.
     */
    bool SetVFlip(bool enabled) override;

    /**
     * @brief Encode the current frame and request a textual explanation.
     *
     * @param question User question included in the multipart request.
     * @return Response body returned by the explanation service.
     * @note Called after a valid service URL and camera frame are available.
     */
    std::string Explain(const std::string& question) override;

private:
    std::string explain_url_;  // Image-explanation service endpoint.
    std::string explain_token_;  // Optional bearer token for the endpoint.
    std::thread encoder_thread_;  // Streams JPEG chunks without blocking capture.
    bool initialized_;  // Records whether the low-level camera pipeline is ready.
    TaskHandle_t camera_display_task_handle_;  // Optional continuous-preview task.
    
    static void CameraDisplayTask(void* param);
};
