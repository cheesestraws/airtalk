#ifndef SCAN_MANAGER_H
#define SCAN_MANAGER_H

void scan_blocking(void);
void scan_nonblocking(void);
void start_scan_manager(void);

void scan_and_send_results(uint8_t node, uint8_t socket, uint8_t nbp_id);

#endif

