#ifndef NAMED_PIPES_H
#define NAMED_PIPES_H

#include <sys/types.h>

#define PERMS 0666

typedef struct pipe {
	size_t pipe_size;
	char *pipe_name;
	int fd;
} named_pipe;


/* creates a named pipe */
named_pipe named_pipe_init(size_t size, const char *fifo_name) __attribute__((nonnull (2)));

/* opens a named pipe with given mode (PERMS) */
void named_pipe_open(named_pipe *fifo, int flags) __attribute__ ((nonnull (1)));

/* closes the file descriptor of the given named pipe */
void named_pipe_close(named_pipe *fifo) __attribute__ ((nonnull (1)));

/* destroys the created pipes */
void named_pipe_unlink(named_pipe *fifo) __attribute__ ((nonnull (1)));

char *named_pipe_read(int readfd, size_t bytes_in_fifo);

void named_pipe_write(int writefd, char *msg, size_t bytes_in_fifo) __attribute__ ((nonnull (2)));


#endif // NAMED_PIPES_H