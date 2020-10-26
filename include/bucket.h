#ifndef BUCKET_H
#define BUCKET_H

#include "record.h"
#include "avl.h"


typedef struct HTItem {
	char *key;
	avl_tree *avl_tree_val;
} HTItem;


typedef struct bucket {
	HTItem **ht_item;
	size_t ith_index;
} Bucket;


typedef struct avl_data {
	Record *record;
	char *entry_file;
	char *exit_file;
	char *country;
} avl_data;


Bucket *bucket_init(size_t num_of_entries);

void bucket_free(Bucket *bucket) __attribute__((nonnull (1)));

uint8_t bucket_is_full(Bucket *bucket, size_t entries_per_bucket) __attribute__((nonnull (1)));

void bucket_data_insert(Bucket *bucket, const char *key, Record *record, const char *date_file, const char *country) __attribute__((nonnull (1, 2, 3, 4, 5)));

void bucket_data_delete(HTItem *ht_item) __attribute__((nonnull (1)));

HTItem *bucket_key_exists(Bucket *bucket, const char *key) __attribute__((nonnull (1, 2)));


#endif // BUCKET_H