#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "recovery.h"
#include "html_page.h"

const static char* TAG = "recovery-app";

static esp_netif_t *ap_if;
static httpd_handle_t srv;

void init_recovery_wifi(void) {
	char myssid[33] = {0};
	uint8_t mymac[6] = {0};
	
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ap_if = esp_netif_create_default_wifi_ap();
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	
	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_AP, mymac));
	sprintf(myssid, "AirTalk-%02x%02x%02x%02x%02x%02x", mymac[0], mymac[1], mymac[2],
		mymac[3], mymac[4], mymac[5]);
		
	ESP_LOGI(TAG, "starting recovery mode on SSID %s", myssid);


	wifi_config_t wifi_config = {
		.ap = {
			.ssid_len = strlen(myssid),
			.channel = 1,
			.password = "airsetup",
			.max_connection = 2,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};
	
	// not an off-by-one: myssid is null-terminated, but the ssid field in the
	// config has an explicitly set length and no null terminator.
	memcpy(wifi_config.ap.ssid, myssid, 32); 
	
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
       
}

static esp_err_t root_handler(httpd_req_t *req) {
	char* qry;

	// Do we have a query string?
	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		qry = malloc(buf_len);
		
		char ssid[33] = {0};
		char key[65] = {0};
	}

	httpd_resp_send(req, details_page, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
};


void start_recovery_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	ESP_ERROR_CHECK(httpd_start(&srv, &config));
	httpd_register_uri_handler(srv, &root);
}