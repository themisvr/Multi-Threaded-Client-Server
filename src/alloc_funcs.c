#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "alloc_funcs.h"
#include "bucket.h"
#include "record.h"
#include "key_value.h"
#include "server_utils.h"


void *bucket_data_alloc(void *bucket_data) {
	return bucket_data;
}


void bucket_data_dealloc(void *bucket_data) {
	Bucket *bucket = (Bucket *)bucket_data;
	bucket_free(bucket);
}


void *list_data_alloc(void *list_data) {
	return list_data;
}


void record_dealloc(void *record_data) {
	free(((Record *)record_data)->record_id);
	free(((Record *)record_data)->status);
	free(((Record *)record_data)->patient_fn);
	free(((Record *)record_data)->patient_ln);
	free(((Record *)record_data)->disease);
	free(record_data);
}


void list_data_dealloc(void *list_data) {
	list_rec *data = (list_rec *)list_data;
	record_dealloc(data->record);
	free(data->entry_date);
	free(data->exit_date);
	free(data);
}


void *tree_data_alloc(void *data) {
	return data;
}


void tree_data_dealloc(void *data) {
	avl_data *tree_data = (avl_data *)data;
	free(tree_data->entry_file);
	free(tree_data->exit_file);
	free(tree_data->country);
	free(tree_data);
}


void *key_value_info_alloc(void *info) {
	return info;
}


void key_value_info_dealloc(void *info) {
	free(((key_value_info *)info)->key);
	free(info);
}


void *country_count_alloc(void *data) {
	return data;
}


void country_count_dealloc(void *data) {
	country_counts *info = (country_counts *)data;
	free(info->country);
	free(info);
}


void *worker_info_alloc(void *data) {
	return data;
}


void worker_info_dealloc(void *data) {
	worker_info *infos = (worker_info *)data;
	free(infos->worker_ip);
	free(infos);
}