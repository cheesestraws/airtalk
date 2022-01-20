#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "hw.h"
#include "led.h"

#include "recovery_listener.h"

const static char* TAG = "recovery_listener";

void recovery_listener_runloop(void* param) {
	int last_recovery = 1;
	int recovery;

	while(1) {
		// This is a hack but it's simple and unintrusive.
		vTaskDelay(800 / portTICK_PERIOD_MS);
		
		recovery = gpio_get_level(RECOVER_BUTTON);
		if (recovery == 0 && last_recovery == 0) {
			// start recovery mode
			turn_led_on(OH_NO_LED);
		}
		
		last_recovery = recovery;
	}

}

void start_recovery_listener(void) {
	gpio_reset_pin(RECOVER_BUTTON);
	gpio_set_direction(RECOVER_BUTTON, GPIO_MODE_INPUT);
	
	xTaskCreate(recovery_listener_runloop, "recovery", 1024, NULL, tskIDLE_PRIORITY, NULL);
}