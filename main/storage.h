#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

void storage_init();
void store_wifi_details(char* ssid, char* pwd);
void get_wifi_details(char* ssid, size_t ssid_len, char* pwd, size_t pwd_len);

#endif
