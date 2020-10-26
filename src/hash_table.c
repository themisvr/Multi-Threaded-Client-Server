#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bucket.h"
#include "hash_table.h"


hashTable *hash_table_init(size_t buckets_num, size_t bucket_size,  dataCompareFunction data_cmp, 
                                          typeAllocatorFunction type_alloc, 
                                          typeDeallocatorFunction type_dealloc) {

    hashTable *htable = malloc(sizeof(struct hash_table));
    if (!htable) {
        fprintf(stderr, "Error: Could not allocate memory for hash table\n");
        exit(EXIT_FAILURE);
    }
    htable->buckets_num = buckets_num;
    htable->entries_per_bucket = (bucket_size - sizeof(Bucket)) / sizeof(HTItem *);
    htable->bucket_size = bucket_size;
    htable->table = malloc(buckets_num * sizeof(doubleLinkedList *));
    if (!htable->table) {
        fprintf(stderr, "Error: Could not allocate memory for table\n");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i != buckets_num; i++) {
        htable->table[i] = list_create(data_cmp, type_alloc, type_dealloc);
        Bucket *bucket = bucket_init(htable->entries_per_bucket);
        list_last_insert(htable->table[i], bucket);
    }   
    return htable;
}

static inline int string_hash(const char *key, size_t buckets) {  
    int hv = 0, a = 33, p = 5381;

    while (*key != '\0') {
        hv = ( (hv * a) + *key ) % p;
        key++;
    }
    return (hv % buckets);
}


avl_tree *hash_table_key_search(hashTable *htable, const char *key) {   
    int hv = string_hash(key, htable->buckets_num);
    doubleLinkedList *search_chain = htable->table[hv];
    listNode *node = NULL;
    for (node = search_chain->head; node != NULL; node = node->next) {
        Bucket *bucket = list_get_item(search_chain, node);
        HTItem *ht_item = bucket_key_exists(bucket, key);
        if (ht_item != NULL)
            return ht_item->avl_tree_val;
    }
    return NULL;
}


void hash_table_insert(hashTable *htable, const char *key, Record *record, const char *date_file, const char *country) {
    int hv = string_hash(key, htable->buckets_num);
    listNode *lastNode = list_last_node(htable->table[hv]);
    Bucket *bucket = (Bucket *)list_get_item(htable->table[hv], lastNode);
    if (bucket_is_full(bucket, htable->entries_per_bucket)) {
        Bucket *new_bucket = bucket_init(htable->entries_per_bucket);
        bucket_data_insert(new_bucket, key, record, date_file, country);
        list_last_insert(htable->table[hv], new_bucket);
        return;
    }
    bucket_data_insert(bucket, key, record, date_file, country);
} 

void hash_table_free(hashTable *htable) {  
    for (size_t i = 0; i != htable->buckets_num; ++i) {
        //first destroy each chain for every bucket
        list_destroy(htable->table[i]);
    }
    free(htable->table);
    free(htable);
}
