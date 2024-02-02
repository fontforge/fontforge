#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream);

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

#endif /* #ifdef _WIN32 */
