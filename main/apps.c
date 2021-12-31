#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "apps.h"
#include "scan_manager.h"
#include "buffer_pool.h"
#include "packet_types.h"
#include "packet_utils.h"
#include "crc.h"
#include "crc32.h"
#include "pstrings.h"
#include "storage.h"

#define REQUIRE(x) if(!(x)) { return false; }

const char* TAG = "APPS";

QueueHandle_t qFromUART, qToUART, qToUDP;
buffer_pool_t* buffer_pool;

void send_fake_nbp_LkUpReply(uint8_t node, uint8_t socket, uint8_t enumerator,
	uint8_t nbp_id, char* object, char* type, char* zone) {

	llap_packet* packet = bp_fetch(buffer_pool);
	if (packet == NULL) {
		// um I don't know what to do, let's bail out and hope it goes away
		return;
	}
	
	// first of all, let's work out how long this packet is going to be
	uint16_t object_len = strlen(object);
	uint16_t type_len = strlen(type);
	uint16_t zone_len = strlen(zone);
	
	uint16_t tuple_len = 5 + // network information
					  3 + // string length bytes
					  object_len + type_len + zone_len;
					
	uint16_t nbp_packet_len = tuple_len + 2;
	uint16_t ddp_packet_len = nbp_packet_len + 5;
	uint16_t llap_packet_len = ddp_packet_len + 3;
	
	// now start populating the packet.  LLAP headers first:
	packet->length = llap_packet_len;
	packet->packet[0] = node;
	packet->packet[1] = 129; // see if we can get away with this
	packet->packet[2] = 1; // 1 -> DDP, short header.
	
	// DDP headers next:
	packet->packet[3] = 0;
	packet->packet[4] = ddp_packet_len;
		
	packet->packet[5] = socket;
	packet->packet[6] = 2; // source socket number
	packet->packet[7] = 2; // DDP type for NBP
	
	// NBP header:
	packet->packet[8] = 0x31;  // lkup-reply, 1 tuple
	packet->packet[9] = nbp_id;
	
	// NBP tuple.  We fake this based on the crc32 of the object name
	uint32_t my_crc32 = crc32buf(object, object_len);
	
	packet->packet[10] = my_crc32 & 0xFF000000 >> 24; // network, msb
	packet->packet[11] = my_crc32 & 0x00FF0000 >> 16;
	packet->packet[12] = my_crc32 & 0x0000FF00 >> 8; // node ID
	packet->packet[13] = my_crc32 & 0x000000FF; // socket number
	packet->packet[14] = enumerator;
	
	// add names into tuple
	int cursor = 15;
	
	// object
	packet->packet[cursor++] = object_len;
	for (int i = 0; i < object_len; i++) {
		packet->packet[cursor++] = object[i];
	}
	
	// type
	packet->packet[cursor++] = type_len;
	for (int i = 0; i < type_len; i++) {
		packet->packet[cursor++] = type[i];
	}

	// zone
	packet->packet[cursor++] = zone_len;
	for (int i = 0; i < zone_len; i++) {
		packet->packet[cursor++] = zone[i];
	}
	
	// finally, add the crc
	crc_state_t crc;
	crc_state_init(&crc);
	crc_state_append_all(&crc, packet->packet, packet->length);
	
	packet->length += 2;
	packet->packet[cursor++] = crc_state_byte_1(&crc);
	packet->packet[cursor++] = crc_state_byte_2(&crc);
	
	xQueueSendToBack(qToUART, &packet, portMAX_DELAY);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	
	//bp_relinquish(buffer_pool, &packet);
}

bool is_our_nbp_lkup(llap_packet* packet, char* nbptype, nbp_return_t* addr) {
	// First, we check whether this is a broadcast NBP lookup with at least one
	// tuple.
	REQUIRE(is_nbp_packet(packet));
	REQUIRE(ddp_destination_node(packet) == 255);
	REQUIRE(nbp_function_code(packet) == NBP_LKUP);
	REQUIRE(nbp_tuple_count(packet) > 0);
			
	// Now we extract the first tuple and check whether the type is the one
	// our caller wants.
	nbp_tuple_t tuple = nbp_tuple(packet, 0);
	if (!tuple.ok) {
		return false;
	}
		
	// let's check our type string is actually a protocol-compliant length.
	// just in case.
	if (pstrlen(tuple.type) > 32) {
		return false;
	}

	// Now compare.  Note that the tuple contains pascal strings, so we have
	// to convert it.
	char typename[33];
	pstrncpy(typename, tuple.type, 32);
	if (strncmp(typename, nbptype, 33) != 0) {
		return false;
	}
	
	// If we get to this point, we care about this update, so fill in the 
	// return address for the caller.
	addr->node = tuple.node;
	addr->socket = tuple.socket;
	addr->nbp_id = nbp_id(packet);

	return true;
}

bool handle_configuration_packet(llap_packet* packet) {
	// We're looking for an ATP TREQ packet...
	REQUIRE(is_atp_packet(packet));
	REQUIRE(atp_function_code(packet) == ATP_TREQ);
	
	// ... aimed at a broadcast address ...
	REQUIRE(ddp_destination_node(packet) == 255);
	
	// ... with a destination socket of 4 ...
	REQUIRE(ddp_destination_socket(packet) == 4);
	
	unsigned char* payload = &packet->packet[atp_payload_offset(packet)];
	
	// ... with a payload that begins with "airtalk setap" as a pascal string
	if (memcmp(payload + 1, "airtalk setap", 13) != 0) {
		return false;
	}

	// copy stuff out of the payload
	char ssid[64] = {0};
	char passwd[64] = {0};
	pstrncpy(ssid, payload+14, 63);
	pstrncpy(passwd, payload + 14 + pstrlen(payload+14)+1, 63);
	
	ESP_LOGI(TAG, "wifi config request r'd, ssid: %s, pass: %s", ssid, passwd);
	store_wifi_details(ssid, passwd);
	esp_restart();
	
	// we never actually return but C wants us to pretend
	return false;
}

void apps_runloop(void* unused) {
	llap_packet* packet = NULL;
	nbp_return_t nbp_addr;
	
	while(1) {
		if (xQueueReceive(qFromUART, &packet, portMAX_DELAY)) {
			if (is_our_nbp_lkup(packet, "AirTalkAP", &nbp_addr)) {
				ESP_LOGI(TAG, "nbp lkup");
				scan_and_send_results(nbp_addr.node, nbp_addr.socket, nbp_addr.nbp_id);
			}
			
			if (handle_configuration_packet(packet)) {
				bp_relinquish(buffer_pool, (void**)&packet);
				continue;
			}
		
			xQueueSendToBack(qToUDP, &packet, portMAX_DELAY);
		}
	}
}

void start_apps(buffer_pool_t* packet_pool, QueueHandle_t fromUART, 
	QueueHandle_t toUART, QueueHandle_t toUDP) {

	qFromUART = fromUART;
	qToUART = toUART;
	qToUDP = toUDP;
	buffer_pool = packet_pool;
	
	xTaskCreate(&apps_runloop, "APPS", 4096, (void*)packet_pool, 5, NULL);
}
