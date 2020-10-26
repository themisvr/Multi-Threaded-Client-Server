#ifndef _NETWORKING_H
#define _NETWORKING_H

#include <netinet/in.h>
#include <arpa/inet.h>

#define ANY_ADDR INADDR_ANY


typedef struct socket_t {
	struct sockaddr_in address;
	int socket_fd;
} socket_t;


int socket_create(size_t port_n, in_addr_t inaddr, socket_t *ip_socket) __attribute__((nonnull (3)));

int socket_bind(socket_t *ip_socket) __attribute__((nonnull (1)));

int socket_listen(socket_t *ip_socket) __attribute__((nonnull (1)));

int socket_accept(socket_t *server_socket, socket_t *client_socket) __attribute__((nonnull (1, 2)));

int socket_connect(socket_t *ip_socket) __attribute__((nonnull (1)));


#endif // NETWORKING_H