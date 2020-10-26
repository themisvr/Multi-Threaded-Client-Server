#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bucket.h"
#include "compare_funcs.h"
#include "alloc_funcs.h"


Bucket *bucket_init(size_t num_of_entries) {
	Bucket *bucket = malloc(sizeof(Bucket));
	if (!bucket) {
		fprintf(stderr, "Error: Could not allocate memory for bucket\n");
		exit(EXIT_FAILURE);
	}
	bucket->ht_item = malloc(num_of_entries * sizeof(HTItem *));
	if (!bucket->ht_item) {
		fprintf(stderr, "Error: Could not allocate memory for ht_item\n");
		exit(EXIT_FAILURE);
	}
	memset(bucket->ht_item, '\0', num_of_entries * sizeof(HTItem *));
	bucket->ith_index = 0;
	return bucket;
}

void bucket_data_delete(HTItem *ht_item) {
	free(ht_item->key);
	avl_free(ht_item->avl_tree_val);
	free(ht_item);
}

HTItem *bucket_key_exists(Bucket *bucket, const char *key) {
	for (size_t i = 0; i != bucket->ith_index; ++i) {
		HTItem *ht_info = bucket->ht_item[i];
		if (strncmp(ht_info->key, key, strlen(key)) == 0) {
			return ht_info;
		}
	}
	return NULL;
}

void bucket_free(Bucket *bucket) {
	for (size_t i = 0; i != bucket->ith_index; i++) {
		bucket_data_delete(bucket->ht_item[i]);
	}
	free(bucket->ht_item);
	free(bucket);
}

uint8_t bucket_is_full(Bucket *bucket, size_t entries_per_bucket) {
	return bucket->ith_index == entries_per_bucket;
}

static inline HTItem *ht_item_init(const char *key) {	
	size_t key_length = strlen(key);
	HTItem *ht_item = malloc(sizeof(HTItem));
	if (!ht_item) {
		fprintf(stderr, "Error: Could not allocate memory for ht_item\n");
		exit(EXIT_FAILURE);
	}
	ht_item->key = malloc(key_length + 1);
	memcpy(ht_item->key, key, key_length);
	ht_item->key[key_length] = '\0';
	ht_item->avl_tree_val = avl_init(tree_data_alloc, tree_data_dealloc, avl_id_cmp);
	return ht_item;
}


void bucket_data_insert(Bucket *bucket, const char *key, Record *record, const char *date_file, const char *country) {
	HTItem *ht_item = bucket_key_exists(bucket, key);
	if (ht_item == NULL) {
		ht_item = ht_item_init(key);
		bucket->ht_item[bucket->ith_index] = ht_item;
		bucket->ith_index++;
	}
	avl_tree *avltree = ht_item->avl_tree_val;
	avl_data *tree_data = malloc(sizeof(avl_data));
	if (!tree_data) {
		fprintf(stderr, "Error: Could not allocate memory for tree_data\n");
		exit(1);
	}
	tree_data->record = record;
	tree_data->entry_file = strdup(date_file);
	tree_data->exit_file = strdup("--");
	tree_data->country = strdup(country);
	avl_data_insert(avltree, tree_data);
}