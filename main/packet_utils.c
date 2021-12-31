#include "packet_utils.h"

#define NUM_LEN_GUARD(x) if (packet->length <= (x)) { return 0; }
#define BOOL_LEN_GUARD(x) if (packet->length <= (x)) { return false; }

bool skip_pstring(llap_packet* packet, int* cursor) {
	BOOL_LEN_GUARD(*cursor);
	*cursor += packet->packet[*cursor] + 1;
	return true;
}

bool read_pstring(llap_packet* packet, int* cursor, unsigned char** str) {
	BOOL_LEN_GUARD(*cursor);
	BOOL_LEN_GUARD(*cursor + packet->packet[*cursor]);
	*str = &packet->packet[*cursor];
	*cursor += packet->packet[*cursor] + 1;
	return true;
}


bool read_uint16(llap_packet* packet, int* cursor, uint16_t* result) {
	BOOL_LEN_GUARD(*cursor + 1);
	*result = ((uint16_t)packet->packet[*cursor] << 8) + ((uint16_t)packet->packet[*cursor+1]);
	*cursor += 2;
	return true;
}

bool read_uint8(llap_packet* packet, int* cursor, uint8_t* result) {
	BOOL_LEN_GUARD(*cursor);
	*result = packet->packet[*cursor];
	*cursor += 1;
	return true;
}


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

int nbp_function_code(llap_packet* packet) {
	int p = ddp_payload_offset(packet);
	
	NUM_LEN_GUARD(p);

	// Top nybble of the first byte of the packet
	return packet->packet[p] >> 4;
}

int nbp_tuple_count(llap_packet* packet) {
	int p = ddp_payload_offset(packet);
	
	NUM_LEN_GUARD(p);

	// Top nybble of the first byte of the packet
	return packet->packet[p] & 0x0F;
}

int nbp_id(llap_packet* packet) {
	int p = ddp_payload_offset(packet);
	NUM_LEN_GUARD(p+1);
	return packet->packet[p+1];
}

nbp_tuple_t nbp_tuple(llap_packet* packet, int tuple) {
	nbp_tuple_t ret = {0};
	bool ran_out_of_packet = false;
	int cursor = ddp_payload_offset(packet) + 2;
	
	// skip tuples until we get to the one we want
	for (int i = 0; i < tuple; i++) {
		// skip network metadata and enumerator
		cursor += 5;
		
		// and skip three pascal strings
		for (int j = 0; j < 3; j++) {
			if (!skip_pstring(packet, &cursor)) {
				ran_out_of_packet = true;
				break;
			}
		}
		
		if (ran_out_of_packet) {
			break;
		}
	}
	
	// At this point either our cursor is pointing at the beginning of the 
	// tuple we want, or we ran out of packet.  If the latter, return an
	// nbp_tuple_t with no ok set.
	if (ran_out_of_packet) {
		return ret;
	}
	
	if (!read_uint16(packet, &cursor, &ret.network)) {
		return ret;
	}
	
	if (!read_uint8(packet, &cursor, &ret.node)) {
		return ret;
	}
	
	if (!read_uint8(packet, &cursor, &ret.socket)) {
		return ret;
	}
	
	if (!read_uint8(packet, &cursor, &ret.enumerator)) {
		return ret;
	}
	
	if (!read_pstring(packet, &cursor, &ret.object)) {
		return ret;
	}
	
	if (!read_pstring(packet, &cursor, &ret.type)) {
		return ret;
	}


	if (!read_pstring(packet, &cursor, &ret.zone)) {
		return ret;
	}
	
	ret.ok = true;
	return ret;
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
