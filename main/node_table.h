#ifndef NODE_TABLE_H
#define NODE_TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/*	This file, and its corresponding C file, manage a node freshness table.
	This table consists of a set of pairs mapping an LocalTalk node ID to
	the time it was last seen transmitting.

	We need to know when a node was last transmitting so we can make a reasonable
	guess as to whether it's alive or not.  If it's alive, we need to tell
	TashTalk to proxy LocalTalk control packets (CTS/RTS) for it, because if
	we try to tunnel those over UDP, the time guarantees can't be met. (Also, 
	the LToUDP protocol doesn't let us).

	The node_table_t type maintains this table. */
typedef struct {
	time_t last_seen[256];
	bool seen_at_all[256];
} node_table_t;


/*	The node_update_packet_t type contains a serialised form of a node table
	to be sent to tashtalk.*/
typedef struct {
	uint8_t nodebits[32];
} node_update_packet_t;


// touch marks a node as alive
void nt_touch(node_table_t* table, uint8_t node);

// fresh returns true if a node last checked in (using nt_touch) less than
// cutoff seconds ago.
// You can pronounce this "minty fresh" if you want to.
bool nt_fresh(node_table_t* table, uint8_t node, time_t cutoff);

// serialise turns a node_table_t into a node_update_packet_t, for sending to
// tashtalk.  It only includes nodes newer than cutoff seconds old.
// Returns true if changes were made to the node_update_packet_t, or false
// if no changes were made.
bool nt_serialise(node_table_t* table, node_update_packet_t* packet, time_t cutoff);


#endif