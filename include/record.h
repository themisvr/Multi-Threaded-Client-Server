#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>

/* some type of structs that I needed */

typedef struct record {
	char *record_id;
	char *status;
	char *patient_fn;
	char *patient_ln;
	char *disease;
	uint8_t age;
} Record;


typedef struct list_rec {
	Record *record;
	char *entry_date;
	char *exit_date;
} list_rec;


typedef struct country_counts {
	char *country;
	size_t counter;
} country_counts;


#endif // RECORD_H