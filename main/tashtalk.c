#include <strings.h>
#include <stdbool.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "tashtalk.h"
#include "packet_types.h"
#include "crc.h"
#include "led.h"

static const char* TAG = "tashtalk";

tashtalk_rx_state_t* new_tashtalk_rx_state(buffer_pool_t* buffer_pool, 
		QueueHandle_t output_queue) {

	tashtalk_rx_state_t* ns = malloc(sizeof(tashtalk_rx_state_t));
	
	if (ns == NULL) {
		return NULL;
	}
	
	bzero(ns, sizeof(tashtalk_rx_state_t));
	
	ns->buffer_pool = buffer_pool;
	ns->output_queue = output_queue;
	
	return ns;
}

void append(llap_packet* packet, unsigned char byte) {
	if (packet->length < 605) {
		packet->packet[packet->length] = byte;
		packet->length++;
	} else {
		ESP_LOGE(TAG, "buffer overflow in packet");
	}
}

void do_something_sensible_with_packet(tashtalk_rx_state_t* state) {
   	flash_led_once(LT_RX_GREEN);
	xQueueSendToBack(state->output_queue, &state->packet_in_progress, portMAX_DELAY);
}

void feed(tashtalk_rx_state_t* state, unsigned char byte) {

	// do we have a buffer?
	if (state->packet_in_progress == NULL) {
		state->packet_in_progress = bp_fetch(state->buffer_pool);
		crc_state_init(&state->crc);
	}
	
	// buggered if I know what else to do here other than go in circles
	while (state->packet_in_progress == NULL) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
		state->packet_in_progress = bp_fetch(state->buffer_pool);
		crc_state_init(&state->crc);
		ESP_LOGE(TAG, "around and around and around we go");
	}
	
	// Are we in an escape state?
	if (state->in_escape) {
		switch(byte) {
			case 0xFF:
				// 0x00 0xFF is a literal 0x00 byte
				append(state->packet_in_progress, 0x00);
				crc_state_append(&state->crc, 0x00);
				break;
				
			case 0xFD:
				// 0x00 0xFD is a complete frame
				if (!crc_state_ok(&state->crc)) {
					ESP_LOGE(TAG, "/!\\ CRC fail: %d", state->crc);
					flash_led_once(LT_RED_LED);
				}
				
				do_something_sensible_with_packet(state);
				state->packet_in_progress = NULL;
				break;
				
			case 0xFE:
				// 0x00 0xFD is a complete frame
				ESP_LOGI(TAG, "framing error of %d bytes", 
					state->packet_in_progress->length);
				flash_led_once(LT_RED_LED);
				
				bp_relinquish(state->buffer_pool, (void**) &state->packet_in_progress);
				state->packet_in_progress = NULL;
				break;

			case 0xFA:
				// 0x00 0xFD is a complete frame
				ESP_LOGI(TAG, "frame abort of %d bytes", 
					state->packet_in_progress->length);
				flash_led_once(LT_RED_LED);
				
				bp_relinquish(state->buffer_pool, (void**) &state->packet_in_progress);
				state->packet_in_progress = NULL;
				break;			
		}
		
		state->in_escape = false;
	} else if (byte == 0x00) {
		state->in_escape = true;
	} else {
		append(state->packet_in_progress, byte);
		crc_state_append(&state->crc, byte);
	}
}

void feed_all(tashtalk_rx_state_t* state, unsigned char* buf, int count) {
	for (int i = 0; i < count; i++) {
		feed(state, buf[i]);
	}
}

bool tashtalk_tx_validate(llap_packet* packet) {
	if (packet->length < 5) {
		// Too short
		ESP_LOGE(TAG, "tx packet too short");
		return false;
	}
	
	if (packet->length == 5 && !(packet->packet[2] & 0x80)) {
		// 3 byte packet is not a control packet
		ESP_LOGE(TAG, "tx 3-byte non-control packet, wut?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	if ((packet->packet[2] & 0x80) && packet->length != 5) {
		// too long control frame
		ESP_LOGE(TAG, "tx too-long control packet, wut?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	if (packet->length == 6) {
		// impossible packet length
		ESP_LOGE(TAG, "tx impossible packet length, wut?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	if (packet->length >= 7 && (((packet->packet[3] & 0x3) << 8) | packet->packet[4]) != packet->length - 5) {
		// packet length does not match claimed length
		ESP_LOGE(TAG, "tx length field (%d) does not match actual packet length (%d)", (((packet->packet[3] & 0x3) << 8) | packet->packet[4]), packet->length - 5);
		flash_led_once(LT_RED_LED);
		return false;
	}

	// check the CRC
	crc_state_t crc;
	crc_state_init(&crc);
	crc_state_append_all(&crc, packet->packet, packet->length);
	if (!crc_state_ok(&crc)) {
		ESP_LOGE(TAG, "bad CRC on tx: IP bug?");
		flash_led_once(LT_RED_LED);
		return false;
	}
	
	return true;
}