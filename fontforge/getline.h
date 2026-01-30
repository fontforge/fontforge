#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

ssize_t getdelim_(char **lineptr, size_t *n, int delimiter, FILE *stream);

ssize_t getline_(char **lineptr, size_t *n, FILE *stream);
