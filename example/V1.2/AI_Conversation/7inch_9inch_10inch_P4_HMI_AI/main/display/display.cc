#include <esp_log.h>
#include <esp_err.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <font_awesome.h>

#include "display.h"
#include "board.h"
#include "application.h"
#include "audio_codec.h"
#include "settings.h"
#include "assets/lang_config.h"

#define TAG "Display"

/*---------------------------------------------------------------
 * Generic display interface
 * Provide safe logging fallbacks for boards without a graphical display;
 * LCD and OLED implementations override these methods with real UI work.
 *--------------------------------------------------------------*/

/**
 * @brief Construct the generic display interface.
 * @param None.
 * @return A display object with no hardware-specific resources.
 * @note Called by the selected board display implementation.
 */
Display::Display() {
}

/**
 * @brief Release the generic display interface.
 * @param None.
 * @return None.
 * @note Called when a board display object is destroyed.
 */
Display::~Display() {
}

/**
 * @brief Present the current device status through the fallback logger.
 * @param status Null-terminated status text.
 * @return None.
 * @note Called whenever Application changes a visible device state.
 */
void Display::SetStatus(const char* status) {
    ESP_LOGW(TAG, "SetStatus: %s", status);
}

/**
 * @brief Forward a C++ notification string to the common implementation.
 * @param notification Text shown to the user.
 * @param duration_ms Display duration in milliseconds.
 * @return None.
 * @note Called by application code that owns an std::string.
 */
void Display::ShowNotification(const std::string &notification, int duration_ms) {
    ShowNotification(notification.c_str(), duration_ms);
}

/**
 * @brief Present a temporary notification through the fallback logger.
 * @param notification Null-terminated notification text.
 * @param duration_ms Requested display duration in milliseconds.
 * @return None.
 * @note Overridden by graphical displays that support notification widgets.
 */
void Display::ShowNotification(const char* notification, int duration_ms) {
    ESP_LOGW(TAG, "ShowNotification: %s", notification);
}

/**
 * @brief Refresh status-bar information on displays that provide one.
 * @param update_all true to refresh every field rather than changed fields.
 * @return None.
 * @note Called periodically by the application clock event.
 */
void Display::UpdateStatusBar(bool update_all) {
}

/**
 * @brief Present an assistant emotion through the fallback logger.
 * @param emotion Emotion identifier received from the server.
 * @return None.
 * @note Called when a JSON emotion message arrives.
 */
void Display::SetEmotion(const char* emotion) {
    ESP_LOGW(TAG, "SetEmotion: %s", emotion);
}

/**
 * @brief Present one conversation message through the fallback logger.
 * @param role Message source such as user or assistant.
 * @param content Message text.
 * @return None.
 * @note Called for incoming and outgoing transcript updates.
 */
void Display::SetChatMessage(const char* role, const char* content) {
    ESP_LOGW(TAG, "Role:%s", role);
    ESP_LOGW(TAG, "     %s", content);
}

/**
 * @brief Select and persist the active display theme.
 * @param theme Theme object that remains valid after this call.
 * @return None.
 * @note Called when the user or configuration changes the UI theme.
 */
void Display::SetTheme(Theme* theme) {
    current_theme_ = theme;
    Settings settings("display", true);
    settings.SetString("theme", theme->name());
}

/**
 * @brief Request display power-saving behavior through the fallback logger.
 * @param on true to enter power saving, false to resume normal display.
 * @return None.
 * @note Overridden by displays that can control panel power.
 */
void Display::SetPowerSaveMode(bool on) {
    ESP_LOGW(TAG, "SetPowerSaveMode: %d", on);
}
