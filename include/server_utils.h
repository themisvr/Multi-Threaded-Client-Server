#ifndef _SERVER_UTILS_H
#define _SERVER_UTILS_H

#include <assert.h>
#include "networking.h"
#include "generic_list.h"
#include "report_errors.h"

typedef struct cmd_args {
	size_t q_port;
	size_t s_port;
	size_t threads_n;
	size_t cbuf_s;
} cmd_args;


typedef struct worker_info {
	char *worker_ip;
	uint16_t worker_port;
} worker_info;



static inline worker_info *create_ip_port_tuple(size_t sock_fd, uint16_t port) {

	struct sockaddr_in peer;
   	socklen_t peer_len;
 
 	peer_len = sizeof(peer);

	worker_info *infos = malloc(sizeof(worker_info));
	assert(infos);

	getpeername(sock_fd, (struct sockaddr *)&peer, &peer_len);

	infos->worker_ip = strdup(inet_ntoa(peer.sin_addr));
	infos->worker_port = port;

	return infos;
}


void parse_server_args(int argc, char **argv, cmd_args *args) __attribute__ ((nonnull (2, 3)));

void setup_server(size_t port, socket_t *server_socket) __attribute__ ((nonnull (2)));

uint16_t read_stats_from_worker(size_t sock_fd);

char *recv_request_from_client(size_t sock_fd);

char *send_request_to_workers(doubleLinkedList *workers, char *request) __attribute__ ((nonnull (1, 2)));

void send_response_to_client(size_t sock_fd, char *response) __attribute__ ((nonnull (2)));

#endif // _SERVER_UTILS_H