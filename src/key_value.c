#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "key_value.h"


hash_set *hash_set_init(size_t n_buckets, dataCompareFunction data_cmp, typeAllocatorFunction type_alloc, 
                                        typeDeallocatorFunction type_dealloc) {

	hash_set *set = malloc(sizeof(hash_set));
	if (!set) {
		fprintf(stderr, "Error: Could not allocate memory for hash set\n");
		exit(EXIT_FAILURE);
	}
	set->n_buckets = n_buckets;
	set->table = malloc(n_buckets * sizeof(doubleLinkedList *));
	if (!set->table) {
		fprintf(stderr, "Error: Could not allocate memory for the set table\n");
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i != n_buckets; ++i) {
		set->table[i] = list_create(data_cmp, type_alloc, type_dealloc);
	}
	return set;
}


static inline int string_hash(const char *key, size_t buckets) {   
    int hv = 0, a = 33, p = 5381;

    while (*key != '\0') {
        hv = ( (hv * a) + *key ) % p;
        key++;
    }
    return (hv % buckets);
}


key_value_info *hash_set_key_search(hash_set *set, const char *key) {
	int hv = string_hash(key, set->n_buckets);
	if (!set->table[hv]) {
		//printf("Could not fine value with key [%s]\n", key);
		return NULL;
	}
	doubleLinkedList *chain = set->table[hv];
	listNode *found = list_data_search(chain, key);
	if (found) {
		key_value_info *info = (key_value_info *)found->data;
		return info;
	}
	else return NULL;
}


static inline void check_age_range(uint8_t age, key_value_info *info) {
	if ((age > 0) && (age <= 20)) {
		info->first_r++;
	}
	else if ((age >= 21) && (age <= 40)) {
		info->second_r++;
	}
	else if ((age >= 41) && (age <= 60)) {
		info->third_r++;
	}
	else {
		info->fourth_r++;
	}
}

void hash_set_insert(hash_set *set, const char *key, uint8_t age) {
	int hv = string_hash(key, set->n_buckets);
	key_value_info *info;
	if ((info = hash_set_key_search(set, key)) != NULL) {
		check_age_range(age, info);
		return; 
	}
	info = malloc(sizeof(key_value_info));
	if (!info) {
		fprintf(stderr, "Error: Could not allocate memory for the key_value_info\n");
		exit(EXIT_FAILURE);
	}
	info->key = strdup(key);
	info->first_r = 0;
	info->second_r = 0;
	info->third_r = 0;
	info->fourth_r = 0;
	check_age_range(age, info);
	list_last_insert(set->table[hv], info); 
}


void hash_set_free(hash_set *set) {
    for (size_t i = 0; i != set->n_buckets; ++i) {
        //first destroy each chain for every bucket
        list_destroy(set->table[i]);
    }
    free(set->table);
    free(set);
}