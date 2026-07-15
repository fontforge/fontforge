#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

ssize_t getdelim_(char **lineptr, size_t *n, int delimiter, FILE *stream);

ssize_t getline_(char **lineptr, size_t *n, FILE *stream);

#ifdef __cplusplus
}
#endif
