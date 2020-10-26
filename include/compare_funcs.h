#ifndef COMPARE_FUNCTIONS_H
#define COMPARE_FUNCTIONS_H

#include <stdlib.h>
#include <string.h>

#include "record.h"
#include "bucket.h"
#include "utils.h"
#include "key_value.h"


static inline ssize_t avl_id_cmp(void *data_a, void *data_b) {
	
	avl_data *data1 = (avl_data *)data_a;
	avl_data *data2 = (avl_data *)data_b;
	return strcmp(data1->record->record_id, data2->record->record_id);
}


static inline int id_cmp(const void *data_a, const void *data_b) {

	char *rec_a = (char *)data_a;
	list_rec *rec_b = (list_rec *)data_b;
	return strcmp(rec_a, rec_b->record->record_id); 
}


static inline int disease_cmp(const void *data_a, const void *data_b) {
	
	char *key = (char *)data_a;
	key_value_info *info_b = (key_value_info *)data_b;
	return strcmp(key, info_b->key);
}


static inline int country_cmp(const void *a, const void *b) {

	char *data_a = (char *)a;
	country_counts *data_b = (country_counts *)b;
	return strcmp(data_a, data_b->country);
}


static inline int port_cmp(const void *a, const void *b) {

	int port_a = *(int *)a;
	int port_b = *(int *)b;
	return port_a - port_b;
}

#endif // COMPARE_FUNCTIONS_H