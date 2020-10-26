#ifndef _PARENT_UTILS_H
#define _PARENT_UTILS_H

#include <stdint.h>

#include "named_pipes.h"
#include "hash_table.h"


typedef struct cmd_args {
	size_t n_workers;
	size_t buff_size;
	ssize_t server_port;
	char *server_ip;
	char *input_dir;
} cmd_args;

/* general info needed for each worker */
typedef struct worker_info {
	int pipe_fd;
	char *fifo;
	pid_t pid;
	uint32_t n_countries;
	char **countries;
	char *country_paths;
} worker_info;


/* parses command line args */
void parse_master_args(int argc, char **argv, cmd_args *args) __attribute__((nonnull (2, 3)));

/* creates an array of named pipes and initializes em */
named_pipe *create_pipes(size_t n_workers, size_t buff_size);

/* takes input_dir as argument and shares the files to the spawned processes */
char **list_subdirs(const char *dirpath, size_t n_dirs) __attribute__((nonnull (1)));

void send_info_to_workers(int writefd, char *dirs, size_t bytes_in_fifo, size_t port_n, char *server_ip) __attribute__ ((nonnull (2, 5)));

void send_server_ip(size_t workers, named_pipe *fifos, char *server_ip, size_t bytes_in_fifo) __attribute__ ((nonnull (2, 3)));

void send_server_port(int writefd, int port_n);

/* distributes the files to the processes */
char *distribute_dirs(uint32_t index, size_t curr_worker, size_t workers, uint32_t n_dirs, char **subdirs) __attribute__ ((nonnull (5)));

char **list_countries(char *countries, size_t n_countries) __attribute__ ((nonnull (1)));

void free_fifos(named_pipe* fifos, worker_info* winfo, size_t workers) __attribute__ ((nonnull (1, 2)));


#endif // _PARENT_UTILS_H