#ifndef LED_H
#define LED_H

typedef enum {
	WIFI_RED_LED, WIFI_GREEN_LED,
	UDP_RED_LED, UDP_TX_GREEN, UDP_RX_GREEN,
	LT_RED_LED, LT_TX_GREEN, LT_RX_GREEN,
	OH_NO_LED,
} LED;

void led_init(void);
void flash_led_once(LED led);
void flash_all_leds_once(void);
void turn_led_on(LED led);
void turn_led_off(LED led);

#endif