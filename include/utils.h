#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>

/* general purpose functions created for that project */

/* converts date struct to integer (seconds) */
time_t date_to_seconds(const char *date) __attribute__ ((nonnull (1)));

/* counts how many countries got each worker */
uint32_t countries_count(char *restrict dirs) __attribute__ ((nonnull (1)));

#endif // _UTILS_H