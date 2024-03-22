#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef ERROR_H
#define ERROR_H

/** Beginning of an error message. */
#define ERROR_TEXT "ERROR: "

/**
 * Function that handles program exiting in case of error.
 * 
 * @param error Error message format.
*/
void error_exit(const char *error, ...);

/**
 * Custom malloc function including error handling.
 */
void* my_malloc(size_t size);

/**
 * Custom realloc function including error handling.
 */
void* my_realloc(void *p, size_t size);

#endif
/* end of "error.h" */