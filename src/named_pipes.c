#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#include "named_pipes.h"


named_pipe named_pipe_init(size_t size, const char *fifo_name) {
	named_pipe fifo = {
			.pipe_size = size,
			.fd = -1
	};
	size_t name_len = strlen(fifo_name);
	fifo.pipe_name = malloc(name_len + 1);
	if (!fifo.pipe_name) {
		fprintf(stderr, "Error: Could not allocate memory for pipe's name\n");
		exit(EXIT_FAILURE);
	}
	memcpy(fifo.pipe_name, fifo_name, name_len + 1);
	fifo.pipe_name[name_len] = '\0';

	if ((mkfifo(fifo_name, PERMS) < 0) && (errno != EEXIST)) {
		fprintf(stderr, "Error: Could not create a named pipe\n");
		exit(EXIT_FAILURE);
	}
	return fifo;
}


void named_pipe_open(named_pipe *fifo, int flags) {
	if ((fifo->fd = open(fifo->pipe_name, flags)) == -1) {
		fprintf(stderr, "Error: Could not open given fifo\n");
		exit(EXIT_FAILURE);
	}
}


void named_pipe_close(named_pipe *fifo) {
	if (close(fifo->fd) == -1) {
		fprintf(stderr, "Error: Could not close the file descriptor of the given fifo\n");
		exit(EXIT_FAILURE);
	}
}


void named_pipe_unlink(named_pipe *fifo) {
	if (unlink(fifo->pipe_name) == -1) {
		fprintf(stderr, "Error: Could not unlink given fifo\n");
		exit(EXIT_FAILURE);
	}
}


char *named_pipe_read(int readfd, size_t bytes_in_fifo) {
	ssize_t n_read;
	size_t msg_len = 0;

	if ((n_read = read(readfd, &msg_len, sizeof(size_t))) < 0) {
		if (errno != EINTR) {
			perror("read from pipe");
			exit(EXIT_FAILURE);
		}
	}
	char *msg = malloc(msg_len + 1);
	if (!msg) {
		fprintf(stderr, "Error: Could not allocate memory for msg\n");
		exit(EXIT_FAILURE);
	}
	memset(msg, '\0', msg_len + 1);

	if (bytes_in_fifo > msg_len) {
		bytes_in_fifo = msg_len + 1;
	}
	char *buff = malloc(bytes_in_fifo + 1);
	if (!buff) {
		fprintf(stderr, "Error: Could not allocate memory for buffer\n");
		exit(EXIT_FAILURE);
	}
	memset(buff, '\0', bytes_in_fifo + 1);

	size_t curr_read = 0;
	while (curr_read <= msg_len) {
		if ((n_read = read(readfd, buff, bytes_in_fifo)) < 0) {
			if (errno != EINTR) {
				perror("read from pipe");
				exit(EXIT_FAILURE);
			}
		}
		curr_read += n_read;
		strncat(msg, buff, n_read);
	}
	msg[msg_len] = '\0';
	free(buff);

	return msg; 
}


void named_pipe_write(int writefd, char *msg, size_t bytes_in_fifo) {
	ssize_t n_write = 0;
	size_t total_write = 0;

	size_t msg_len = strlen(msg);
	char *temp = msg;

	if ((n_write = write(writefd, &msg_len, sizeof(size_t))) < 0) {
		if (errno != EINTR) {
			perror("write()");
			exit(EXIT_FAILURE);
		}
	}
	if (bytes_in_fifo > msg_len) {
		bytes_in_fifo = msg_len + 1;
	}
	while (total_write <= msg_len) {
		if ((n_write = write(writefd, temp, bytes_in_fifo)) < 0) {
			perror("write()");
			exit(EXIT_FAILURE);
		}
		total_write += n_write;
		if (total_write > msg_len) break;
		temp += sizeof(char) * n_write;
		if (total_write + bytes_in_fifo > msg_len) {
			bytes_in_fifo = (msg_len - total_write) + 1;
		}
	}
}
