#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "net_common.h"
#include "led.h"
#include "storage.h"
#include "scan_manager.h"

static const char* TAG = "APP-WIFI";

bool ssid_configured = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		turn_led_off(WIFI_GREEN_LED);
		turn_led_on(WIFI_RED_LED);
		scan_blocking();
		net_if_ready = false;
		if (ssid_configured) {
			esp_wifi_connect();
		}
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		turn_led_off(WIFI_GREEN_LED);
		turn_led_on(WIFI_RED_LED);
		net_if_ready = false;
		scan_blocking(); 
		scan_blocking(); // Do a scan so we have some info when we next connect
		esp_wifi_connect();
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		turn_led_off(WIFI_RED_LED);
		turn_led_on(WIFI_GREEN_LED);
		net_if_ready = true;
	}
}


void init_at_wifi(void)
{
	active_net_if = esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

	wifi_config_t wifi_config = {0};
	get_wifi_details((char*)wifi_config.sta.ssid, 32, (char*)wifi_config.sta.password, 64);
	ESP_LOGI(TAG,"details from nvs: ssid %s, pwd %s", wifi_config.sta.ssid, wifi_config.sta.password);
	// do we have an ssid?
	if (wifi_config.sta.ssid[0] != '\0') {
		ssid_configured = true;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");
}

esp_netif_t* get_active_net_intf(void) {
	return active_net_if;
}

bool is_net_if_ready(void) {
	return net_if_ready;
}
