#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "hw.h"

#include "led.h"

#define FLASH_LENGTH 125

static const char* TAG = "LED";
void led_runloop(void* param);

/* Configuration for the status LEDs */
typedef struct {
	/* initialise these fields */
	
	int enabled; 
	char* name;
	int gpio_pin;
	int also;
	
	/* don't initialise these */
	
	TaskHandle_t task;
	int gpio_state;
} led_config_t;

static led_config_t led_config[] = {
	[WIFI_RED_LED] = {
		.enabled = 1,
		.name = "WIFI_RED_LED",
		.gpio_pin = WIFI_RED_LED_PIN
	},
	
	[WIFI_GREEN_LED] = {
		.enabled = 1,
		.name = "WIFI_GREEN_LED",
		.gpio_pin = WIFI_GREEN_LED_PIN
	},
	
	[UDP_RED_LED] = {
		.enabled = 1,
		.name = "UDP_RED_LED",
		.gpio_pin = UDP_RED_LED_PIN,
		.also = ANY_ERR_LED
	},
	
	[UDP_TX_GREEN] = {
		.enabled = 1,
		.name = "UDP_TX_GREEN",
		.gpio_pin = UDP_TX_GREEN_PIN,
		.also = ANY_ACT_LED
	},
	
	[UDP_RX_GREEN] = {
		.enabled = 1,
		.name = "UDP_RX_GREEN",
		.gpio_pin = UDP_RX_GREEN_PIN,
		.also = ANY_ACT_LED
	},
	
	[LT_RED_LED] = {
		.enabled = 1,
		.name = "LT_RED_LED",
		.gpio_pin = LT_RED_LED_PIN,
		.also = ANY_ERR_LED
	},
	
	[LT_TX_GREEN] = {
		.enabled = 1,
		.name = "LT_TX_GREEN",
		.gpio_pin = LT_TX_GREEN_PIN,
		.also = ANY_ACT_LED
	},
	
	[LT_RX_GREEN] = {
		.enabled = 1,
		.name = "LT_RX_GREEN",
		.gpio_pin = LT_RX_GREEN_PIN,
		.also = ANY_ACT_LED
	},
	
	[OH_NO_LED] = {
		.enabled = 1,
		.name = "OH_NO_LED",
		.gpio_pin = OH_NO_LED_PIN
	},
	
	[ANY_ERR_LED] = {
		.enabled = 1,
		.name = "ANY_ERR_LED",
		.gpio_pin = GENERIC_ERR_LED_PIN
	},
	
	[ANY_ACT_LED] = {
		.enabled = 1,
		.name = "ANY_ACT_LED",
		.gpio_pin = GENERIC_ACT_LED_PIN
	},
};

/* led_init sets up gpio pins and starts the background tasks for each LED */
void led_init(void) {
	int num_of_leds = sizeof(led_config) / sizeof(led_config[0]);
	
	/* LEDs with a pin number of -1 should be disabled.  This is a bit
	   manky but makes the hw.h file considerably more readable. */
	for(int i = 0; i < num_of_leds; i++) {
		if (led_config[i].gpio_pin == -1) {
			led_config[i].enabled = 0;
		}
	}
	
	/* set up GPIO pin, light up each LED */
	
	for(int i = 0; i < num_of_leds; i++) {
		if (led_config[i].enabled != 1) {
			continue;
		}
		
		ESP_LOGI(TAG, "doing %s: %d", led_config[i].name, led_config[i].gpio_pin);
		
		gpio_reset_pin(led_config[i].gpio_pin);
		gpio_set_direction(led_config[i].gpio_pin, GPIO_MODE_OUTPUT);
		gpio_set_level(led_config[i].gpio_pin, 1);
		
		ESP_LOGI(TAG, "done %d", led_config[i].gpio_pin);
	}
	
	vTaskDelay(250 / portTICK_PERIOD_MS);
	
	/* Now turn off each LED and start the background task for each LED */
	for(int i = 0; i < num_of_leds; i++) {
		if (led_config[i].enabled != 1) {
			continue;
		}
	
		gpio_set_level(led_config[i].gpio_pin, 0);
		xTaskCreate(led_runloop, led_config[i].name, 1024, (void*)i, tskIDLE_PRIORITY, &led_config[i].task);
	}
}


void led_runloop(void* param) {
	LED led = (LED) param;
	uint32_t dummy_value = 0;
	
	/* We just run in a loop, waiting to be notified, flashing our little
	   light, then going back to sleep again. */
	while(1) {
		xTaskNotifyWait(0, 0, &dummy_value, portMAX_DELAY);
		gpio_set_level(led_config[led].gpio_pin, (led_config[led].gpio_state + 1) % 2);
		vTaskDelay(FLASH_LENGTH / portTICK_PERIOD_MS);
		gpio_set_level(led_config[led].gpio_pin, led_config[led].gpio_state);
	}
}

void turn_led_on(LED led) {
	led_config[led].gpio_state = 1;
	gpio_set_level(led_config[led].gpio_pin, 1);
}

void turn_led_off(LED led) {
	led_config[led].gpio_state = 0;
	gpio_set_level(led_config[led].gpio_pin, 0);
}


void flash_led_once(LED led) {
	int num_of_leds = sizeof(led_config) / sizeof(led_config[0]);
	
	if (led >= num_of_leds || !led_config[led].enabled) {
		ESP_LOGW(TAG, "attempt to flash illicit LED: %d", led);
	}
	
	xTaskNotify(led_config[led].task, 1, eSetValueWithOverwrite);
	
	// do we have a second LED we should also flash?
	if (led_config[led].also) {
		flash_led_once(led_config[led].also);
	}
}

void flash_all_leds_once(void) {
	int num_of_leds = sizeof(led_config) / sizeof(led_config[0]);
	
	for(int i = 0; i < num_of_leds; i++) {
		if (led_config[i].enabled != 1) {
			continue;
		}
		
		flash_led_once(i);
	}
}

void turn_all_leds_on(void) {
	int num_of_leds = sizeof(led_config) / sizeof(led_config[0]);
	
	for(int i = 0; i < num_of_leds; i++) {
		if (led_config[i].enabled != 1) {
			continue;
		}
		
		turn_led_on(i);
	}
}

