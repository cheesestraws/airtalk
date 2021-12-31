#ifndef APPS_H
#define APPS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "buffer_pool.h"

/* apps.{c,h} contains the runloop for the "applications"---that is to say,
   things that capture and respond to packets from the LocalTalk side. */

// start_apps starts the runloop for applications and packet filters
void start_apps(buffer_pool_t* packet_pool, QueueHandle_t fromUART, QueueHandle_t toUART, QueueHandle_t toUDP);

// send_fake_nbp_LkUpReply does exactly what it says on the tin.
void send_fake_nbp_LkUpReply(uint8_t node, uint8_t socket, uint8_t enumerator,
	uint8_t nbp_id, char* object, char* type, char* zone);

#endif