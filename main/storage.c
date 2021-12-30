#include "storage.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "led.h"

static const char* TAG = "storage";

void storage_init() {
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err) {
    	turn_led_on(OH_NO_LED);
    }
}

void store_wifi_details(char* ssid, char* pwd) {
	esp_err_t err;
	nvs_handle_t h;
	
    err = nvs_open("wifi", NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "couldn't write NVS: %s", esp_err_to_name(err));
        turn_led_on(OH_NO_LED);
        return;
    }
    
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "pwd", pwd);
    
    nvs_commit(h);
    nvs_close(h);
}

void get_wifi_details(char* ssid, size_t ssid_len, char* pwd, size_t pwd_len) {
	esp_err_t err;
	nvs_handle_t h;
	
    err = nvs_open("wifi", NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "couldn't write NVS: %s", esp_err_to_name(err));
        turn_led_on(OH_NO_LED);
        return;
    }
    
    nvs_get_str(h, "ssid", ssid, &ssid_len);
    nvs_get_str(h, "pwd", pwd, &pwd_len);
    
    nvs_close(h);
}