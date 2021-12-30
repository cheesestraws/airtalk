#include <time.h>

#include "esp_log.h"

#include "node_table.h"

void nt_touch(node_table_t* table, uint8_t node) {
	if (node == 0 || node == 255) {
		return;
	}
	
	table->last_seen[node] = time(NULL);
	table->seen_at_all[node] = true;
}

bool nt_fresh(node_table_t* table, uint8_t node, time_t cutoff) {
	time_t now = time(NULL);
	
	if (!table->seen_at_all[node]) {
		return false;
	}
	
	if (now <= cutoff || table->last_seen[node] > (now - cutoff)) {
		return true;
	}
	
	return false;
}

bool nt_serialise(node_table_t* table, node_update_packet_t* packet, time_t cutoff) {
	time_t now = time(NULL);
	int byte_idx = 0;
	bool changed = false;
	uint8_t oldbyte;
	
	// we iterate through the node ID space bytewise
	for (int i = 0; i < 256; i += 8) {
		// save the old byte so we can check if we changed anything later
		oldbyte = packet->nodebits[byte_idx];
		
		packet->nodebits[byte_idx] = 0;
		for (int j = 0; j < 8; j++) {
			// have we seen the node at all?
			int node = i + j;
						
			if (!table->seen_at_all[node]) {
				continue;
			}
						
			// we need to explicit check as to whether we've not been up
			// longer than cutoff, because our timer starts at 0 when we
			// start up (no battery-backed RTC!)
			if (now <= cutoff || table->last_seen[node] > (now - cutoff)) {
				packet->nodebits[byte_idx] |= (1 << j);
			}		
		}
		
		if (oldbyte != packet->nodebits[byte_idx]) {
			changed = true;
		}
		byte_idx++;
	}
	
	return changed;
}
