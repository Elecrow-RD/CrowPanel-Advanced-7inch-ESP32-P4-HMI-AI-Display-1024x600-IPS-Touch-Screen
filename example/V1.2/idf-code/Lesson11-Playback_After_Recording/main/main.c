/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"  // Include the main header file containing declarations and macros

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
static void init_or_halt(const char *name, esp_err_t err)  // Function to check initialization result and halt if failed
{
	if (err != ESP_OK)  // If initialization failed
	{
		MAIN_ERROR("%s init failed: %s", name, esp_err_to_name(err));  // Log error message with component name
		while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }  // Enter infinite loop with 1s delay to prevent program from continuing
	}
}

void app_main(void)  // Main entry point for the application
{
	MAIN_INFO("Record 5s and playback original audio");  // Log start message for recording and playback process

	// Audio amplifier and I2S playback initialization
	esp_err_t err = audio_ctrl_init();  // Initialize audio amplifier and I2S playback control
	init_or_halt("audio ctrl", err);  // Check initialization result; halt if failed
	set_Audio_ctrl(false);  // Disable audio amplifier initially
	err = audio_init();  // Initialize audio playback system (I2S configuration, etc.)
	init_or_halt("audio", err);  // Check initialization result; halt if failed

	// Microphone initialization
	err = mic_init();  // Initialize microphone input
	init_or_halt("mic", err);  // Check microphone initialization result; halt if failed

	// Record for 5 seconds and playback
	MAIN_INFO("Start 5s recording...");  // Log recording start message
	err = mic_read_to_audio(5);  // Record 5 seconds of audio and play it back
	if (err != ESP_OK)  // If recording or playback failed
		MAIN_ERROR("record/playback error: %s", esp_err_to_name(err));  // Log error message
	else
		MAIN_INFO("Playback done");  // Log success message when playback finishes

	// Keep task alive
	while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }  // Keep the task alive indefinitely with a 1s delay
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
