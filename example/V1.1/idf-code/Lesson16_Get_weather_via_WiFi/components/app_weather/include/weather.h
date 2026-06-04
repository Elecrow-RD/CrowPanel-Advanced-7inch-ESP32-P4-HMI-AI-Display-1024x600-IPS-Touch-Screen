#ifndef _WEATHER_H
#define _WEATHER_H

#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdlib.h> // For malloc/free
#include <stdbool.h> // For bool type


// Define JSON buffer size and URL
#define JSON_BUFFER_SIZE    256
#define WEATHER_JSON_URL    "http://service.thinknode.cc/api/users/weather"


// “Object” handle in C language
typedef struct {
    char *json_response;  // Buffer to store JSON response
} weather_t;


/**
 * @brief Create and initialize a Weather module instance
 * @return weather_t* Returns a pointer to the instance on success, NULL on failure
 */
weather_t* weather_create(void);

/**
 * @brief Release resources occupied by the Weather module instance
 * @param weather Pointer to the instance to be destroyed
 */
void weather_destroy(weather_t* weather);

/**
 * @brief Main function to get weather information
 * @param weather Instance pointer
 * @param temp_c Temperature output pointer (double*)
 * @param weather_text Weather description output buffer (char*)
 * @param timestamp Timestamp output pointer (int*)
 * @return bool Returns true (1) on success, false (0) on failure
 */
bool weather_get_weather(weather_t* weather, double *temp_c, char *weather_text, int *timestamp);

#endif // _WEATHER_H
