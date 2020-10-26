#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server_utils.h"
#include "networking.h"
#include "report_errors.h"


void parse_server_args(int argc, char **argv, cmd_args *args) {

	int opt;
	while ((opt = getopt(argc, argv, "q:s:w:b:")) != -1) {
		switch (opt) {
			case 'q':
				args->q_port = atoi(optarg);
				break;
			case 's':
				args->s_port = atoi(optarg);
				break;
			case 'w':
				args->threads_n = atoi(optarg);
				if (args->threads_n < 1) {
					fprintf(stderr, "Error: Invalid number of workers\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'b':
				args->cbuf_s = atoi(optarg);
				if (args->cbuf_s < 1) {
					fprintf(stderr, "Error: Invalid buffer size\n");
					exit(EXIT_FAILURE);
				}
				break;
			default:    /* ? */
				fprintf(stderr, "Usage: %s [-q queryPortNum] [-s statisticsPortNum] [-w numThreads] [-b bufferSize]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}
}


void setup_server(size_t port, socket_t *server_socket) {

	int server_fd = socket_create(port, ANY_ADDR, server_socket);
	if (server_fd < 0) handle_error_en(server_fd, "socket");

	if (socket_bind(server_socket) < 0) handle_error("bind");

	if (socket_listen(server_socket) < 0) handle_error("listen");
}


uint16_t read_stats_from_worker(size_t sock_fd) {
	
	size_t response_len = 1024;
	char buff[response_len];
	ssize_t nread;
	size_t curr_read = 0;
	char *response = NULL;

	size_t resp_len = 0;
	if ((nread = read(sock_fd, &resp_len, sizeof(size_t))) < 0)
		handle_error_en(nread, "read");
	
	response = malloc(resp_len + 1);
	if (!response) handle_error("malloc");
	memset(response, '\0', resp_len + 1);
	
	if (response_len > resp_len) {
		response_len = resp_len + 1;
	}
	while (curr_read <= resp_len) {
		if ((nread = read(sock_fd, buff, response_len)) < 0) {
			if (errno != EINTR)
				handle_error_en(nread, "read");
		}
		if (curr_read > resp_len) break;
		curr_read += nread;
		strncat(response, buff, nread);
		if (curr_read + response_len > resp_len) {
			response_len = (resp_len - curr_read) + 1;
		}
	}
	response[resp_len] = '\0';
	//printf("%s", response);
	free(response);

	uint16_t worker_port = 0;
	if ((nread = read(sock_fd, &worker_port, sizeof(uint16_t))) < 0)
		handle_error_en(nread, "read");

	return worker_port;
}


static inline socket_t connect_to_worker_sever(worker_info *ip_port_tuple) {

	socket_t worker_socket = {0};

	char *ip = ip_port_tuple->worker_ip;
	uint16_t worker_port = ip_port_tuple->worker_port;

	if (socket_create(worker_port, inet_addr(ip), &worker_socket) < 0)
		handle_error("socket");
	
	if (socket_connect(&worker_socket) < 0)
		handle_error("connect");

	return worker_socket;
}


char *recv_request_from_client(size_t sock_fd) {
	ssize_t n_read;
	uint32_t request_len = 0;
	char *request = NULL;

	if ((n_read = read(sock_fd, &request_len, sizeof(uint32_t))) < 0)
		handle_error_en(n_read, "read");

	request = malloc(request_len + 1);
	if (!request) handle_error("malloc");
	memset(request, '\0', request_len + 1);

	if ((n_read = read(sock_fd, request, request_len)) < 0)
		handle_error_en(n_read, "read");

	request[request_len] = '\0';

	return request;
}


static inline char *recv_response_from_workers(socket_t worker_socket) {
	ssize_t n_read;
	size_t response_len = 0;
	char *response;

	if ((n_read = read(worker_socket.socket_fd, &response_len, sizeof(size_t))) < 0)
		handle_error_en(n_read, "read");

	response = malloc(response_len + 1);
	if (!response) handle_error("malloc");
	memset(response, '\0', response_len + 1);

	if ((n_read = read(worker_socket.socket_fd, response, response_len)) < 0)
		handle_error_en(n_read, "read");

	response[response_len] = '\0';
	
	return response;
}

void send_response_to_client(size_t client_fd, char *response) {
	ssize_t n_write;
	size_t response_len;

	response_len = strlen(response);
	if ((n_write = write(client_fd, &response_len, sizeof(size_t))) < 0)
		handle_error_en(n_write, "write");

	if ((n_write = write(client_fd, response, response_len)) < 0)
		handle_error_en(n_write, "write");
}


char *send_request_to_workers(doubleLinkedList *workers, char *request) {
	ssize_t n_write;
	size_t request_len;
	size_t total_size = 0;
	worker_info *ip_port_tuple;
	socket_t worker_socket;
	char *response = NULL;
	char *total_response = NULL;

	total_response = malloc(1);
	memset(total_response, '\0', 1);

	for (listNode *node = workers->head; node != NULL; node = node->next) {
		ip_port_tuple = (worker_info *) list_get_item(workers, node);
		worker_socket = connect_to_worker_sever(ip_port_tuple);

		request_len = strlen(request);
		if ((n_write = write(worker_socket.socket_fd, &request_len, sizeof(size_t))) < 0)
			handle_error_en(n_write, "write");

		if ((n_write = write(worker_socket.socket_fd, request, request_len)) < 0)
			handle_error_en(n_write, "write");
	
		response = recv_response_from_workers(worker_socket);
		
		if ( close(worker_socket.socket_fd) < 0 )
			handle_error("close");

		if (strncmp(response, "NULL", strlen("NULL")) != 0) {
			total_size += strlen(response) + 1;
			total_response = realloc(total_response, total_size);
			strncat(total_response, response, strlen(response));
		}
		free(response);
	}
	total_response[strlen(total_response)] = '\0';
	if (strlen(total_response) == 0) {
		free(total_response);
		total_response = strdup("NULL");
	}
	
	return total_response;
}
