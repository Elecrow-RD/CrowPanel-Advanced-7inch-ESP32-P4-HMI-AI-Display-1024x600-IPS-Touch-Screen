#include "weather.h"

#include <esp_http_client.h>
#include <cJSON.h>

#define TAG "WeatherC"

// ---------------------- Constructor / Destructor ----------------------

weather_t* weather_create(void)
{
    weather_t* weather = (weather_t*)malloc(sizeof(weather_t));
    if (weather == NULL) {
        ESP_LOGE(TAG, "Failed to allocate weather_t");
        return NULL;
    }

    // Replacement for new char[] in C++ constructor
    weather->json_response = (char*)malloc(JSON_BUFFER_SIZE);
    if (weather->json_response == NULL) {
        ESP_LOGE(TAG, "Failed to allocate json_response buffer");
        free(weather);
        return NULL;
    }
    
    // Initialize buffer
    memset(weather->json_response, 0, JSON_BUFFER_SIZE);
    return weather;
}

void weather_destroy(weather_t* weather)
{
    if (weather) {
        // Replacement for delete[] in C++ destructor
        if (weather->json_response) {
            free(weather->json_response);
        }
        free(weather); // Free the structure itself
    }
}

// ---------------------- Internal implementation functions ----------------------

/**
 * @brief HTTP response callback function (keep C-style signature)
 * @param evt HTTP event structure
 * @return esp_err_t 
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    // Key point: get weather_t instance pointer from evt->user_data
    weather_t *weather_inst = (weather_t*)evt->user_data;
    if (weather_inst == NULL || weather_inst->json_response == NULL) {
        return ESP_FAIL;
    }
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Copy received data into buffer
            if (evt->data_len < JSON_BUFFER_SIZE - strlen(weather_inst->json_response) - 1) {
                // Use strncat to safely concatenate strings
                strncat(weather_inst->json_response, (char*)evt->data, evt->data_len);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief Send HTTP request and get JSON
 * @param weather Instance pointer (replacement for this)
 * @return bool 
 */
static bool weather_http_get_json(weather_t* weather)
{
    // Clear buffer
    memset(weather->json_response, 0, JSON_BUFFER_SIZE); 
    
    // Note: In C language, struct initialization usually requires explicitly
    // specifying all members, or using {0} for initialization
    esp_http_client_config_t config = {
        .url = WEATHER_JSON_URL,
        .event_handler = http_event_handler, // Use C-style function
        .user_data = weather,  // Pass current weather_t instance pointer
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    bool success = false;
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP status code: %d", esp_http_client_get_status_code(client));
        ESP_LOGI(TAG, "Received JSON response:\n%s", weather->json_response);
        success = true;
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    // Clean up resources
    esp_http_client_cleanup(client);
    return success;
}

/**
 * @brief Parse temperature and weather condition from JSON string
 * @param weather Instance pointer (replacement for this)
 * @param json_str JSON string
 * @param temp_c Temperature output pointer
 * @param weather_text Weather description output buffer
 * @param timestamp Timestamp output pointer
 * @return bool 
 */
static bool weather_analyse_weather_json(weather_t *weather, const char *json_str, double *temp_c, char* const weather_text, int *timestamp)
{
    // C language does not require the first weather parameter,
    // but it is kept for method consistency and not actually used
    (void)weather; // Avoid unused variable warning

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "JSON parse failed: %s", cJSON_GetErrorPtr());
        return false;
    }

    // 2.1 Get data node
    cJSON *data_node = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (!cJSON_IsObject(data_node)) {
        ESP_LOGE(TAG, "Valid data node not found");
        cJSON_Delete(root);
        return false;
    }

    // Extract temp_c
    cJSON *temp_c_node = cJSON_GetObjectItemCaseSensitive(data_node, "temp");
    if (!cJSON_IsNumber(temp_c_node)) {
        ESP_LOGE(TAG, "Valid temp node not found");
        cJSON_Delete(root);
        return false;
    }
    *temp_c = temp_c_node->valuedouble;

    // Extract weather
    cJSON *weather_node = cJSON_GetObjectItemCaseSensitive(data_node, "weather");
    if (!cJSON_IsString(weather_node) || weather_node->valuestring == NULL) {
        ESP_LOGE(TAG, "Valid condition.text node not found");
        cJSON_Delete(root);
        return false;
    } 
    // Use strncpy instead of strcpy to safely copy string
    strncpy(weather_text, weather_node->valuestring, strlen(weather_node->valuestring) + 1);

    // Extract timestamp
    cJSON *timestamp_node = cJSON_GetObjectItemCaseSensitive(data_node, "timestamp");
    if (!cJSON_IsNumber(timestamp_node)) {
        ESP_LOGE(TAG, "Valid timestamp node not found");
        cJSON_Delete(root);
        return false;
    }
    *timestamp = timestamp_node->valueint;

    // Output result (omitted)

    // Free memory (critical)
    cJSON_Delete(root);
    return true;
}


// ---------------------- External API function ----------------------

bool weather_get_weather(weather_t* weather, double *temp_c, char *weather_text, int *timestamp)
{
    if (weather == NULL) {
        ESP_LOGE(TAG, "Weather instance is NULL");
        return false;
    }
    
    // Send HTTP request to get JSON
    if (false == weather_http_get_json(weather)) {
        return false;
    }
    // Parse JSON to get temperature and weather condition
    if (false == weather_analyse_weather_json(weather, weather->json_response, temp_c, weather_text, timestamp)) {
        return false;
    }
    return true;
}
