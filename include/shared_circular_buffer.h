#ifndef _SHARED_CIRCULAR_BUFFER_H
#define _SHARED_CIRCULAR_BUFFER_H

#include <pthread.h>
#include <stdbool.h>

#include "networking.h"

typedef enum State {CLIENT = 0, WORKER = 1} State;

typedef struct cbuf_info {
	socket_t client_socket;
	State type;
} cbuf_info;

typedef struct circular_buff_t {
	cbuf_info *infos;
	pthread_cond_t empty;
	pthread_cond_t full;
	pthread_mutex_t mutex;
	size_t head;
	size_t tail;
	size_t max_capacity;
} circular_buff_t;


circular_buff_t *circular_buff_init(size_t max_capacity);

void circular_buff_free(circular_buff_t *cbuf);

void circular_buff_push(circular_buff_t *cbuf, socket_t client_socket, State type);

cbuf_info *circular_buff_pop(circular_buff_t *cbuf);

bool circular_buff_empty(circular_buff_t *cbuf);

bool circular_buff_full(circular_buff_t *cbuf);

size_t circular_buff_max_capacity(circular_buff_t *cbuf);

size_t circular_buff_curr_size(circular_buff_t *cbuf);

#endif // SHARED_CIRCULAR_BUFFER_H