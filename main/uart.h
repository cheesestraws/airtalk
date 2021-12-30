#ifndef UART_H
#define UART_H

#include "buffer_pool.h"

void uart_init(void);
void uart_init_tashtalk(void);
void uart_start(buffer_pool_t* packet_pool, QueueHandle_t txQueue, QueueHandle_t rxQueue);

#endif