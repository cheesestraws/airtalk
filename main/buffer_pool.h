#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* This file and its corresponding .c file contain a thread-safe buffer pool
   implementation.  This is used to avoid having to allocate memory for buffers
   dynamically during the lifetime of the application: instead, all buffers can
   be allocated at the start of the program, which minimises the likelihood
   of nasty memory surprises later. */

typedef struct {
	int buffer_count;
	int buffer_size;
	QueueHandle_t queue;
} buffer_pool_t;

// new_buffer_pool allocates memory for and initialises a new buffer pool.
buffer_pool_t* new_buffer_pool(int buffer_count, int buffer_size);

// bp_fetch returns a zeroed-out buffer from the pool.
void* bp_fetch(buffer_pool_t* bp);

// bp_relinquish returns a buffer to the pool.  There is no need to zero the
// buffer before relinquishing it.
void bp_relinquish(buffer_pool_t* bp, void** buff);

int bp_buffersize(buffer_pool_t* bp);
int bp_available(buffer_pool_t* bp);

#endif
