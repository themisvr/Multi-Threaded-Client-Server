#ifndef _WORKER_UTILS_H
#define _WORKER_UTILS_H

#include "hash_table.h"
#include "key_value.h"
#include "networking.h"

#define N_BUCKETS 100
#define BUCKETS_SIZE 100


char *read_info_from_master(int readfd, size_t bytes_in_fifo, size_t *port_n, char *server_ip,
							hashTable *htable, doubleLinkedList *gen_list) __attribute__ ((nonnull (5, 6)));

char *insert_records(hashTable *htable, doubleLinkedList *gen_list, char *filename, char *dirname) __attribute__ ((nonnull (1, 2, 3, 4)));

char *construct_stats(const char *dirname, char *filename, hash_set *set) __attribute__ ((nonnull (1, 2, 3)));

char *parse_commands(char *command, hashTable *htable, doubleLinkedList *gen_list) __attribute__ ((nonnull (1, 2, 3)));

void connect_to_server(size_t server_port, char *server_ip, socket_t *worker_socket) __attribute__ ((nonnull (2, 3)));

void send_stats_to_server(char *stats, socket_t *worker_socket) __attribute__ ((nonnull (1)));

void setup_worker_as_server(int stats_fd, socket_t *worker_server_socket) __attribute__ ((nonnull (2)));

void send_worker_query_port(int stats_fd, socket_t *worker_server_socket) __attribute__ ((nonnull (2)));

char *handle_request(size_t client_fd);

void send_response_to_server(char *request, size_t sock_fd, hashTable *htable, doubleLinkedList *gen_list) __attribute__ ((nonnull (1, 3, 4)));

void send_error_to_server(size_t sock_fd);

int validate_request(char *request) __attribute__ ((nonnull (1)));

#endif // _WORKER_UTILS_H