#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include "esp_netif.h"

void init_at_wifi(void);
esp_netif_t* get_active_net_intf(void);
bool is_net_if_ready(void);

#endif