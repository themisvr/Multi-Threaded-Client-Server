#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "hash_table.h"
#include "bucket.h"
#include "utils.h"
#include "compare_funcs.h"
#include "avl.h"


static size_t check_date_bounds(avl_tree_node *node, time_t date1, time_t date2) {	
	if (!node) { return 0; }

	avl_data *data = (avl_data *)node->data;
	time_t entry_date = date_to_seconds((char *)data->entry_file);

	int res = strcmp((char *)((Record *)data->record)->status, "ENTER");

	if (date1 == entry_date && date2 == entry_date && res == 0) {
		return 1;
	}

	if (entry_date >= date1 && entry_date <= date2 && res == 0) {
		return 1 + check_date_bounds(node->left, date1, date2) + check_date_bounds(node->right, date1, date2);
	}
	else if (entry_date < date1)
		return check_date_bounds(node->right, date1, date2);
	else
		return check_date_bounds(node->left, date1, date2);
}


static void check_country(avl_tree_node *node, const char *country, time_t date1, time_t date2, size_t *counter) {	
	if (!node) { return; }
	check_country(node->left, country, date1, date2, counter);
	avl_data *data = (avl_data *)node->data;
	Record *record = (Record *)data->record;
	int res = strcmp(record->status, "ENTER");
	time_t entry_date = date_to_seconds((char *)data->entry_file);
	if ((res == 0 && strcmp(data->country, country) == 0) && entry_date >= date1 && entry_date <= date2) {
		(*counter)++;
	}
	check_country(node->right, country, date1, date2, counter);
}


size_t disease_frequency(char *command, hashTable *htable) {
	char *virus, *date1, *date2, *country;
	char *end_tok;

	virus = strtok_r(command, " \t\n", &end_tok);
	date1 = strtok_r(NULL, " \t\n", &end_tok);
	date2 = strtok_r(NULL, " \t\n", &end_tok);
	country = strtok_r(NULL, " \t\n", &end_tok);

	avl_tree *avltree_disease = hash_table_key_search(htable, virus);
	if (!avltree_disease) {
		//printf("Error: Could not find the virus with the name [%s]\n", virus);
		return -1;
	}
	size_t counter = 0;
	time_t date_1 = date_to_seconds(date1);
	time_t date_2 = date_to_seconds(date2);
	if (!country) {
		counter = check_date_bounds(avltree_disease->root, date_1, date_2);
	}
	else {
		check_country(avltree_disease->root, country, date_1, date_2, &counter);
	}
	return counter;
}

static inline uint8_t check_age(uint8_t age) {
	if ((age > 0) && (age <= 20)) {
		return 0;
	}
	else if ((age >= 21) && (age <= 40)) {
		return 1;
	}
	else if ((age >= 41) && (age <= 60)) {
		return 2;
	}
	else {
		return 3;
	}
}


typedef struct age_ranges {
	size_t counter;
	char *range;
} age_ranges;


static void country_disease_ages(avl_tree_node *node, const char *country, struct age_ranges *ages, time_t date1, time_t date2) {
	if (!node) { return; }
	country_disease_ages(node->left, country, ages, date1, date2);
	avl_data *data = (avl_data *)node->data;
	Record *record = (Record *)data->record;
	int res = strcmp(record->status, "ENTER");
	time_t entry_date = date_to_seconds((char *)data->entry_file);
	if ((res == 0 && strcmp(data->country, country) == 0) && entry_date >= date1 && entry_date <= date2) {
		uint8_t res = check_age(record->age);
		ages[res].counter++;
	}
	country_disease_ages(node->right, country, ages, date1, date2);	
}


static inline int counter_cmp(const void *a, const void *b) {
	struct age_ranges count1 = *(age_ranges *)a;
	struct age_ranges count2 = *(age_ranges *)b;
	return (count1.counter - count2.counter); 
}


char *topk_age_ranges(char *command, hashTable *disease_ht) {
	char *k, *country, *disease, *date1, *date2;
	char *end_tok;

	k = strtok_r(command, " \t\n", &end_tok);
	country = strtok_r(NULL, " \t\n", &end_tok);
	disease = strtok_r(NULL, " \t\n", &end_tok);
	date1 = strtok_r(NULL, " \t\n", &end_tok);
	date2 = strtok_r(NULL, " \t\n", &end_tok);

	avl_tree *avltree_disease = hash_table_key_search(disease_ht, disease);
	if (!avltree_disease) {
		//printf("Error: Could not find the disease with the name [%s]\n", disease);
		return NULL;
	}
	
	age_ranges *ages = malloc(4 * sizeof(struct age_ranges));
	if (!ages) {
		fprintf(stderr, "Error: Could not allocate memory for ages array\n");
		exit(EXIT_FAILURE);
	}
	for (uint8_t i = 0; i != 4; ++i) {
		ages[i].counter = 0;
	}
	ages[0].range = strdup("0-20");
	ages[1].range = strdup("21-40");
	ages[2].range = strdup("41-60");
	ages[3].range = strdup("60+");
		
	country_disease_ages(avltree_disease->root, country, ages, date_to_seconds(date1), date_to_seconds(date2));

	qsort(ages, 4, sizeof(age_ranges), counter_cmp);

	size_t total = 0;
	for (int i = 0; i != 4; ++i) {
		total += ages[i].counter;
	}

	char *msg = malloc(1);
	if (!msg) {
		fprintf(stderr, "Error: Could not allocate memory for msg\n");
		exit(EXIT_FAILURE);
	}
	memset(msg, '\0', 1);

	size_t total_s = 0;
	int index = 4 - atoi(k);
	for (int i = 3; i != index-1; --i) {
		if (ages[i].counter != 0) {
			double percentage = 100.0*ages[i].counter/total;
			char *res = malloc(20);
			memset(res, '\0', 20);
			snprintf(res, 20, "%s: %.0f%%\n", ages[i].range, percentage);
			total_s += strlen(res) + 2;
			msg = realloc(msg, total_s);
			strncat(msg, res, strlen(res));
			free(res);
		}
	}
	msg[strlen(msg)] = '\0';
	for (int i = 0; i != 4; ++i) {
		free(ages[i].range);
	}
	free(ages);
	
	return msg;
}


char *search_patient_record(char *command, doubleLinkedList *rec_list) {
	char *rec_id, *end_tok;

	rec_id = strtok_r(command, " \t\n", &end_tok);

	listNode *found = list_data_search(rec_list, rec_id);
	if (found) {
		list_rec *data = (list_rec *)list_get_item(rec_list, found);
		// add space for " " (spaces)
		size_t rec_size = 10 + strlen(data->record->record_id) + strlen(data->record->patient_fn) + strlen(data->record->patient_ln)
					 + strlen(data->record->disease) + strlen(data->entry_date) + strlen(data->exit_date);
		char *res = malloc(rec_size + 1);
		snprintf(res, rec_size, "%s %s %s %s %d %s %s", data->record->record_id, data->record->patient_fn, data->record->patient_ln, data->record->disease, data->record->age, data->entry_date, data->exit_date);
		res[strlen(res)] = '\0';
		return res;
	}
	return NULL;
}


static void admissions_per_country(avl_tree_node *node, doubleLinkedList *country_list, time_t date1, time_t date2) {
	if (!node) { return; }
	admissions_per_country(node->left, country_list, date1, date2);
	avl_data *data = (avl_data *)node->data;
	Record *record = (Record *)data->record;
	int res = strcmp(record->status, "ENTER");
	time_t entry_date = date_to_seconds((char *)data->entry_file);
	if (res == 0 && entry_date >= date1 && entry_date <= date2) {
		listNode *exists = list_data_search(country_list, data->country);
		if (exists) {
			country_counts *info = (country_counts *)list_get_item(country_list, exists);
			info->counter++;
		}
		else {
			country_counts *info = malloc(sizeof(country_counts));
			if (!info) {
				fprintf(stderr, "Error: Could not allocate memory for info\n");
				exit(EXIT_FAILURE);
			}
			info->counter = 1;
			info->country = strdup(data->country);
			list_last_insert(country_list, info);
		}
	}
	admissions_per_country(node->right, country_list, date1, date2);	
}


char *num_patient_admissions(char *command, hashTable *htable) {
	char *virus, *date1, *date2, *country;
	char *end_tok;
	char *msg;

	virus = strtok_r(command, " \t\n", &end_tok);
	date1 = strtok_r(NULL, " \t\n", &end_tok);
	date2 = strtok_r(NULL, " \t\n", &end_tok);
	country = strtok_r(NULL, " \t\n", &end_tok);

	avl_tree *avltree_disease = hash_table_key_search(htable, virus);
	if (!avltree_disease) {
		//printf("Error: Could not find the virus with the name [%s]\n", virus);
		return NULL;
	}
	size_t counter = 0;
	time_t date_1 = date_to_seconds(date1);
	time_t date_2 = date_to_seconds(date2);

	msg = malloc(1);
	if (!msg) {
		fprintf(stderr, "Error: Could not allocate memory for msg\n");
		exit(EXIT_FAILURE);
	}
	memset(msg, '\0', 1);

	size_t total_s = 0;
	if (!country) {
		doubleLinkedList *country_list = list_create(country_cmp, country_count_alloc, country_count_dealloc);
		admissions_per_country(avltree_disease->root, country_list, date_1, date_2);
		for (listNode *start = country_list->head; start != NULL; start = start->next) {
			country_counts *info = (country_counts *)list_get_item(country_list, start);
			char *res = malloc(20);
			memset(res, '\0', 20);
			snprintf(res, 20, "%s %ld\n", info->country, info->counter);
			total_s += strlen(res) + 2;
			msg = realloc(msg, total_s);
			strncat(msg, res, strlen(res));
			free(res);
		}
		list_destroy(country_list);
	}
	else {
		check_country(avltree_disease->root, country, date_1, date_2, &counter);
		if (counter > 0) {
			msg = realloc(msg, strlen(country) + 10);
			snprintf(msg, strlen(country) + 10, "%s %ld\n", country, counter);
		}
		else {
			free(msg); 
			return NULL; 
		}
	}
	msg[strlen(msg)] = '\0';
	return msg;
}


static void discharges_per_country(avl_tree_node *node, doubleLinkedList *country_list, time_t date1, time_t date2) {
	if (!node) { return; }
	discharges_per_country(node->left, country_list, date1, date2);
	avl_data *data = (avl_data *)node->data;
	int res = strcmp(data->exit_file, "--");
	time_t exit_date = date_to_seconds((char *)data->exit_file);
	if (res != 0 && exit_date >= date1 && exit_date <= date2) {
		listNode *exists = list_data_search(country_list, data->country);
		if (exists) {
			country_counts *info = (country_counts *)list_get_item(country_list, exists);
			info->counter++;
		}
		else {
			country_counts *info = malloc(sizeof(country_counts));
			if (!info) {
				fprintf(stderr, "Error: Could not allocate memory for info\n");
				exit(EXIT_FAILURE);
			}
			info->counter = 1;
			info->country = strdup(data->country);
			list_last_insert(country_list, info);
		}
	}
	discharges_per_country(node->right, country_list, date1, date2);	
}


static void check_discharges(avl_tree_node *node, const char *country, time_t date1, time_t date2, size_t *counter) {
	if (!node) { return; }
	check_discharges(node->left, country, date1, date2, counter);
	avl_data *data = (avl_data *)node->data;
	int res = strcmp(data->exit_file, "--");
	time_t entry_date = date_to_seconds((char *)data->entry_file);
	if ((res != 0 && strcmp(data->country, country) == 0) && entry_date >= date1 && entry_date <= date2) {
		(*counter)++;
	}
	check_discharges(node->right, country, date1, date2, counter);
}

char *num_patient_discharges(char *command, hashTable *htable) {
	char *virus, *date1, *date2, *country;
	char *end_tok;
	char *msg;

	virus = strtok_r(command, " \t\n", &end_tok);
	date1 = strtok_r(NULL, " \t\n", &end_tok);
	date2 = strtok_r(NULL, " \t\n", &end_tok);
	country = strtok_r(NULL, " \t\n", &end_tok);

	avl_tree *avltree_disease = hash_table_key_search(htable, virus);
	if (!avltree_disease) {
		//printf("Error: Could not find the virus with the name [%s]\n", virus);
		return NULL;
	}
	size_t counter = 0;
	time_t date_1 = date_to_seconds(date1);
	time_t date_2 = date_to_seconds(date2);

	msg = malloc(1);
	if (!msg) {
		fprintf(stderr, "Error: Could not allocate memory for msg\n");
		exit(EXIT_FAILURE);
	}
	memset(msg, '\0', 1);

	size_t total_s = 0;
	if (!country) {
		doubleLinkedList *country_list = list_create(country_cmp, country_count_alloc, country_count_dealloc);
		discharges_per_country(avltree_disease->root, country_list, date_1, date_2);
		for (listNode *start = country_list->head; start != NULL; start = start->next) {
			country_counts *info = (country_counts *)list_get_item(country_list, start);
			char *res = malloc(20);
			memset(res, '\0', 20);
			snprintf(res, 20, "%s %ld\n", info->country, info->counter);
			total_s += strlen(res) + 2;
			msg = realloc(msg, total_s);
			strncat(msg, res, strlen(res));
			free(res);
		}
		list_destroy(country_list);
	}
	else {
		check_discharges(avltree_disease->root, country, date_1, date_2, &counter);
		if (counter > 0) {
			msg = realloc(msg, strlen(country) + 10);
			snprintf(msg, strlen(country) + 10, "%s %ld\n", country, counter);
		}
		else {
			free(msg); 
			return NULL; 
		}
	}
	msg[strlen(msg)] = '\0';
	return msg;
}