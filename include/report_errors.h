#ifndef _REPORT_ERRORS_H
#define _REPORT_ERRORS_H

#include <errno.h>

#define handle_error_en(en, msg) \
		do { errno = en; perror(msg); strerror(errno); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
		do { perror(msg); exit(EXIT_FAILURE); } while (0)


#endif // _REPORT_ERRORS_H