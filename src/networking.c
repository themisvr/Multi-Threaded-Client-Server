#include <sys/socket.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "networking.h"

int socket_create(size_t port_n, in_addr_t inaddr, socket_t *ip_socket) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) return -1;
	socklen_t socket_option_value = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &socket_option_value, sizeof(socket_option_value));
	struct sockaddr_in address;
	bzero(&address, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_port = htons(port_n);
	address.sin_addr.s_addr = inaddr;
	ip_socket->address = address;
	ip_socket->socket_fd = fd;
	return 0;
}


int socket_bind(socket_t *ip_socket) {
	return bind(ip_socket->socket_fd, 
				(const struct sockaddr *)&ip_socket->address, 
				(socklen_t)sizeof(ip_socket->address));

}


int socket_listen(socket_t *ip_socket) {
	return listen(ip_socket->socket_fd, SOMAXCONN);
}


int socket_accept(socket_t *server_socket, socket_t *client_socket) {
	socklen_t client_len = sizeof(client_socket->address);
	int socket_fd = accept(server_socket->socket_fd, (struct sockaddr *)&client_socket->address, &client_len);
	if (socket_fd == -1) return -1;
	client_socket->socket_fd = socket_fd;
	return 0;
}


int socket_connect(socket_t *ip_socket) {
	return connect(	ip_socket->socket_fd, 
					(const struct sockaddr *)&ip_socket->address, 
					(socklen_t)sizeof(ip_socket->address));
}
