#include <stdbool.h>

#include <esp_system.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_types.h>
#include <esp_eth.h>
#include <driver/gpio.h>

#include "hw.h"
#include "net_common.h"
#include "ethernet.h"

static const char* TAG = "ETHERNET";


void ethernet_init(void) {
#ifdef ETHERNET
	/* set up ESP32 internal MAC */
	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	mac_config.sw_reset_timeout_ms = 1000;

	eth_esp32_emac_config_t emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
	emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
	emac_config.clock_config.rmii.clock_gpio = EMAC_CLK_IN_GPIO;
	emac_config.smi_gpio.mdc_num = ETH_MAC_MDC;
	emac_config.smi_gpio.mdio_num = ETH_MAC_MDIO;
	
	esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_config, &mac_config);

	/* set up PHY */
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.phy_addr = 1;
	phy_config.reset_gpio_num = -1;
	esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);

	// Enable external oscillator (pulled down at boot to allow IO0 strapping)
	ESP_ERROR_CHECK(gpio_set_direction(ETH_50MHZ_EN, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_set_level(ETH_50MHZ_EN, 1));
	
	ESP_LOGD(TAG, "Starting Ethernet interface...");

	// Install and start Ethernet driver
	esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
	esp_eth_handle_t eth_handle = NULL;
	ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

	esp_netif_config_t const netif_config = ESP_NETIF_DEFAULT_ETH();
	esp_netif_t *global_netif = esp_netif_new(&netif_config);
	esp_eth_netif_glue_handle_t eth_netif_glue = esp_eth_new_netif_glue(eth_handle);
	ESP_ERROR_CHECK(esp_netif_attach(global_netif, eth_netif_glue));
	char* hostname = generate_hostname();
	ESP_ERROR_CHECK(esp_netif_set_hostname(global_netif, hostname));
	free(hostname);
	ESP_ERROR_CHECK(esp_eth_start(eth_handle));
	
	active_net_if = global_netif;
	net_if_ready = true;
#endif
}