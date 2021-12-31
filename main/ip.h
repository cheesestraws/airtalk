#ifndef IP_H
#define IP_H

#include "buffer_pool.h"

/* ip.{c,h} contains the code to maintain and listen to the LToUDP socket and
   squirt data down it.*/

// start_udp starts the UDP connection.  Packets that are sent to txQueue are
// transmitted; packets that are received are sent to rxQueue.  Packets from
// rxQueue are drawn from the pool; packets in txQueue are returned to the pool
// once sent, so make sure that all packets sent are drawn from the pool.
void start_udp(buffer_pool_t* pool, QueueHandle_t txQueue, QueueHandle_t rxQueue);

#endif
