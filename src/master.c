#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "parent_utils.h"
#include "file_handler.h"
#include "alloc_funcs.h"
#include "utils.h"


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


int main(int argc, char *argv[]) {

	if (argc != 11) {
		fprintf(stderr, "Usage: %s [-w numWorkers] [-b bufferSize] [-s serverIP] [-p serverPort] [-i input_dir]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	cmd_args args;
	parse_master_args(argc, argv, &args);

	register_signals();

	size_t actual_workers = args.n_workers;
	
	uint32_t n_dirs = n_files_in_directory(args.input_dir, "dirs");
	char **subdirs = list_subdirs(args.input_dir, n_dirs);

	/* we dont have to create extra workers */
	if (args.n_workers > n_dirs) { 
		actual_workers = n_dirs; 
	}
	named_pipe *fifo_array = create_pipes(actual_workers, args.buff_size);
	uint32_t  each_worker_dirs = n_dirs / actual_workers;
	uint32_t index = 0;

	worker_info *info = malloc(actual_workers * sizeof(worker_info));
	assert(info);
	memset(info, 0, actual_workers * sizeof(worker_info));

	pid_t pid, wpid;
	int status;
	for (size_t i = 0; i != actual_workers; ++i) {

		pid = fork();

		if (pid != 0) { // parent process (client)

			named_pipe_open(&fifo_array[i], O_WRONLY);

			char *dirs = distribute_dirs(index, i, actual_workers, n_dirs, subdirs);
			if ((n_dirs % actual_workers) == 0) { 
				index += each_worker_dirs; 
			}
			else {
				if (i <= ((n_dirs % actual_workers) - 1)) {
					index += each_worker_dirs + 1;
				}
				else {
					index += each_worker_dirs;
				}
			}

			char *countries = strdup(dirs);
			char *tok = strdup(dirs);
			size_t n_countries = countries_count(dirs);

			worker_info winfo = {
				.pipe_fd = fifo_array[i].fd,
				.fifo = strdup(fifo_array[i].pipe_name),
				.pid = pid,
				.n_countries = n_countries,
				.countries = list_countries(countries, n_countries),
				.country_paths = strdup(tok)
			};
			info[i] = winfo;

			send_info_to_workers(winfo.pipe_fd, tok, args.buff_size, args.server_port, args.server_ip);

			free(dirs);
			free(countries);
			free(tok);
		}
		else if (pid == 0) {   // child process (server)
			
			char *pass_args[] = { "./worker", fifo_array[i].pipe_name, argv[4], NULL };
			char *new_env[] = { NULL };

			execve("./worker", pass_args, new_env);
			perror("execve");
	        exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error: Could not fork child processes\n");
			exit(EXIT_FAILURE);
		}
	}

	while (1) {
		if (gotsig == SIGINT) {
			for (size_t i = 0; i != actual_workers; ++i) {
				kill(info[i].pid, SIGKILL);
			}
			while ((wpid = wait(&status)) > 0);

			free_fifos(fifo_array, info, actual_workers);
			for (size_t i = 0; i != n_dirs; ++i) {
				free(subdirs[i]);
			}
			free(subdirs);

			exit(EXIT_SUCCESS);
		}
	}
}