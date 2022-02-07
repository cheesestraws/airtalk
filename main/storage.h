#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

void storage_init();
void store_wifi_details(char* ssid, char* pwd);
void get_wifi_details(char* ssid, size_t ssid_len, char* pwd, size_t pwd_len);

void store_recovery_for_next_boot(void);
void clear_recovery(void);
bool get_recovery(void);

#endif
