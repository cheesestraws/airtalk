#include "packet_utils.h"

#define NUM_LEN_GUARD(x) if (packet->length <= (x)) { return 0; }
#define BOOL_LEN_GUARD(x) if (packet->length <= (x)) { return false; }


/* DDP */

#define LLAP_TYPE_OFFSET 2
#define LLAP_PAYLOAD_OFFSET 3

bool is_ddp_packet(llap_packet* packet) {
	// packets of length 3 or less can't be DDP packets
	if (packet->length <= 3) {
		return false;
	}
	
	
	if (packet->packet[LLAP_TYPE_OFFSET] == 1 ||
		packet->packet[LLAP_TYPE_OFFSET] == 2) {
	 
	 	return true;   
	}
	
	return false;
}

bool ddp_has_long_header(llap_packet* packet) {
	return packet->packet[LLAP_TYPE_OFFSET] == 2;
}

uint8_t ddp_type(llap_packet* packet) {
	if (ddp_has_long_header(packet)) {
		NUM_LEN_GUARD(LLAP_PAYLOAD_OFFSET + 12);
		return packet->packet[LLAP_PAYLOAD_OFFSET + 12];
	} else {
		NUM_LEN_GUARD(LLAP_PAYLOAD_OFFSET + 4);
		return packet->packet[LLAP_PAYLOAD_OFFSET + 4];
	}
}

uint8_t ddp_destination_node(llap_packet* packet) {
	if (ddp_has_long_header(packet)) {
		NUM_LEN_GUARD(LLAP_PAYLOAD_OFFSET + 8);
		return packet->packet[LLAP_PAYLOAD_OFFSET + 8];
	} else {
		return packet->packet[0];
	}
}

uint8_t ddp_destination_socket(llap_packet* packet) {
	if (ddp_has_long_header(packet)) {
		NUM_LEN_GUARD(LLAP_PAYLOAD_OFFSET + 10);
		return packet->packet[LLAP_PAYLOAD_OFFSET + 10];
	} else {
		NUM_LEN_GUARD(LLAP_PAYLOAD_OFFSET + 4);
		return packet->packet[LLAP_PAYLOAD_OFFSET + 2];
	}
}


int ddp_payload_offset(llap_packet* packet) {
	if (ddp_has_long_header(packet)) {
		return LLAP_PAYLOAD_OFFSET + 13;
	} else {
		return LLAP_PAYLOAD_OFFSET + 5;
	}
}


/* NBP */

bool is_nbp_packet(llap_packet* packet) {
	return is_ddp_packet(packet) && (ddp_type(packet) == 2);
}


/* ATP */

bool is_atp_packet(llap_packet* packet) {
	return is_ddp_packet(packet) && (ddp_type(packet) == 3);
}

int atp_function_code(llap_packet* packet) {
	int p = ddp_payload_offset(packet);
	
	NUM_LEN_GUARD(p);

	// Top two bits of the first byte of the packet
	return (packet->packet[p] & 0xC0) >> 6;
}

int atp_payload_offset(llap_packet* packet) {
	return ddp_payload_offset(packet) + 8;
}
