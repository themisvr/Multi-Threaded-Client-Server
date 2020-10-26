#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "networking.h"
#include "file_handler.h"
#include "client_utils.h"
#include "report_errors.h"


void parse_client_args(int argc, char **argv, cmd_args *args) {
	int opt;
	while ((opt = getopt(argc, argv, "q:w:p:s:")) != -1) {
		switch (opt) {
			case 'q':
				args->query_file = optarg;
				if (!file_exists(args->query_file)) {
					fprintf(stderr, "Error: Please give an existing query file\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'w':
				args->threads_n = atoi(optarg);
				if (args->threads_n < 1) {
					fprintf(stderr, "Error: Invalid number of threads\n");
					exit(EXIT_FAILURE);
				} 
				break;
			case 'p':
				args->server_port = atoi(optarg);
				break;
			case 's':
				args->server_ip = optarg;
				break;
			default:    /* ? */
				fprintf(stderr, "Usage: %s [-q queryFile] [-w numThreads] [-sp serverPort] [-sip serverIP]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}
}