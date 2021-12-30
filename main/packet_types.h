#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H

#include <stdint.h>

// actually represents an LLAP packet + the FCS
typedef struct {
	int length;
	unsigned char packet[605]; /* 603 max length of LLAP packet: Inside 
	                              Appletalk2nd. Ed. p. 1-6) + 2 bytes FCS. */
} llap_packet;

// represents a return address for a LocalTalk NBP LkUp.  
typedef struct {
	uint8_t node;
	uint8_t socket;
	uint8_t nbp_id;
	uint8_t enumerator;
} nbp_return_t;

#endif