#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include "esp_netif.h"

void init_at_wifi(void);
esp_netif_t* get_wifi_intf(void);
bool is_wifi_ready(void);

#endif