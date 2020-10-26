#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shared_circular_buffer.h"


static inline size_t cbuf_head_next_index(circular_buff_t *cbuf) {
	size_t index = (cbuf->head + 1) % cbuf->max_capacity;
	return index;
}


static inline size_t cbuf_tail_next_index(circular_buff_t *cbuf) {
	size_t index = (cbuf->tail + 1) % cbuf->max_capacity;
	return index;
}


circular_buff_t *circular_buff_init(size_t max_capacity) {
	assert(max_capacity);
	circular_buff_t *cbuf = malloc(sizeof(circular_buff_t));
	assert(cbuf);
	cbuf->infos = malloc(max_capacity * sizeof(cbuf_info));
	assert(cbuf->infos);
	memset(cbuf->infos, 0, sizeof(cbuf_info) * max_capacity);
	cbuf->max_capacity = max_capacity;
	cbuf->head = 0;
	cbuf->tail = 0;
	pthread_mutex_init(&cbuf->mutex, NULL);
	pthread_cond_init(&cbuf->full, NULL);
	pthread_cond_init(&cbuf->empty, NULL);
	return cbuf;
}


void circular_buff_free(circular_buff_t *cbuf) {
	assert(cbuf);
	free(cbuf->infos);
	pthread_mutex_destroy(&cbuf->mutex);
	pthread_cond_destroy(&cbuf->full);
	pthread_cond_destroy(&cbuf->empty);
	free(cbuf);
}


void circular_buff_push(circular_buff_t *cbuf, socket_t client_socket, State type) {
	assert(cbuf);
	pthread_mutex_lock(&cbuf->mutex);

	while(circular_buff_full(cbuf)) {
		pthread_cond_wait(&cbuf->full, &cbuf->mutex);
	}
	cbuf->infos[cbuf->head].client_socket = client_socket;
	cbuf->infos[cbuf->head].type = type;
	cbuf->head = cbuf_head_next_index(cbuf);

	pthread_cond_signal(&cbuf->empty);
	pthread_mutex_unlock(&cbuf->mutex);
}


cbuf_info *circular_buff_pop(circular_buff_t *cbuf) {
	assert(cbuf);
	pthread_mutex_lock(&cbuf->mutex);

	while(circular_buff_empty(cbuf)) {
		pthread_cond_wait(&cbuf->empty, &cbuf->mutex);
	}
	cbuf_info *info = &cbuf->infos[cbuf->tail];
	cbuf->tail = cbuf_tail_next_index(cbuf);

	pthread_cond_signal(&cbuf->full);
	pthread_mutex_unlock(&cbuf->mutex);

	return info;
}


bool circular_buff_empty(circular_buff_t *cbuf) {
	assert(cbuf);
	return (cbuf->head == cbuf->tail);
}


bool circular_buff_full(circular_buff_t *cbuf) {
	assert(cbuf);
	return (cbuf_head_next_index(cbuf) == cbuf->tail);
}


size_t circular_buff_max_capacity(circular_buff_t *cbuf) {
	assert(cbuf);
	return cbuf->max_capacity;
}

size_t circular_buff_curr_size(circular_buff_t *cbuf) {
	assert(cbuf);
	size_t curr_size = cbuf->max_capacity;
	if (!circular_buff_full(cbuf)) {
		if (cbuf->head >= cbuf->tail) { curr_size = cbuf->head - cbuf->tail; }
		else { curr_size = cbuf->max_capacity + cbuf->head - cbuf->tail; }
	}
	return curr_size;
}


