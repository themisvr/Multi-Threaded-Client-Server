#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include "parent_utils.h"
#include "named_pipes.h"
#include "file_handler.h"
#include "commands.h"
#include "utils.h"


void parse_master_args(int argc, char **argv, cmd_args *args) {

	int opt;
	while ((opt = getopt(argc, argv, "w:b:s:p:i:")) != -1) {
		switch (opt) {
			case 'w':
				args->n_workers = atoi(optarg);
				if (args->n_workers < 1) {
					fprintf(stderr, "Error: Invalid number of workers\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'b':
				args->buff_size = atoi(optarg);
				if (args->buff_size < 1) {
					fprintf(stderr, "Error: Invalid buffer size\n");
					exit(EXIT_FAILURE);
				} 
				break;
			case 's':
				args->server_ip = optarg;
				break;
			case 'p':
				args->server_port = atoi(optarg);
				if (args->server_port < 0) {
					fprintf(stderr, "Error: Give a valid port\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'i':
				args->input_dir = optarg;
				if (args->input_dir[strlen(args->input_dir) - 1] == '/') {
					args->input_dir[strlen(args->input_dir) - 1] = '\0';
				}
				if (!file_is_dir(args->input_dir)) {
					fprintf(stderr, "Error: You must give a directory as <input_dir> parameter\n");
					exit(EXIT_FAILURE);
				}
				break;
			default:    /* ? */
				fprintf(stderr, "Usage: %s [-w numWorkers] [-b bufferSize] [-s serverIP] [-p serverPort] [-i input_dir]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}
}


named_pipe *create_pipes(size_t n_workers, size_t buff_size) {
	char fifo_name[50];
	size_t  n_pipes = n_workers;

	named_pipe *fifo_array = malloc(sizeof(named_pipe) * n_pipes);
	if (!fifo_array) {
		fprintf(stderr, "Error: Could not allocate memory for fifos' array\n");
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i != n_pipes; ++i) {
		sprintf(fifo_name, "fifo.%ld", i);
		fifo_array[i] = named_pipe_init(buff_size, fifo_name);	
	}
	return fifo_array;
}


char **list_subdirs(const char *dirpath, size_t n_dirs) {
	if (file_is_dir(dirpath)) {
		char **subdirs = subdir_names(dirpath, n_dirs);
		return subdirs;
	}
	else { return NULL; }
}


char *distribute_dirs(uint32_t index, size_t curr_worker, size_t workers, uint32_t n_dirs, char **subdirs) {
	char *dirs = NULL;
	/* n_dirs / workers is the number of directories that a worker is going to process */
	uint32_t each_worker_dirs = n_dirs / workers;
	/* n_dirs % workers is the number of workers that are going to take +1 directory to process */
	uint32_t rr = n_dirs % workers;

	dirs = malloc(strlen(subdirs[index]) + 2);
	if (!dirs) {
		fprintf(stderr, "Error: Could not allocate memory for directories' names\n");
		exit(EXIT_FAILURE);
	}
	memset(dirs, '\0', strlen(subdirs[index]) + 2);
	size_t str_len = strlen(subdirs[index]) + 2;

	if (rr != 0) {
		if (curr_worker <= (rr - 1)) {
			each_worker_dirs++;
		}
	}
	for (uint32_t i = 0; i != each_worker_dirs; ++i) {
		strncat(dirs, subdirs[index + i], str_len);
		str_len += strlen(subdirs[index + i]) + 10;
		dirs = realloc(dirs, str_len);
		strncat(dirs, " ", 1);
	}
	dirs[strlen(dirs)-1] = '\0';

	return dirs;
}


void send_info_to_workers(int writefd, char *dirs, size_t bytes_in_fifo, size_t port_n, char *server_ip) {
	size_t total_info_len = strlen(server_ip) + 10 + strlen(dirs) + 2 * strlen("\n");  
	char *infos = malloc(total_info_len + 1);
	assert(infos);
	memset(infos, '\0', total_info_len + 1);
	sprintf(infos, "%s\n%ld\n%s", server_ip, port_n, dirs);
	named_pipe_write(writefd, infos, bytes_in_fifo);
	free(infos);
}


void free_fifos(named_pipe* fifos, worker_info* winfo, size_t workers) {
	for (size_t i = 0; i != workers; ++i) {
		named_pipe_close(&fifos[i]);
		named_pipe_unlink(&fifos[i]);

		for (size_t j = 0; j != winfo[i].n_countries; ++j) {
			free(winfo[i].countries[j]);
		}
		free(winfo[i].countries);
		free(winfo[i].fifo);
		free(winfo[i].country_paths);

		free(fifos[i].pipe_name);
	}
	free(fifos);
	free(winfo);
}


static inline char *parse_dirs(char *countries) {
	char *token = NULL, *tok_prev = NULL, *end_tok;
	char *country = NULL, *tok, *prev = NULL;
	token = strtok_r(countries, " ", &end_tok);
	while (token != NULL) {
		tok_prev = token;
		token = strtok_r(NULL, " ", &end_tok);
	}
	if (tok_prev != NULL) {
		prev = strtok_r(tok_prev, "/", &tok);
		while (prev != NULL) {
			country = prev;
			prev = strtok_r(NULL, "/", &tok);
		}
	}
	return country;
}

char **list_countries(char *countries, size_t n_countries) {
	char **list = malloc(n_countries * sizeof(char *));
	if (!list) {
		fprintf(stderr, "Error: Could not allocate memory for list\n");
		exit(EXIT_FAILURE);
	}
	for (uint32_t i = 0; i != n_countries; ++i) {
		list[i] = malloc(20 * sizeof(char));
		if (!list[i]) {
			fprintf(stderr, "Error: Could not allocate memory for countries list\n");
			exit(EXIT_FAILURE);
		}
		memset(list[i], '\0', 20);
	}

	for (uint32_t i = 0; i != n_countries; ++i) {
		char *country = parse_dirs(countries);
		strncpy(list[i], country, strlen(country));
	}
	return list;
}
