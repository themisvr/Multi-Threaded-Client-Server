#ifndef _COMMANDS_H
#define _COMMANDS_H

#include <stdint.h>

#include "utils.h"
#include "parent_utils.h"

size_t disease_frequency(char *command, hashTable *htable) __attribute__ ((nonnull (1, 2)));

char *topk_age_ranges(char *command, hashTable *disease_ht) __attribute__ ((nonnull (1, 2)));

char *search_patient_record(char *command, doubleLinkedList *rec_list) __attribute__ ((nonnull (1, 2)));

char *num_patient_admissions(char *command, hashTable *htable) __attribute__ ((nonnull (1, 2)));

char *num_patient_discharges(char *command, hashTable *htable) __attribute__ ((nonnull (1, 2)));

#endif // _COMMANDS_H