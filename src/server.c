#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "networking.h"
#include "server_utils.h"
#include "report_errors.h"
#include "alloc_funcs.h"
#include "generic_list.h"
#include "shared_circular_buffer.h"


volatile sig_atomic_t sig = 0;

cmd_args args;
socket_t stats_server_socket;
socket_t query_server_socket;
circular_buff_t *cbuffer;
doubleLinkedList *workers;
pthread_t *tpool;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;


static void *handle_requests(__attribute__((unused)) void *arg) {

	if (pthread_detach(pthread_self()) < 0)
		handle_error("pthread_detach");

	size_t cl_fd;
	uint16_t worker_port;
	char *request = NULL;
	char *response = NULL;
	worker_info *ip_port_tuple;

	while (true) {

		cbuf_info *bufinfo = circular_buff_pop(cbuffer);
		cl_fd = (bufinfo->client_socket).socket_fd;

		if (bufinfo->type == WORKER) {
			worker_port = read_stats_from_worker(cl_fd);
			ip_port_tuple = create_ip_port_tuple(cl_fd, worker_port);

			pthread_mutex_lock(&list_mutex);
			list_last_insert(workers, ip_port_tuple);
			pthread_mutex_unlock(&list_mutex);

			if ( close(cl_fd) < 0 )
				handle_error("close");
		}
		/* That means CLIENT requested */
		else {
			pthread_mutex_lock(&print_mutex);
			
			/* Get request from cient */
			request = recv_request_from_client(cl_fd);
			printf("%s\n", request);
			
			/* Send the request from client to workers and get the response */
			response = send_request_to_workers(workers, request);
			
			// if (strncmp(strtok(response, "\n"), "400 Bad Request", strlen("400 Bad Request")) == 0)
			// 	printf("400 Bad Request\n\n");
			if (strncmp(response, "NULL", strlen("NULL")) != 0)
				printf("%s\n", response);

			/* send response from workers to client */
			send_response_to_client(cl_fd, response);

			free(request);
			free(response);

			if ( close(cl_fd) < 0 )
				handle_error("close");
			
			pthread_mutex_unlock(&print_mutex);

		}
	}
}


static void start_threads() {
	for (size_t i = 0; i != args.threads_n; ++i) {
		if (pthread_create(&tpool[i], NULL, handle_requests, NULL) < 0)
			handle_error("pthread_create");
	}
}


static void cancel_threads() {
	for (size_t i = 0; i != args.threads_n; ++i)
		pthread_cancel(tpool[i]);
}


static void handle_sigint(int signum) {
    assert(signum == SIGINT);
    sig = 1;
}

void register_signals() {
    struct sigaction setup_action = {};
    sigemptyset(&setup_action.sa_mask);
    setup_action.sa_handler = handle_sigint;
    setup_action.sa_flags = 0;
    sigaction(SIGINT, &setup_action, NULL);
}

int main(int argc, char **argv) {

	if (argc != 9) {
		fprintf(stderr, "Usage: %s [-q queryPortNum] [-s statisticsPortNum] [-w numThreads] [-b bufferSize]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	register_signals();

	struct pollfd pfds[2];

	parse_server_args(argc, argv, &args);

	tpool = calloc(args.threads_n, sizeof(pthread_t));
	if (!tpool) handle_error("calloc");
	
	workers = list_create(NULL, worker_info_alloc, worker_info_dealloc);
	cbuffer = circular_buff_init(args.cbuf_s);

	setup_server(args.s_port, &stats_server_socket);
	setup_server(args.q_port, &query_server_socket);
	printf("Server listening...\n");

	start_threads();

	int ret;

	pfds[0].fd = query_server_socket.socket_fd;
	pfds[0].events = POLLIN;

	pfds[1].fd = stats_server_socket.socket_fd;
	pfds[1].events = POLLIN;

	sigset_t block_mask, unblock_mask, waiting_mask;
	sigfillset(&block_mask);

	while (!sig) {

		while (poll(pfds, 2, -1) == -1 && errno == EINTR) {
			if (sig) {
				/* cleanup() */;
				goto cleanup;
			}
		}

		sigprocmask(SIG_SETMASK, &block_mask, &unblock_mask);
		
		socket_t client_socket = {0};
		State type;

		if (pfds[1].revents & POLLIN) {
			/* Statistics */
			type = WORKER;
			if ((ret = socket_accept(&stats_server_socket, &client_socket)) < 0)
				handle_error_en(ret, "accept");
			circular_buff_push(cbuffer, client_socket, type);

		}
		if (pfds[0].revents & POLLIN) {
			/* Query */
			type = CLIENT;
			if ((ret = socket_accept(&query_server_socket, &client_socket)) < 0)
				handle_error_en(ret, "accept");
			circular_buff_push(cbuffer, client_socket, type);
		}

		sigprocmask(SIG_SETMASK, &unblock_mask, NULL);
		sigpending(&waiting_mask);
		if (sigismember(&waiting_mask, SIGINT)) {
			raise(SIGINT);
		}
	}
		
	cleanup:
		cancel_threads();
		list_destroy(workers);
		circular_buff_free(cbuffer);
		pthread_mutex_destroy(&list_mutex);
		pthread_mutex_destroy(&print_mutex);
		free(tpool);
	
		exit(EXIT_SUCCESS);

}