#include "protocol.h"

#include <esp_log.h>

#define TAG "Protocol"

/*---------------------------------------------------------------
 * Protocol event registration
 * Store application callbacks so each transport can report incoming
 * messages, channel state, and network errors through one interface.
 *--------------------------------------------------------------*/

/**
 * @brief Register the handler for incoming JSON control messages.
 * @param callback Function that consumes a parsed cJSON root object.
 * @return None.
 * @note Called while Application configures the selected transport.
 */
void Protocol::OnIncomingJson(std::function<void(const cJSON* root)> callback) {
    on_incoming_json_ = callback;
}

/**
 * @brief Register the handler for incoming encoded audio packets.
 * @param callback Function that takes ownership of one audio packet.
 * @return None.
 * @note Called before the audio channel is opened.
 */
void Protocol::OnIncomingAudio(std::function<void(std::unique_ptr<AudioStreamPacket> packet)> callback) {
    on_incoming_audio_ = callback;
}

/**
 * @brief Register the notification for a usable audio channel.
 * @param callback Function invoked after transport negotiation succeeds.
 * @return None.
 * @note Called during application protocol setup.
 */
void Protocol::OnAudioChannelOpened(std::function<void()> callback) {
    on_audio_channel_opened_ = callback;
}

/**
 * @brief Register the notification for audio-channel closure.
 * @param callback Function invoked after disconnection or channel shutdown.
 * @return None.
 * @note Called during application protocol setup.
 */
void Protocol::OnAudioChannelClosed(std::function<void()> callback) {
    on_audio_channel_closed_ = callback;
}

/**
 * @brief Register the handler that presents transport errors to the user.
 * @param callback Function that receives a readable error message.
 * @return None.
 * @note Called before the transport starts connecting.
 */
void Protocol::OnNetworkError(std::function<void(const std::string& message)> callback) {
    on_network_error_ = callback;
}

/**
 * @brief Register the transport-connected notification.
 * @param callback Function invoked after the underlying network connects.
 * @return None.
 * @note Called during application protocol setup.
 */
void Protocol::OnConnected(std::function<void()> callback) {
    on_connected_ = callback;
}

/**
 * @brief Register the transport-disconnected notification.
 * @param callback Function invoked when the underlying network disconnects.
 * @return None.
 * @note Called during application protocol setup.
 */
void Protocol::OnDisconnected(std::function<void()> callback) {
    on_disconnected_ = callback;
}

/*---------------------------------------------------------------
 * Session control messages
 * Translate application actions into the JSON schema understood by
 * the remote conversational service.
 *--------------------------------------------------------------*/

/**
 * @brief Record a transport error and notify the application once.
 * @param message User-facing description of the failure.
 * @return None.
 * @note Called by a concrete transport when an operation fails.
 */
void Protocol::SetError(const std::string& message) {
    error_occurred_ = true;
    if (on_network_error_ != nullptr) {
        on_network_error_(message);
    }
}

/**
 * @brief Ask the server to stop the current speech response.
 * @param reason Reason used to distinguish a wake-word interruption.
 * @return None.
 * @note Called when the user interrupts playback or changes chat state.
 */
void Protocol::SendAbortSpeaking(AbortReason reason) {
    std::string message = "{\"session_id\":\"" + session_id_ + "\",\"type\":\"abort\"";
    if (reason == kAbortReasonWakeWordDetected) {
        message += ",\"reason\":\"wake_word_detected\"";
    }
    message += "}";
    SendText(message);
}

/**
 * @brief Report the locally detected wake word to the current session.
 * @param wake_word Text produced by the wake-word engine.
 * @return None.
 * @note Called immediately after wake-word audio is prepared for upload.
 */
void Protocol::SendWakeWordDetected(const std::string& wake_word) {
    std::string json = "{\"session_id\":\"" + session_id_ + 
                      "\",\"type\":\"listen\",\"state\":\"detect\",\"text\":\"" + wake_word + "\"}";
    SendText(json);
}

/**
 * @brief Tell the server to begin listening in the selected mode.
 * @param mode Realtime, automatic-stop, or manual-stop listening mode.
 * @return None.
 * @note Called when Application enters the listening state.
 */
void Protocol::SendStartListening(ListeningMode mode) {
    std::string message = "{\"session_id\":\"" + session_id_ + "\"";
    message += ",\"type\":\"listen\",\"state\":\"start\"";
    if (mode == kListeningModeRealtime) {
        message += ",\"mode\":\"realtime\"";
    } else if (mode == kListeningModeAutoStop) {
        message += ",\"mode\":\"auto\"";
    } else {
        message += ",\"mode\":\"manual\"";
    }
    message += "}";
    SendText(message);
}

/**
 * @brief Tell the server that microphone streaming has stopped.
 * @param None.
 * @return None.
 * @note Called when manual listening ends or the application becomes idle.
 */
void Protocol::SendStopListening() {
    std::string message = "{\"session_id\":\"" + session_id_ + "\",\"type\":\"listen\",\"state\":\"stop\"}";
    SendText(message);
}

/**
 * @brief Wrap an MCP payload in the active session envelope.
 * @param payload Serialized JSON object produced by the MCP server.
 * @return None.
 * @note Called when a local tool result must be sent to the remote service.
 */
void Protocol::SendMcpMessage(const std::string& payload) {
    std::string message = "{\"session_id\":\"" + session_id_ + "\",\"type\":\"mcp\",\"payload\":" + payload + "}";
    SendText(message);
}

/**
 * @brief Detect a stalled channel from the last incoming packet time.
 * @param None.
 * @return true when no packet has arrived for more than 120 seconds.
 * @return false while the channel remains active.
 * @note Called periodically by transports and connection management code.
 */
bool Protocol::IsTimeout() const {
    const int kTimeoutSeconds = 120;
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_incoming_time_);
    bool timeout = duration.count() > kTimeoutSeconds;
    if (timeout) {
        ESP_LOGE(TAG, "Channel timeout %ld seconds", (long)duration.count());
    }
    return timeout;
}
