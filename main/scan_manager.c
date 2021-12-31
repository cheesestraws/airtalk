#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "led.h"
#include "apps.h"

static const char* TAG = "SCANMAN";

TaskHandle_t scanner_task;

struct {
	SemaphoreHandle_t lock;
	uint16_t ap_count;
	wifi_ap_record_t* aps;
	time_t when;
} scan_state = { 0 };

void scan_blocking(void) {
	esp_err_t err;
	
	err = esp_wifi_scan_start(NULL, true);
	if (err == ESP_ERR_WIFI_STATE) {
		// if we're connecting, nowt we can do about it.
		return;
	}
	
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "error scanning: %s", esp_err_to_name(err));
		return;
	}
	
	// get the AP information
	uint16_t ap_count;
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	// should probably clamp ap_count to some max but meh
	
	wifi_ap_record_t* aps;
	aps = malloc(ap_count * sizeof(wifi_ap_record_t));
	if (aps == NULL) {
		turn_led_on(OH_NO_LED);
		return;
	}
	esp_wifi_scan_get_ap_records(&ap_count, aps);
	
	for (int i = 0; i < ap_count; i++) {
		ESP_LOGI(TAG, "Found AP: %s", aps[i].ssid);
	}
	
	// Now dump them into the scan state, taking the mutex to avoid crashes
	xSemaphoreTake(scan_state.lock, portMAX_DELAY);
	
	// is the old AP state initialised?
	if (scan_state.aps != NULL) {
		free(scan_state.aps);
	}
	
	scan_state.ap_count = ap_count;
	scan_state.aps = aps;
	scan_state.when = time(NULL);
	
	xSemaphoreGive(scan_state.lock);
	
	// do NOT free aps, since we've stashed it in scan_state
}

void scan_nonblocking(void) {
	xTaskNotify(scanner_task, 1, eSetValueWithOverwrite);
}

void scan_manager_runloop(void* param) {
	unsigned int dummy_value = 0;
	
	while(1) {
		xTaskNotifyWait(0, 0, &dummy_value, portMAX_DELAY);
		scan_blocking();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void start_scan_manager(void) {
	scan_state.lock = xSemaphoreCreateMutex();

	xTaskCreate(scan_manager_runloop, "SCANMAN", 4096, NULL, tskIDLE_PRIORITY, &scanner_task);
}

void scan_and_send_results(uint8_t node, uint8_t socket, uint8_t nbp_id) {
	xSemaphoreTake(scan_state.lock, portMAX_DELAY);
	
	time_t now = time(NULL);
	
	// Are the results young enough to send?
	if (scan_state.aps != NULL && scan_state.when > (now - 30)) {
		for (int i = 0; i < scan_state.ap_count; i++) {	
			// we can't deal with ssids longer than 32 characters (?)
			if (strlen((char*)scan_state.aps[i].ssid) > 32) {
				continue;
			}
			
			send_fake_nbp_LkUpReply(node, socket, i, nbp_id,
				(char*)scan_state.aps[i].ssid, "AirTalkAP", "*");
		}
	}
	
	scan_nonblocking();
	
	xSemaphoreGive(scan_state.lock);
}
