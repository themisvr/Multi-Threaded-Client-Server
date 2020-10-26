#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>
#include <stdint.h>

#include "record.h"
#include "avl.h"
#include "generic_list.h"


typedef struct hash_table {
    doubleLinkedList **table;
    size_t buckets_num;
    size_t bucket_size;
    size_t entries_per_bucket;
} hashTable;

// creates an empty hash table
hashTable *hash_table_init(size_t buckets_num, size_t bucket_size, dataCompareFunction data_cmp, 
										typeAllocatorFunction type_alloc, 
                                        typeDeallocatorFunction type_dealloc);

avl_tree *hash_table_key_search(hashTable *htable, const char *key) __attribute__((nonnull (1, 2)));

// Inserts a key (with its item) in the hash (replace if exists)
void hash_table_insert(hashTable *htable, const char *key, Record *record, const char *date_file, const char *country) __attribute__((nonnull (1, 2, 3, 4, 5)));

// removes the key from the hash
void hash_table_entry_delete(hashTable *htable, const char *key) __attribute__((nonnull (1, 2)));

// Destroys the hash by freeing all reserved memory.
void hash_table_free(hashTable *htable) __attribute__((nonnull (1)));

#endif // HASH_TABLE_H