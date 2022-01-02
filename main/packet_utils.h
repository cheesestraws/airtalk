#ifndef PACKET_UTILS_H
#define PACKET_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#include "packet_types.h"

/* packet_utils.{c,h} include utilities for peering into AppleTalk packets
   to work out what they are. */
   
/* LLAP */

int llap_type(llap_packet* packet);
int llap_destination_node(llap_packet* packet);
   
/* DDP */
   
// is_ddp_packet returns true if an LLAP packet is a DDP packet, false if not.
bool is_ddp_packet(llap_packet* packet);

// ddp_has_long_headers returns true if the packet is in the long header format
bool ddp_has_long_header(llap_packet* packet);

// ddp_type returns the ddp type of the packet
uint8_t ddp_type(llap_packet* packet);

uint8_t ddp_destination_node(llap_packet* packet);

uint8_t ddp_destination_socket(llap_packet* packet);

int ddp_payload_offset(llap_packet* packet);


/* NBP */

#define NBP_LKUP 2

// an nbp_tuple is a tuple of pointers to *Pascal strings* inside an NBP
// packet.  Its lifetime is tied to that of its parent packet.  Do not use it
// after its corresponding packet has been relinquished or freed.
typedef struct {
	bool ok;

	uint16_t network;
	uint8_t node;
	uint8_t socket;
	uint8_t enumerator;
	
	unsigned char* object;
	unsigned char* type;
	unsigned char* zone;
} nbp_tuple_t;

bool is_nbp_packet(llap_packet* packet);
int nbp_function_code(llap_packet* packet);
int nbp_tuple_count(llap_packet* packet);
int nbp_id(llap_packet* packet);
nbp_tuple_t nbp_tuple(llap_packet* packet, int tuple);

/* ATP */

#define ATP_TREQ 1
#define ATP_TRESP 2
#define ATP_TREL 3

bool is_atp_packet(llap_packet* packet);

int atp_function_code(llap_packet* packet);

int atp_payload_offset(llap_packet* packet);

#endif
