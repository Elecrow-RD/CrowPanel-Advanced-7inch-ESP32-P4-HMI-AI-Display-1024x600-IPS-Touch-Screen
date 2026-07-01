#pragma once

#include "camera.h"
#include "bsp_camera.h"
#include <string>
#include <thread>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct JpegChunk {
    uint8_t* data;
    size_t len;
};

class P4BspCamera : public Camera {
public:
    P4BspCamera();
    ~P4BspCamera() override;

    void SetExplainUrl(const std::string& url, const std::string& token) override;
    bool Capture() override;
    bool SetHMirror(bool enabled) override;
    bool SetVFlip(bool enabled) override;
    std::string Explain(const std::string& question) override;

private:
    std::string explain_url_;
    std::string explain_token_;
    std::thread encoder_thread_;
    bool initialized_;
    TaskHandle_t camera_display_task_handle_;
    
    static void CameraDisplayTask(void* param);
};
