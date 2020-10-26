#ifndef _DATA_ALLOCATOR_H
#define _DATA_ALLOCATOR_H


void *bucket_data_alloc(void *bucket_data) __attribute__ ((nonnull (1)));
void bucket_data_dealloc(void *bucket_data) __attribute__ ((nonnull (1)));

void record_dealloc(void *record_data) __attribute__ ((nonnull (1)));

void *list_data_alloc(void *list_data) __attribute__ ((nonnull (1)));
void list_data_dealloc(void *list_data) __attribute__ ((nonnull (1)));

void *tree_data_alloc(void *data) __attribute__ ((nonnull (1)));
void tree_data_dealloc(void *data) __attribute__ ((nonnull (1)));

void *key_value_info_alloc(void *info) __attribute__ ((nonnull (1)));
void key_value_info_dealloc(void *info) __attribute__ ((nonnull (1)));

void *country_count_alloc(void *data)  __attribute__ ((nonnull (1)));
void country_count_dealloc(void *data) __attribute__ ((nonnull (1)));

void *worker_info_alloc(void *data) __attribute__ ((nonnull (1)));
void worker_info_dealloc(void *data) __attribute__ ((nonnull (1)));


#endif // _DATA_ALLOCATOR_H