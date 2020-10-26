#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include "worker_utils.h"
#include "file_handler.h"
#include "hash_table.h"
#include "alloc_funcs.h"
#include "named_pipes.h"
#include "key_value.h"
#include "commands.h"
#include "compare_funcs.h"
#include "report_errors.h"


static inline Record *record_info_create(char *record_line) {
	
	Record *record = malloc(sizeof(Record));
	if (!record) {
		fprintf(stderr, "Error: Could not allocate memory for record\n");
		exit(EXIT_FAILURE);
	}
	char *id, *status, *first, *last, *disease, *age;
	char *end;

	if ((id = strtok_r(record_line, " \t\n", &end)) == NULL) { return NULL; } 
	record->record_id = strdup(id);
	
	if ((status = strtok_r(NULL, " \t\n", &end)) == NULL) { return NULL; }
	record->status = strdup(status);

	if ((first = strtok_r(NULL, " \t\n", &end)) == NULL) { return NULL; }
	record->patient_fn = strdup(first);

	if ((last = strtok_r(NULL, " \t\n", &end)) == NULL) { return NULL; }
	record->patient_ln = strdup(last);

	if ((disease = strtok_r(NULL, " \t\n", &end)) == NULL) { return NULL; }
	record->disease = strdup(disease);

	if ((age = strtok_r(NULL, " \t\n", &end)) == NULL) { return NULL; }
	record->age = atoi(age);

	return record;
}


static inline void change_exit_value(Record *record, hashTable *htable, char *date_file) {

	avl_tree *tree = hash_table_key_search(htable, record->disease);
	if (!tree) { return; }
	avl_data *dummy_search = malloc(sizeof(avl_data));
	if (!dummy_search) {
		fprintf(stderr, "Error: Could not allocate memory for dummy_search\n");
		exit(EXIT_FAILURE);
	}
	dummy_search->record = record;
	avl_tree_node *exists = avl_data_search(tree, dummy_search);
	if (exists) {
		avl_data *info = (avl_data *)exists->data;
		if (strncmp(info->exit_file, "--", strlen("--")) == 0) {
			free(info->exit_file);
			info->exit_file = strdup(date_file);
		}
		free(dummy_search);
	}
}


static inline int validate_record(Record *record, doubleLinkedList *gen_list, hashTable *htable, char *date_file) {

	int valid = 1;
	if (record->age <= 0 || record->age > 120) {
		valid = -1;
	}
	if (!list_is_empty(gen_list)) {
		listNode *exists;
		if ((exists = list_data_search(gen_list, record->record_id)) != NULL) {
			list_rec *data = (list_rec *)list_get_item(gen_list, exists);
			// same id with same status
			if (strcmp(record->status, data->record->status) == 0) {
				valid = -1;
			}
			// the record with the same id but different status is valid
			else {
				if ((strncmp(record->status, "EXIT", strlen("EXIT")) == 0) && (strncmp(data->exit_date, "--", strlen("--")) == 0)) {
					if (date_to_seconds(date_file) < date_to_seconds(data->entry_date)) {
						valid = -1;
					}
					else {
						free(data->exit_date);
						data->exit_date = strdup(date_file);
						change_exit_value(record, htable, date_file);
						valid = 0;
					}
				}
			}
		}
	}
	return valid;
}


char *construct_stats(const char *dirname, char *filename, hash_set *set) {

	char *stats = malloc(5000);
	if (!stats) {
		fprintf(stderr, "Error: Could not allocate memory for stats\n");
		exit(EXIT_FAILURE);
	}
	memset(stats, '\0', 5000);

	uint8_t newl_s = strlen("\n"); 
	strncat(stats, filename, strlen(filename));
	strncat(stats, "\n", newl_s);
	strncat(stats, dirname, strlen(dirname));
	strncat(stats, "\n", newl_s);

	for (size_t i = 0; i != set->n_buckets; ++i) {
		doubleLinkedList *chain = set->table[i];
		for (listNode *node = chain->head; node != NULL; node = node->next) {
			key_value_info *info = list_get_item(chain, node);
			char temp[50];
			strncat(stats, info->key, strlen(info->key));
			strncat(stats, "\n", newl_s);
			sprintf(temp, "Age range 0-20 years: %d cases\n", info->first_r);
			strncat(stats, temp, strlen(temp));
			sprintf(temp, "Age range 21-40 years: %d cases\n", info->second_r);
			strncat(stats, temp, strlen(temp));
			sprintf(temp, "Age range 41-60 years: %d cases\n", info->third_r);
			strncat(stats, temp, strlen(temp));
			sprintf(temp, "Age range 60+ years: %d cases\n\n", info->fourth_r);
			strncat(stats, temp, strlen(temp));
		}
	}
	stats[strlen(stats)] = '\0';
	return stats;
}


static inline char *parse_path(char *filename) {

	char *tok = NULL, *prev = NULL, *end;
	tok = strtok_r(filename, "/\n", &end);
	while (tok != NULL) {
		prev = tok;
		tok = strtok_r(NULL, "/\n", &end);
	}
	return prev;
}


static inline int date_cmp(const void *a, const void *b) {
   	const char *pa = *(const char **)a;
   	char *path1 = strdup(pa);
   	char *date1 = parse_path(path1);
	const char *pb = *(const char **)b;
	char *path2 = strdup(pb);
	char *date2 = parse_path(path2);
	int val1 = date_to_seconds(date1);
	int val2 = date_to_seconds(date2);
	
	free(path1);
	free(path2);
	return (val1 - val2);
}


char *insert_records(hashTable *htable, doubleLinkedList *gen_list, char *filename, char *dirname) {

	FILE *fp;
	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Error: Could not open filename for reading\n");
		exit(EXIT_FAILURE);
	}
	uint8_t stat_flag = 0;
	ssize_t nread;
	size_t len = 0;
	char *rec_line = NULL;

	hash_set *set = hash_set_init(N_BUCKETS, disease_cmp, key_value_info_alloc, key_value_info_dealloc);

	char *date_file = parse_path(filename);
	char *nc = strdup(dirname);
	char *country = parse_path(nc);

	while ((nread = getline(&rec_line, &len, fp)) != -1) {
		Record *record = record_info_create(rec_line);
		if (!record) {
			fprintf(stderr, "ERROR\n");
			continue;
		}
		/* check for age, dates, valid id */
		int valid = validate_record(record, gen_list, htable, date_file);
		if (valid == 1) {
			list_rec *data = malloc(sizeof(list_rec));
			if (!data) {
				fprintf(stderr, "Error: Could not allocate memory for list record\n");
				exit(EXIT_FAILURE);
			}
			data->record = record;
			data->entry_date = strdup(date_file);
			data->exit_date = strdup("--");
			list_last_insert(gen_list, data);
			hash_table_insert(htable, record->disease, record, date_file, country);
			if (strncmp(record->status, "ENTER", strlen("ENTER")) == 0) {
				stat_flag = 1;
				hash_set_insert(set, record->disease, record->age);
			}
		}
		else {
			if (valid == -1) {
				fprintf(stderr, "ERROR\n");
			}
			record_dealloc(record);
			continue; 
		}
	}
	// if stats == NULL then the file we just read was full of EXIT records
	char *stats = NULL;
	if (stat_flag) {
		stats = construct_stats(country, date_file, set);
	}

	hash_set_free(set);
	free(rec_line);
	free(nc);
	fclose(fp);
	return stats;
}


char *read_info_from_master(int readfd, size_t bytes_in_fifo, size_t *port_n, char *server_ip,
							hashTable *htable, doubleLinkedList *gen_list) {

	char *dir_name = NULL;
	char *tok;

	char *infos = named_pipe_read(readfd, bytes_in_fifo);

	char *bigger_picture = malloc(1);
	if (!bigger_picture) {
		fprintf(stderr, "Error: Could not allocate memory for bigger_picture\n");
		exit(EXIT_FAILURE);
	}
	memset(bigger_picture, '\0', 1);
	size_t total = 0;

	strcpy(server_ip, strtok_r(infos, " \t\n", &tok));
	char *port = strtok_r(NULL, " \n\t", &tok);
	*port_n = atoi(port);

	dir_name = strtok_r(NULL, " \t\n", &tok);
	while (dir_name != NULL) {
		/* 
			if dir_name does not exists function prints error 
			otherwise it returns false if file is not type of S_IFDIR
		*/
		if (file_is_dir(dir_name)) {
		 	
		 	size_t n_files = n_files_in_directory(dir_name, "file");
			char **files = subdir_names(dir_name, n_files);
			qsort(files, n_files, sizeof(char *), date_cmp);
			
			for (size_t i = 0; i != n_files; ++i) {
			 	char *stats = insert_records(htable, gen_list, files[i], dir_name);
			 	if (stats) {
			 		total += strlen(stats) + 2;
			 		bigger_picture = realloc(bigger_picture, total);
			 		strncat(bigger_picture, stats, strlen(stats));
			 	}
				free(stats);
			}
			for (size_t i = 0; i != n_files; ++i) {
				free(files[i]);
			}
			free(files);
		}
		dir_name = strtok_r(NULL, " \t\n", &tok);
	}
	free(infos);

	return bigger_picture;
}


char *parse_commands(char *command, hashTable *htable, doubleLinkedList *gen_list) {

	char *token = NULL, *rem_tok;
	char *result = NULL;

	token = strtok_r(command, " \t\n", &rem_tok);
	if (token) {
		/* disease frequency command */
		if (strcmp(token, "/diseaseFrequency") == 0) {
			result = num_patient_admissions(rem_tok, htable);
		}
		/* top k age ranges command */
		if (strcmp(token, "/topk-AgeRanges") == 0) {
			result = topk_age_ranges(rem_tok, htable);
		}
		/* search patient record command */
		else if (strcmp(token, "/searchPatientRecord") == 0) {
			result = search_patient_record(rem_tok, gen_list);
		}
		/* num patient admissions command */
		else if (strcmp(token, "/numPatientAdmissions") == 0) {
			result = num_patient_admissions(rem_tok, htable);
		}
		/* num patient discharges command */
		else if (strcmp(token, "/numPatientDischarges") == 0) {
			result = num_patient_discharges(rem_tok, htable);
		}
		return result;
	}
}


void send_response_to_server(char *request, size_t sock_fd, hashTable *htable, doubleLinkedList *gen_list) {
	ssize_t n_write;
	size_t response_len;
	char *response = NULL;

	response = parse_commands(request, htable, gen_list);
	if (response == NULL || strlen(response) == 0) {
		response = strdup("NULL");
	}

	response_len = strlen(response);
	if ((n_write = write(sock_fd, &response_len, sizeof(size_t))) < 0)
		handle_error_en(n_write, "write");

	if ((n_write = write(sock_fd, response, response_len)) < 0)
		handle_error_en(n_write, "write");
}


void send_error_to_server(size_t sock_fd) {
	ssize_t n_write;
	size_t error_len;

	error_len = strlen("400 Bad Request\n");
	if ((n_write = write(sock_fd, &error_len, sizeof(size_t))) < 0)
		handle_error_en(n_write, "write");

	if ((n_write = write(sock_fd, "400 Bad Request\n", error_len)) < 0)
		handle_error_en(n_write, "write");
}


void connect_to_server(size_t server_port, char *server_ip, socket_t *worker_socket) {
	int ret;

	if ((ret = socket_create(server_port, inet_addr(server_ip), worker_socket)) < 0)
		handle_error_en(ret, "socket");
	
	if ((ret = socket_connect(worker_socket)) < 0)
		handle_error_en(ret, "connect");
}


void send_stats_to_server(char *stats, socket_t *worker_socket) {
	size_t sendbytes = 1024;
	size_t total_write = 0;
	ssize_t n_write = 0;
	char *temp = stats;

	size_t stats_len = strlen(stats);

	size_t send_stats_len = stats_len;
	if ((n_write = write(worker_socket->socket_fd, &send_stats_len, sizeof(size_t))) < 0) {
		if (errno != EINTR)
			handle_error_en(n_write, "write");
	}

	if (sendbytes > stats_len) {
		sendbytes = stats_len + 1;
	}
	while (total_write <= stats_len) {
		if ((n_write = write(worker_socket->socket_fd, temp, sendbytes)) < 0)
			handle_error_en(n_write, "write");
		total_write += n_write;
		if (total_write > stats_len) break;
		temp += sizeof(char) * n_write;
		if (total_write + sendbytes > stats_len) {
			sendbytes = (stats_len - total_write) + 1;
		}
	}
}


void send_worker_query_port(int stats_fd, socket_t *worker_server_socket) {
	ssize_t nwrite;
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);

	sin = worker_server_socket->address;
	if (getsockname(worker_server_socket->socket_fd, (struct sockaddr *)&sin, &len) < 0)
		handle_error("getsockname");
	
	uint16_t worker_port = ntohs(sin.sin_port);
	if ((nwrite = write(stats_fd, &worker_port, sizeof(uint16_t))) < 0)
		handle_error_en(nwrite, "write");
	
	if (close(stats_fd) < 0) 
		handle_error("close");
}


void setup_worker_as_server(int stats_fd, socket_t *worker_server_socket) {
	int ret = socket_create(0, ANY_ADDR, worker_server_socket);
	if (ret < 0) handle_error_en(ret, "socket");

	if (socket_bind(worker_server_socket) < 0) handle_error("bind");

	if (socket_listen(worker_server_socket) < 0) handle_error("listen");
	printf("Worker listening...\n");

	send_worker_query_port(stats_fd, worker_server_socket);

}


char *handle_request(size_t client_fd) {
	ssize_t n_read;
	size_t request_len = 0;
	char *request = NULL;

	if ((n_read = read(client_fd, &request_len, sizeof(size_t))) < 0)
		handle_error_en(n_read, "read");

	request = malloc(request_len + 1);
	if (!request) handle_error("malloc");
	memset(request, '\0', request_len + 1);

	if ((n_read = read(client_fd, request, request_len)) < 0)
		handle_error_en(n_read, "read");

	request[request_len] = '\0';

	return request;
}


int validate_request(char *request) {
	int valid;
	char *query;
	char *token, *rem_str;

	query = strdup(request);
	token = strtok_r(query, " \n\t", &rem_str);

	if (strncmp(token, "/diseaseFrequency", strlen("/diseaseFrequency")) == 0) {
		const char *virus, *date1, *date2;
		virus = strtok_r(NULL, " \n\t", &rem_str);
		if (virus == NULL) {
			free(query);
			return -1;
		}
		date1 = strtok_r(NULL, " \n\t", &rem_str);
		if (date1 == NULL) {
			free(query);
			return -1;
		}
		date2 = strtok_r(NULL, " \n\t", &rem_str);
		if (date2 == NULL) {
			free(query);
			return -1;
		}
		if (date_to_seconds(date1) > date_to_seconds(date2)) {
			free(query);
			return -1;
		}
		valid = 1;
	}
	else if (strncmp(token, "/topk-AgeRanges", strlen("/topk-AgeRanges")) == 0) {
		const char *k, *country, *disease, *date1, *date2;
		k = strtok_r(NULL, " \n\t", &rem_str);
		if (k == NULL) {
			free(query);
			return -1;
		}
		if (atoi(k) > 4 || atoi(k) < 1) {
			free(query);
			return -1;
		}
		country = strtok_r(NULL, " \n\t", &rem_str);
		if (country == NULL) {
			free(query);
			return -1;
		}
		disease = strtok_r(NULL, " \n\t", &rem_str);
		if (disease == NULL) {
			free(query);
			return -1;
		}	
		date1 = strtok_r(NULL, " \n\t", &rem_str);
		if (date1 == NULL) {
			free(query);
			return -1;
		}
		date2 = strtok_r(NULL, " \n\t", &rem_str);
		if (date2 == NULL) {
			free(query);
			return -1;
		}
		if (date_to_seconds(date1) > date_to_seconds(date2)) {
			free(query);
			return -1;
		}
		valid = 1;
	}
	else if (strncmp(token, "/searchPatientRecord", strlen("/searchPatientRecord")) == 0) {
		const char *record_id = strtok_r(NULL, " \n\t", &rem_str);
		if (record_id == NULL) {
			free(query);
			return -1;
		}
		valid = 1;
	}
	else if (strncmp(token, "/numPatientAdmissions", strlen("/numPatientAdmissions")) == 0) {
		const char *virus, *date1, *date2;
		virus = strtok_r(NULL, " \n\t", &rem_str);
		if (virus == NULL) {
			free(query);
			return -1;
		}
		date1 = strtok_r(NULL, " \n\t", &rem_str);
		if (date1 == NULL) {
			free(query);
			return -1;
		}
		date2 = strtok_r(NULL, " \n\t", &rem_str);
		if (date2 == NULL) {
			free(query);
			return -1;
		}
		if (date_to_seconds(date1) > date_to_seconds(date2)) {
			free(query);
			return -1;
		}
		valid = 1;
	}
	else if (strncmp(token, "/numPatientDischarges", strlen("/numPatientDischarges")) == 0) {
		const char *virus, *date1, *date2;
		virus = strtok_r(NULL, " \n\t", &rem_str);
		if (virus == NULL) {
			free(query);
			return -1;
		}
		date1 = strtok_r(NULL, " \n\t", &rem_str);
		if (date1 == NULL) {
			free(query);
			return -1;
		}
		date2 = strtok_r(NULL, " \n\t", &rem_str);
		if (date2 == NULL) {
			free(query);
			return -1;
		}
		if (date_to_seconds(date1) > date_to_seconds(date2)) {
			free(query);
			return -1;
		}
		valid = 1;
	}
	else {
		free(query);
		return -1;
	}
	free(query);
	return valid;
}