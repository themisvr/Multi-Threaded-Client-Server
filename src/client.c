#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "networking.h"
#include "client_utils.h"
#include "report_errors.h"

#define QUERY_LEN 256


typedef struct thread_args {
	size_t index;
	char *command;
} thread_args;


cmd_args args;
thread_info *tinfos;
thread_args *targs;
size_t queries;
bool syncr = false;
bool ready = false;

/* mutex/cond pair for syncronization of them */
pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sync_cond = PTHREAD_COND_INITIALIZER;

/* mutex/cond pair for reading one-by-one querries from the file */
pthread_mutex_t query_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t query_cond = PTHREAD_COND_INITIALIZER;

/* mutex/cond pair for sending querries to server */
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t send_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t chunk_mutex = PTHREAD_MUTEX_INITIALIZER;


static inline int connect_to_server(size_t ith_thread) {
	int connection = 1;
	socket_t client_socket;
	if (socket_create(args.server_port, inet_addr(args.server_ip), &client_socket) < 0) {
		handle_error("socket");
	}
	tinfos[ith_thread].client_socket = client_socket;
	if (socket_connect(&tinfos[ith_thread].client_socket) < 0) connection = -1;
	
	return connection;
}


static inline void send_query_to_server(size_t ith_thread) {
	ssize_t n_write;
	int sock_fd = tinfos[ith_thread].client_socket.socket_fd;
	uint32_t sendbytes = strlen(tinfos[ith_thread].command);
	char *query = tinfos[ith_thread].command;
	
	if ((n_write = write(sock_fd, &sendbytes, sizeof(uint32_t))) < 0)
		handle_error_en(n_write, "write");

	if ((n_write = write(sock_fd, query, sendbytes)) < 0) 
		handle_error_en(n_write, "write");

	printf("%s\n", query);
}


static inline void get_response(size_t ith_thread) {
	size_t response_len;
	ssize_t n_read;
	size_t sock_fd;
	char *response;

	sock_fd = tinfos[ith_thread].client_socket.socket_fd;

	if ((n_read = read(sock_fd, &response_len, sizeof(size_t))) < 0)
		handle_error_en(n_read, "read");

	response = malloc(response_len + 1);
	if (!response) handle_error("malloc");
	memset(response, '\0', response_len + 1);

	if ((n_read = read(sock_fd, response, response_len)) < 0)
		handle_error_en(n_read, "read");

	response[response_len] = '\0';

	if (strncmp(response, "NULL", strlen("NULL")) != 0)
		printf("%s\n", response);

	if ( close(sock_fd) < 0 )
		handle_error("close");
	free(response);
	
}


static void *client_query(void *arg) {	
	
	pthread_mutex_lock(&sync_mutex);

	/* Initialize ith thread and its command */
	int pos = *(int *)arg;
	tinfos[pos].ith_thread = targs[pos].index;
	tinfos[pos].command = strdup(targs[pos].command);

	ready = true;
	pthread_cond_signal(&query_cond);
	
	while (!syncr) {
		pthread_cond_wait(&sync_cond, &sync_mutex);
	}

	if (connect_to_server(pos) > 0) {
		pthread_mutex_lock(&print_mutex);

		send_query_to_server(pos);
		get_response(pos);

		pthread_mutex_unlock(&print_mutex);
	}
	else {
		fprintf(stderr, "Error: Thread [%ld] could not connect to the server\n", tinfos[pos].pthread_id);
	} 

	pthread_mutex_unlock(&sync_mutex);

}


static void join_threads() {
	int ret;
	for (size_t i = 0; i != args.threads_n; ++i) {
		ret = pthread_join(tinfos[i].pthread_id, NULL);
		if (ret != 0) handle_error_en(ret, "pthread_join");
	}
}


static void read_file(FILE *fp) {

	for (size_t j = 0; j != args.threads_n; ++j) {

		pthread_mutex_lock(&query_mutex);
		/* do some work here */
		char command[QUERY_LEN];
     	if (fgets(command, QUERY_LEN, fp) == NULL) handle_error("fgets");
     	thread_args info = {
     		.index = j,
     		.command = strdup(command)
     	};
     	targs[j] = info;
		/* create the thread and pass the information it need */
		pthread_create(&tinfos[j].pthread_id, NULL, client_query, (void *)&j);
		/* wait until child-thread is ready */
		while (!ready) {
			pthread_cond_wait(&query_cond, &query_mutex);
		}
		ready = false;
		pthread_mutex_unlock(&query_mutex);

	}
	/* when ready broadcast a signal to all threads */
	syncr = true;
    pthread_cond_broadcast(&sync_cond);

}


static void clear_thread_infos() {
	for (size_t i = 0; i != args.threads_n; ++i) {
		free(tinfos[i].command);
		free(targs[i].command);
	}
	free(tinfos);
	free(targs);
}

static void destroy_pthread_vars() {
	pthread_mutex_destroy(&chunk_mutex);
	pthread_mutex_destroy(&print_mutex);
	pthread_mutex_destroy(&query_mutex);
	pthread_cond_destroy(&query_cond);
	pthread_cond_destroy(&sync_cond);
	pthread_mutex_destroy(&sync_mutex);
	pthread_mutex_destroy(&send_mutex);
	pthread_cond_destroy(&send_cond);
}

int main(int argc, char **argv) {

	if (argc != 9) {
		fprintf(stderr, "Usage: %s [-q queryFile] [-w numThreads] [-sp serverPort] [-sip serverIP]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	parse_client_args(argc, argv, &args);

	char command[512]; 							
	FILE *fp; 									
	fp = fopen(args.query_file, "r");						
	if (!fp) handle_error("fopen");				
												
	while (fgets(command, 512, fp) != NULL)		
		++queries;								

	if (fseek(fp, 0, SEEK_SET) < 0) handle_error("fseek");
	
	size_t reps = queries / args.threads_n;

	for (size_t i = 0; i != reps; ++i) {
		tinfos = calloc(args.threads_n, sizeof(thread_info));
		if (!tinfos) handle_error("calloc");
		targs = calloc(args.threads_n, sizeof(thread_args));
		if (!targs) handle_error("calloc");

		read_file(fp);
		join_threads();
		clear_thread_infos();
	}
	if ((queries % args.threads_n) != 0) {
		queries -= reps * args.threads_n;
		/* No need to create more threads than querries */
		if (args.threads_n > queries) args.threads_n = queries;

		tinfos = calloc(args.threads_n, sizeof(thread_info));
		if (!tinfos) handle_error("calloc");
		targs = calloc(args.threads_n, sizeof(thread_args));
		if (!targs) handle_error("calloc");

		read_file(fp);
		join_threads();
		clear_thread_infos();
	}
	fclose(fp);
	destroy_pthread_vars();
	
	return EXIT_SUCCESS;
}