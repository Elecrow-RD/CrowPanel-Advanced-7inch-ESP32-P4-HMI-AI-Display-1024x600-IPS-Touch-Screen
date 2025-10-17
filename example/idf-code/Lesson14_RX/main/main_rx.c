/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
// Include the main header file which contains all necessary definitions and function declarations
#include "include/main.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
// Define global static variables used throughout the file
static esp_ldo_channel_handle_t ldo4 = NULL;    // LDO channel handle for channel 4 (3.3V power control)
static esp_ldo_channel_handle_t ldo3 = NULL;    // LDO channel handle for channel 3 (2.5V power control)
static lv_obj_t *s_rx_label = NULL;             // LVGL label object to display received data
static lv_obj_t *s_rssi_label = NULL;           // LVGL label object to display RSSI value
static lv_obj_t *s_snr_label = NULL;            // LVGL label object to display SNR value
static uint32_t rx_packet_count = 0;            // Counter for the number of received LoRa packets
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

/**
 * @brief Callback function triggered when LoRa data is received
 */
static void rx_data_callback(const char* data, size_t len, float rssi, float snr)
{
    rx_packet_count++;  // Increment the received packet count each time data is received
    
    // (Update LVGL screen display)
    if (lvgl_port_lock(0) == true) {  // Acquire LVGL lock before updating the UI to ensure thread safety
        // (Format received data as RX_Hello World:i)
        if (s_rx_label != NULL) {
            char rx_text[64];  // Buffer to store formatted text
            snprintf(rx_text, sizeof(rx_text), "RX_Hello World:%lu", (unsigned long)rx_packet_count);
            lv_label_set_text(s_rx_label, rx_text);  // Update the text of the RX label
        }
        
        //  (Update RSSI display)
        if (s_rssi_label != NULL) {
            char rssi_text[32];  // Buffer to store formatted RSSI text
            snprintf(rssi_text, sizeof(rssi_text), "RSSI: %.1f dBm", rssi);
            lv_label_set_text(s_rssi_label, rssi_text);  // Update the RSSI label text
        }
        
        //  (Update SNR display)
        if (s_snr_label != NULL) {
            char snr_text[32];  // Buffer to store formatted SNR text
            snprintf(snr_text, sizeof(snr_text), "SNR: %.1f dB", snr);
            lv_label_set_text(s_snr_label, snr_text);  // Update the SNR label text
        }
        
        lvgl_port_unlock();  // Release LVGL lock after updating the UI
    }
    
    char rx_display_text[64];  // Local buffer for logging display
    snprintf(rx_display_text, sizeof(rx_display_text), "RX_Hello World:%lu", (unsigned long)rx_packet_count);
    MAIN_INFO("RX: %s (RSSI: %.1f dBm, SNR: %.1f dB)", rx_display_text, rssi, snr);  // Log received data info to console
}

/**
 * @brief Initialize the LVGL display interface for LoRa RX
 */
static void lvgl_show_rx_interface_init(void)
{
    if (lvgl_port_lock(0) != true) {  // Try to lock LVGL before creating objects
        MAIN_ERROR("LVGL lock failed");  // Log error if lock acquisition fails
        return;  // Exit the function
    }

    lv_obj_t *screen = lv_scr_act();  // Get the current active LVGL screen object
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);  // Set the screen background color to white
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);      // Set background opacity to fully opaque

    // Create a common text style for RSSI and SNR labels
    static lv_style_t info_style;  
    lv_style_init(&info_style);  // Initialize LVGL style object
    lv_style_set_text_font(&info_style, &lv_font_montserrat_42);  // Set font size
    lv_style_set_text_color(&info_style, lv_color_black());       // Set text color to black
    lv_style_set_bg_opa(&info_style, LV_OPA_TRANSP);              // Set transparent background

    // (Create the title label)
    lv_obj_t *title_label = lv_label_create(screen);  // Create a new LVGL label for the title
    if (title_label != NULL) {
        lv_label_set_text(title_label, "LoRa RX Receiver");  // Set the title text
        static lv_style_t title_style;  // Define a separate style for the title
        lv_style_init(&title_style);  
        lv_style_set_text_font(&title_style, &lv_font_montserrat_42);  // Use large font for title
        lv_style_set_text_color(&title_style, lv_color_black());       // Set text color
        lv_style_set_bg_opa(&title_style, LV_OPA_TRANSP);              // Make background transparent
        lv_obj_add_style(title_label, &title_style, LV_PART_MAIN);     // Apply the style to title label
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);            // Align title to top center with Y offset
    }

    //  (Create label to show received message)
    s_rx_label = lv_label_create(screen);  // Create label object for RX message display
    if (s_rx_label != NULL) {
        lv_label_set_text(s_rx_label, "RX_Hello World:0");  // Set initial text content
        static lv_style_t rx_style;  // Create style for RX label
        lv_style_init(&rx_style);
        lv_style_set_text_font(&rx_style, &lv_font_montserrat_42);  // Font for RX text
        lv_style_set_text_color(&rx_style, lv_color_black());       // Black text color
        lv_style_set_bg_opa(&rx_style, LV_OPA_TRANSP);              // Transparent background
        lv_obj_add_style(s_rx_label, &rx_style, LV_PART_MAIN);      // Apply RX style
        lv_obj_align(s_rx_label, LV_ALIGN_CENTER, 0, -40);          // Align label near center
    }

    //  (Create RSSI display label)
    s_rssi_label = lv_label_create(screen);
    if (s_rssi_label != NULL) {
        lv_label_set_text(s_rssi_label, "RSSI: -- dBm");            // Initial RSSI text
        lv_obj_add_style(s_rssi_label, &info_style, LV_PART_MAIN);  // Apply common style
        lv_obj_align(s_rssi_label, LV_ALIGN_CENTER, -180, 150);     // Align to bottom-left area
    }

    //  (Create SNR display label with same style)
    s_snr_label = lv_label_create(screen);
    if (s_snr_label != NULL) {
        lv_label_set_text(s_snr_label, "SNR: -- dB");               // Initial SNR text
        lv_obj_add_style(s_snr_label, &info_style, LV_PART_MAIN);   // Apply shared style
        lv_obj_align(s_snr_label, LV_ALIGN_CENTER, 180, 150);       // Align to bottom-right area
    }

    lvgl_port_unlock();  // Unlock LVGL after all UI components are created
}

/**
 * @brief Handle initialization failure by printing error and halting
 */
static void init_or_halt(const char *name, esp_err_t err)
{
    static bool printed = false;  // Ensure error message is printed only once
    while (err != ESP_OK) {       // Keep looping if initialization fails
        if (!printed) {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));  // Print initialization error
            printed = true;  // Mark as printed
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait 1 second before retry (simulate halt)
    }
}

/**
 * @brief Initialize hardware peripherals: power, LCD, and wireless module
 */
static void Hardware_Init(void)
{
    esp_err_t err = ESP_OK;  // Variable to store error codes

    // Configure LDO3 channel (2.5V output)
    esp_ldo_channel_config_t ldo3_cfg = {
        .chan_id = 3,          // LDO channel 3
        .voltage_mv = 2500,    // Output voltage 2.5V
    };
    err = esp_ldo_acquire_channel(&ldo3_cfg, &ldo3);  // Request LDO channel 3
    init_or_halt("ldo3", err);  // Halt if failed

    // Configure LDO4 channel (3.3V output)
    esp_ldo_channel_config_t ldo4_cfg = {
        .chan_id = 4,          // LDO channel 4
        .voltage_mv = 3300,    // Output voltage 3.3V
    };
    err = esp_ldo_acquire_channel(&ldo4_cfg, &ldo4);  // Request LDO channel 4
    init_or_halt("ldo4", err);  // Halt if failed

    // 1. Initialize LCD hardware and LVGL (important: must init before enabling backlight)
    err = display_init();  // Initialize LCD display
    if (err != ESP_OK) {
        init_or_halt("LCD", err);  // Halt if LCD init fails
    }
    MAIN_INFO("LCD init success");  // Log LCD initialization success

    // 2. Turn on LCD backlight (brightness set to 100 = maximum)
    err = set_lcd_blight(100);  // Set LCD backlight brightness to 100%
    if (err != ESP_OK) {
        init_or_halt("LCD Backlight", err);  // Halt if failed
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");  // Log backlight success

    // 3. Wireless RX init (LoRa receiver initialization)
    err = sx1262_rx_init();  // Initialize SX1262 LoRa receiver
    if (err != ESP_OK) {
        init_or_halt("Wireless Module RX init...", err);  // Halt if failed
    }
    MAIN_INFO("The wireless module RX initialization was successful.");  // Log success
}

/**
 * @brief LoRa receive task that checks incoming packets continuously
 */
static void lora_rx_task(void *param)
{
    while (1) {  // Infinite loop for continuous checking
        //  (Check if data has been received)
        if (sx1262_is_data_received()) {
            // (Get actual received data length)
            size_t len = sx1262_get_received_len();
            received_lora_pack_radio(len);  // Handle received packet data
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Check every 10ms to reduce CPU usage
    }
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/

/*——————————————————————————————————————————Main function—————————————————————————————————————————*/
void app_main(void)
{
    MAIN_INFO("---------- LoRa RX ----------");  // Print startup header log
    Hardware_Init();  // Initialize all hardware peripherals

    lvgl_show_rx_interface_init();  // Initialize LVGL user interface
    MAIN_INFO("-------- LVGL RX Interface OK ----------");  // Log successful UI init

    //  (Set callback function for received data)
    sx1262_set_rx_callback(rx_data_callback);  // Register LoRa RX callback function
    MAIN_INFO("RX callback registered");  // Log callback registration success

    // (Create LoRa receiving task)
    xTaskCreatePinnedToCore(lora_rx_task, "sx1262_rx", 4096, NULL,
                            configMAX_PRIORITIES - 5, NULL, 1);  // Create FreeRTOS task pinned to core 1

    MAIN_INFO("LoRa RX receiver started, waiting for data...");  // Log start message
}
/*———————————————————————————————————————Main function end———————————————————————————————————————*/
