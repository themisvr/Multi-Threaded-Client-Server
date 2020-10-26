#define _XOPEN_SOURCE
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"
#include "file_handler.h"


time_t date_to_seconds(const char *date) {	
    
    struct tm storage;
    memset(&storage, 0, sizeof(struct tm));
    time_t retval = 0;
    char *p = (char *)strptime(date, "%d-%m-%Y", &storage);
    if (!p) { return 0; }
    retval = mktime(&storage);
    return retval;
}


uint32_t countries_count(char *restrict dirs) {

	char *token = NULL, *end_tok;
	int dirs_count = 0;
	token = strtok_r(dirs, " \t\n", &end_tok);
	while (token != NULL) {
		++dirs_count;
		token = strtok_r(NULL, "\t\n", &end_tok);
	}
	return dirs_count;
}
