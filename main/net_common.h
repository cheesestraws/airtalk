#pragma once

#include <stdbool.h>

#include <esp_netif_types.h>

extern esp_netif_t* active_net_if;
extern bool net_if_ready;

char *generate_hostname(void);