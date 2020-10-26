#ifndef _CLIENT_UTILS_H
#define _CLIENT_UTILS_H

#include <stdlib.h>
#include "networking.h"

typedef struct cmd_args {
	size_t threads_n;
	size_t server_port;
	char *server_ip;
	char *query_file;
} cmd_args;


typedef struct thread_info {
	size_t ith_thread;
	pthread_t pthread_id;
	socket_t client_socket;
	char *command;
} thread_info;


void parse_client_args(int argc, char **argv, cmd_args *args) __attribute__ ((nonnull (2, 3)));




#endif // _CLIENT_UTILS_H