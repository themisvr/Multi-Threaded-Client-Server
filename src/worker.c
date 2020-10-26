#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "utils.h"
#include "hash_table.h"
#include "named_pipes.h"
#include "generic_list.h"
#include "worker_utils.h"
#include "alloc_funcs.h"
#include "compare_funcs.h"
#include "report_errors.h"
#include "networking.h"


volatile sig_atomic_t gotsig = -1;


static void int_handler(int signum) {
	gotsig = signum;
}


void register_signals() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
    act.sa_handler = int_handler;
    sigaction(SIGINT, &act, NULL);
}


int main(int argc, char **argv) {

	if (argc != 3) {
		fprintf(stderr, "Error: Child process got wrong number of arguments\n");
		_exit(EXIT_FAILURE);
	}

	int readfd;
	if ((readfd = open(argv[1], O_RDONLY)) == -1) {
		perror("worker: open");
		exit(EXIT_FAILURE);
	}

	//register_signals();

	char *stats = NULL;

	size_t bytes_in_fifo = atoi(argv[2]);
	hashTable *htable = hash_table_init(N_BUCKETS, BUCKETS_SIZE, NULL, bucket_data_alloc, bucket_data_dealloc);
	doubleLinkedList *gen_list = list_create(id_cmp, list_data_alloc, list_data_dealloc);

	size_t server_port;
	char *server_ip = malloc(20);
	memset(server_ip, '\0', 20);
	stats = read_info_from_master(readfd, bytes_in_fifo, &server_port, server_ip, htable, gen_list);

	socket_t *worker_socket = malloc(sizeof(socket_t));
	if (!worker_socket) handle_error("malloc");

	connect_to_server(server_port, server_ip, worker_socket);
	send_stats_to_server(stats, worker_socket);

	socket_t *worker_server_socket = malloc(sizeof(socket_t));
	if (!worker_server_socket) handle_error("malloc");

	setup_worker_as_server(worker_socket->socket_fd, worker_server_socket);

	int ret;
	char *request = NULL;

	while (true) {
		socket_t client_socket = {0};
		if ((ret = socket_accept(worker_server_socket, &client_socket)) < 0)
			handle_error_en(ret, "accept");

		request = handle_request(client_socket.socket_fd);
		if (validate_request(request) > 0)
			send_response_to_server(request, client_socket.socket_fd, htable, gen_list);
		else
			send_error_to_server(client_socket.socket_fd);

		if ( close(client_socket.socket_fd) < 0 )
			handle_error("close");
	}

	if (close(worker_server_socket->socket_fd) < 0) 
		handle_error("close");
	
	free(stats);
	free(server_ip);
	free(worker_socket);
	free(worker_server_socket);
	list_destroy(gen_list);
	hash_table_free(htable);

	exit(EXIT_SUCCESS);
}