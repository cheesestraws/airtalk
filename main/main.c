#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "esp_event.h"

#include "hw.h"

#include "packet_types.h"
#include "buffer_pool.h"
#include "led.h"
#include "wifi.h"
#include "ethernet.h"
#include "ip.h"
#include "uart.h"
#include "apps.h"
#include "scan_manager.h"
#include "storage.h"
#include "recovery.h"
#include "recovery_listener.h"

void recovery_main(void) {
	storage_init();
	led_init();
	turn_all_leds_on();

	init_recovery_wifi();
	start_recovery_webserver();

	printf("would enter recovery here");
	while(1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	printf("storage init\n");
	storage_init();
	uint8_t recovery = 0;
	recovery = get_recovery();
	clear_recovery();
	if (recovery) {
		recovery_main();
	}
	

	// A pool of buffers used for LLAP packets/frames on the way through
	buffer_pool_t* packet_pool = new_buffer_pool(180, sizeof(llap_packet));
	
	// some queues
	QueueHandle_t UARTtoApps = xQueueCreate(60, sizeof(llap_packet*));
	QueueHandle_t AppsToUDP  = xQueueCreate(60, sizeof(llap_packet*));
	QueueHandle_t UDPtoUART  = xQueueCreate(60, sizeof(llap_packet*));

	printf("led init\n");
	led_init();
	printf("uart init\n");
	uart_init();
	
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifndef ETHERNET
	printf("wifi init\n");
	init_at_wifi();
#else
	printf("ethernet init\n");
	ethernet_init();
#endif
	
	start_recovery_listener();	
	start_udp(packet_pool, AppsToUDP, UDPtoUART);
	start_scan_manager();
	start_apps(packet_pool, UARTtoApps, UDPtoUART, AppsToUDP);
	uart_start(packet_pool, UDPtoUART, UARTtoApps);

	while(1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
