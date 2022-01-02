#include <time.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "led.h"
#include "tashtalk.h"
#include "node_table.h"
#include "packet_types.h"
#include "packet_utils.h"

#include "uart.h"

static const char* TAG = "uart";
static const int uart_num = UART_NUM_1;

node_table_t node_table = { 0 };
node_update_packet_t node_table_packet = { 0 };

TaskHandle_t uart_rx_task = NULL;
TaskHandle_t uart_tx_task = NULL;
QueueHandle_t uart_rx_queue = NULL;
QueueHandle_t uart_tx_queue = NULL;



#define RX_BUFFER_SIZE 1024

uint8_t uart_buffer[RX_BUFFER_SIZE];


#define UART_TX GPIO_NUM_14
#define UART_RX GPIO_NUM_27
#define UART_RTS GPIO_NUM_26
#define UART_CTS GPIO_NUM_12

void uart_write_node_table(int uart_num, node_update_packet_t* packet) {
	uart_write_bytes(uart_num, "\x02", 1);
	uart_write_bytes(uart_num, (char*)packet->nodebits, 32);
	
	// debugomatic
	char debugbuff[97] = {0};
	char* ptr = debugbuff;
	for (int i = 0; i < 32; i++) {
		ptr += sprintf(ptr, "%02X ", packet->nodebits[i]);
	}
	ESP_LOGI(TAG, "sent nodebits: %s", debugbuff);
}

node_table_t* get_proxy_nodes() {
	return &node_table;
}

void uart_init_tashtalk(void) {
	char init[1024] = { 0 };
	uart_write_bytes(uart_num, init, 1024);
	uart_write_node_table(uart_num, &node_table_packet);
}

void uart_check_for_tx_wedge(llap_packet* packet) {
	// Sometimes, if buffer weirdness happens, tashtalk gets wedged.  In those
	// cases, we need to detect it.  We can detect this by looking for strings
	// of handshaking requests.
	//
	// This works because we know that we should respond to a CTS for one of our
	// nodes.  If we get more than one burst of CTSes, something's gone wrong
	// and we should reset the tashtalk to a known state.
	
	static int handshake_count = 0;
	if (llap_type(packet) == 0x84 || llap_type(packet) == 0x85) {
		handshake_count++;
		
		if (handshake_count > 10) {
			flash_led_once(LT_RED_LED);
			ESP_LOGE(TAG, "%d consecutive handshakes! reinitialising tashtalk", handshake_count);
			uart_init_tashtalk();
			handshake_count = 0;			
		}
	} else {
		handshake_count = 0;
	}
}

void uart_init(void) {	
	const uart_config_t uart_config = {
		.baud_rate = 1000000,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
		.rx_flow_ctrl_thresh = 0,		
	};
	
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TX, UART_RX, UART_RTS, UART_CTS));
	ESP_ERROR_CHECK(uart_driver_install(uart_num, 2048, 0, 0, NULL, 0));

	uart_init_tashtalk();

	ESP_LOGI(TAG, "uart_init complete");
}

void uart_rx_runloop(void* buffer_pool) {
	static const char *TAG = "UART_RX";	
	ESP_LOGI(TAG, "started");
	
	tashtalk_rx_state_t* rxstate = new_tashtalk_rx_state((buffer_pool_t*)buffer_pool, uart_rx_queue);
	
	while(1){
		/* TODO: this is stupid, read a byte at a time instead and wait for MAX_DELAY */
		const int len = uart_read_bytes(uart_num, uart_buffer, 1, 1000 / portTICK_PERIOD_MS);
		if (len > 0) {
			feed_all(rxstate, uart_buffer, len);
		}
	}
}

void uart_tx_runloop(void* buffer_pool) {
	llap_packet* packet = NULL;
	static const char *TAG = "UART_TX";
	
	ESP_LOGI(TAG, "started");
	
	while(1){
		/* TODO: we should time out here every so often to re-calculate stale
		   nodes in the node table */
		xQueueReceive(uart_tx_queue, &packet, portMAX_DELAY);
		
		// validate the packet: first, check the CRC:
		crc_state_t crc;
		crc_state_init(&crc);
		crc_state_append_all(&crc, packet->packet, packet->length);
		
		if (!tashtalk_tx_validate(packet)) {
			ESP_LOGE(TAG, "packet validation failed");
			flash_led_once(LT_RED_LED);
			goto skip_processing;
		}
		
		// Now mark the sender as active and see if we need to update tashtalk's
		// node mask
		nt_touch(&node_table, packet->packet[1]);
		bool changed = nt_serialise(&node_table, &node_table_packet, 1800);
		if (changed) {
			ESP_LOGI(TAG, "node table changed!");
			uart_write_node_table(uart_num, &node_table_packet);
		}
		
		uart_write_bytes(uart_num, "\x01", 1);
		uart_write_bytes(uart_num, (const char*)packet->packet, packet->length);
		
		flash_led_once(LT_TX_GREEN);
		
skip_processing:
		bp_relinquish((buffer_pool_t*)buffer_pool, (void**)&packet);
	}
}

void uart_start(buffer_pool_t* packet_pool, QueueHandle_t txQueue, QueueHandle_t rxQueue) {
	uart_rx_queue = rxQueue;
	uart_tx_queue = txQueue;
	xTaskCreate(&uart_rx_runloop, "UART_RX", 4096, (void*)packet_pool, 5, &uart_rx_task);
	xTaskCreate(&uart_tx_runloop, "UART_TX", 4096, (void*)packet_pool, 5, &uart_tx_task);
}
