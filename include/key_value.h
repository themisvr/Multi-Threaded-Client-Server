#ifndef KEY_VALUE_H
#define KEY_VALUE_H

#include <stdint.h>

#include "generic_list.h"


typedef struct key_value_info {
	uint32_t first_r;
	uint32_t second_r;
	uint32_t third_r;
	uint32_t fourth_r; 
	char *key;
} key_value_info;


typedef struct hash_set {
	doubleLinkedList **table;
	size_t n_buckets;
} hash_set;


hash_set *hash_set_init(size_t n_buckets, dataCompareFunction data_cmp, typeAllocatorFunction type_alloc, 
                                        typeDeallocatorFunction type_dealloc);

key_value_info *hash_set_key_search(hash_set *set, const char *key) __attribute__ ((nonnull (1, 2)));

void hash_set_insert(hash_set *set, const char *key, uint8_t age)  __attribute__ ((nonnull (1, 2)));

void hash_set_free(hash_set *set)  __attribute__ ((nonnull (1)));


#endif // KEY_VALUE_H