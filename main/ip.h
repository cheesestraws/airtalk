#ifndef IP_H
#define IP_H

#include "buffer_pool.h"

void start_udp(buffer_pool_t* pool, QueueHandle_t txQueue, QueueHandle_t rxQueue);

#endif
