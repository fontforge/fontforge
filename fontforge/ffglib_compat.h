/* GLib compatibility functions for C++ migration
 *
 * This header provides C++ implementations of commonly used GLib functions
 * to facilitate removal of GLib dependency from FontForge core.
 */

#ifndef FONTFORGE_FFGLIB_COMPAT_H
#define FONTFORGE_FFGLIB_COMPAT_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Count UTF-8 characters in a string.
 * maxlen: maximum bytes to scan (-1 for NUL-terminated)
 * Returns: number of UTF-8 characters (not bytes)
 */
static inline size_t ff_utf8_strlen(const char *str, int maxlen) {
    if (str == NULL) return 0;

    size_t count = 0;
    const unsigned char *p = (const unsigned char *)str;
    const unsigned char *end = (maxlen >= 0) ? p + maxlen : NULL;

    while (*p && (end == NULL || p < end)) {
        /* Count only lead bytes (not continuation bytes 10xxxxxx) */
        if ((*p & 0xC0) != 0x80) {
            count++;
        }
        p++;
    }
    return count;
}

/* Locale-independent string to double conversion.
 * Always uses '.' as decimal separator regardless of locale.
 * Compatible with g_ascii_strtod().
 */
static inline double ff_ascii_strtod(const char *nptr, char **endptr) {
    const char *p = nptr;
    double result = 0.0;
    double fraction = 0.0;
    double divisor = 1.0;
    int sign = 1;
    int exp_sign = 1;
    int exponent = 0;
    int has_digits = 0;
    int in_fraction = 0;

    /* Skip leading whitespace */
    while (isspace((unsigned char)*p)) p++;

    /* Handle sign */
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    /* Handle special values */
    if ((*p == 'i' || *p == 'I') &&
        (p[1] == 'n' || p[1] == 'N') &&
        (p[2] == 'f' || p[2] == 'F')) {
        p += 3;
        if ((*p == 'i' || *p == 'I') &&
            (p[1] == 'n' || p[1] == 'N') &&
            (p[2] == 'i' || p[2] == 'I') &&
            (p[3] == 't' || p[3] == 'T') &&
            (p[4] == 'y' || p[4] == 'Y')) {
            p += 5;
        }
        if (endptr) *endptr = (char *)p;
        return sign * HUGE_VAL;
    }
    if ((*p == 'n' || *p == 'N') &&
        (p[1] == 'a' || p[1] == 'A') &&
        (p[2] == 'n' || p[2] == 'N')) {
        p += 3;
        if (endptr) *endptr = (char *)p;
#ifdef NAN
        return NAN;
#else
        return 0.0 / 0.0;
#endif
    }

    /* Parse integer part */
    while (isdigit((unsigned char)*p)) {
        result = result * 10.0 + (*p - '0');
        has_digits = 1;
        p++;
    }

    /* Parse fractional part (using '.' as decimal separator) */
    if (*p == '.') {
        p++;
        in_fraction = 1;
        while (isdigit((unsigned char)*p)) {
            fraction = fraction * 10.0 + (*p - '0');
            divisor *= 10.0;
            has_digits = 1;
            p++;
        }
        result += fraction / divisor;
    }

    /* If no digits found, return 0 and set endptr to start */
    if (!has_digits) {
        if (endptr) *endptr = (char *)nptr;
        return 0.0;
    }

    /* Parse exponent */
    if (*p == 'e' || *p == 'E') {
        const char *exp_start = p;
        p++;
        if (*p == '-') {
            exp_sign = -1;
            p++;
        } else if (*p == '+') {
            p++;
        }
        if (isdigit((unsigned char)*p)) {
            while (isdigit((unsigned char)*p)) {
                exponent = exponent * 10 + (*p - '0');
                p++;
            }
            /* Apply exponent */
            if (exp_sign < 0) {
                while (exponent-- > 0) result /= 10.0;
            } else {
                while (exponent-- > 0) result *= 10.0;
            }
        } else {
            /* No digits after E, backtrack */
            p = exp_start;
        }
    }

    if (endptr) *endptr = (char *)p;
    return sign * result;
}

/* Locale-independent double to string conversion.
 * Always uses '.' as decimal separator regardless of locale.
 * Compatible with g_ascii_formatd().
 * Returns: dest
 */
static inline char *ff_ascii_formatd(char *dest, size_t dest_len, const char *format, double value) {
    char *decimal_point;
    struct lconv *locale_info;

    if (dest == NULL || dest_len == 0) return dest;

    /* Format using current locale */
    snprintf(dest, dest_len, format, value);

    /* Find what the locale uses as decimal point */
    locale_info = localeconv();
    if (locale_info && locale_info->decimal_point &&
        locale_info->decimal_point[0] != '.' &&
        locale_info->decimal_point[0] != '\0') {
        /* Replace locale decimal point with '.' */
        decimal_point = strchr(dest, locale_info->decimal_point[0]);
        if (decimal_point) {
            *decimal_point = '.';
        }
    }

    return dest;
}

/* Simple dynamic array to replace GArray.
 * This is a minimal implementation for FontForge's specific use cases.
 */
typedef struct {
    char *data;           /* Array data */
    size_t len;           /* Number of elements */
    size_t capacity;      /* Allocated capacity (elements) */
    size_t element_size;  /* Size of each element */
    int clear_new;        /* Zero-initialize new elements */
} FFArray;

/* Create a new array with specified element size and initial capacity */
static inline FFArray *ff_array_sized_new(int zero_terminated, int clear, size_t element_size, size_t initial_capacity) {
    FFArray *arr = (FFArray *)malloc(sizeof(FFArray));
    if (!arr) return NULL;

    (void)zero_terminated; /* Not used in our implementation */

    arr->element_size = element_size;
    arr->len = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 16;
    arr->clear_new = clear;
    arr->data = (char *)malloc(arr->capacity * element_size);
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    if (clear) {
        memset(arr->data, 0, arr->capacity * element_size);
    }
    return arr;
}

/* Ensure array has room for at least one more element */
static inline int ff_array_maybe_grow(FFArray *arr) {
    if (arr->len >= arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        char *new_data = (char *)realloc(arr->data, new_capacity * arr->element_size);
        if (!new_data) return 0;
        if (arr->clear_new) {
            memset(new_data + arr->capacity * arr->element_size, 0,
                   (new_capacity - arr->capacity) * arr->element_size);
        }
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    return 1;
}

/* Append a value to the array.
 * Note: Unlike g_array_append_val, this takes a pointer to the value.
 * Usage: ff_array_append(arr, &value);
 */
static inline void ff_array_append(FFArray *arr, const void *val) {
    if (!ff_array_maybe_grow(arr)) return;
    memcpy(arr->data + arr->len * arr->element_size, val, arr->element_size);
    arr->len++;
}

/* Free the array and optionally its data */
static inline void ff_array_free(FFArray *arr, int free_data) {
    if (arr) {
        if (free_data && arr->data) {
            free(arr->data);
        }
        free(arr);
    }
}

/* Strip leading and trailing whitespace in-place.
 * Compatible with g_strstrip().
 * Returns: str (modified in place)
 */
static inline char *ff_strstrip(char *str) {
    char *start, *end;

    if (str == NULL) return NULL;

    /* Skip leading whitespace */
    start = str;
    while (isspace((unsigned char)*start)) start++;

    /* If all whitespace, truncate string */
    if (*start == '\0') {
        *str = '\0';
        return str;
    }

    /* Find end of string */
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;

    /* Terminate after last non-whitespace */
    *(end + 1) = '\0';

    /* Move to beginning if needed */
    if (start != str) {
        memmove(str, start, (end - start) + 2);
    }

    return str;
}

/* Case-insensitive ASCII string comparison.
 * Compatible with g_ascii_strcasecmp().
 */
static inline int ff_ascii_strcasecmp(const char *s1, const char *s2) {
    if (s1 == NULL && s2 == NULL) return 0;
    if (s1 == NULL) return -1;
    if (s2 == NULL) return 1;

    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/* Decode percent-encoded URI string.
 * Compatible with g_uri_unescape_string().
 * Returns: newly allocated string (caller must free)
 */
static inline char *ff_uri_unescape_string(const char *escaped, const char *illegal_characters) {
    (void)illegal_characters; /* Not implemented */

    if (escaped == NULL) return NULL;

    size_t len = strlen(escaped);
    char *result = (char *)malloc(len + 1);
    if (!result) return NULL;

    const char *src = escaped;
    char *dst = result;

    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            /* Decode %XX */
            char hex[3] = { src[1], src[2], '\0' };
            char *end;
            long val = strtol(hex, &end, 16);
            if (end == hex + 2 && val >= 0 && val <= 255) {
                *dst++ = (char)val;
                src += 3;
                continue;
            }
        }
        if (*src == '+') {
            /* + often represents space in URL encoding */
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    return result;
}

/* Get UTC offset in seconds for local time.
 * Replaces GDateTime timezone handling.
 */
#ifdef _WIN32
#include <time.h>
static inline long ff_get_utc_offset(time_t t) {
    struct tm local_tm, utc_tm;
    localtime_s(&local_tm, &t);
    gmtime_s(&utc_tm, &t);
    time_t local_t = mktime(&local_tm);
    time_t utc_t = mktime(&utc_tm);
    return (long)difftime(local_t, utc_t);
}
#else
#include <time.h>
static inline long ff_get_utc_offset(time_t t) {
    struct tm local_tm;
    localtime_r(&t, &local_tm);
    return local_tm.tm_gmtoff;
}
#endif

/* Random number functions to replace GLib g_random_* functions.
 * Uses standard C library random functions.
 */

/* Seed the random number generator */
static inline void ff_random_set_seed(unsigned int seed) {
    srand(seed);
}

/* Return a random double in [0.0, 1.0) */
static inline double ff_random_double(void) {
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

/* Return a random 32-bit integer */
static inline int ff_random_int(void) {
    /* Combine two rand() calls for better distribution on systems
     * where RAND_MAX is only 15 bits (e.g., Windows) */
#if RAND_MAX < 0x7FFFFFFF
    return (rand() << 16) ^ rand();
#else
    return rand();
#endif
}

/* Return a random integer in [begin, end) */
static inline int ff_random_int_range(int begin, int end) {
    if (begin >= end) return begin;
    int range = end - begin;
    return begin + (int)(ff_random_double() * range);
}

/* Get the system character set.
 * Sets *charset to the charset name (static string, do not free).
 * Returns true if the charset is UTF-8.
 * Compatible with g_get_charset().
 */
#ifdef _WIN32
#include <windows.h>
static inline int ff_get_charset(const char **charset) {
    static char charset_buf[32];
    UINT cp = GetACP();
    if (cp == 65001) {
        *charset = "UTF-8";
        return 1;
    }
    snprintf(charset_buf, sizeof(charset_buf), "CP%u", cp);
    *charset = charset_buf;
    return 0;
}
#else
#include <langinfo.h>
static inline int ff_get_charset(const char **charset) {
    const char *cs = nl_langinfo(CODESET);
    if (charset) *charset = cs;
    /* Check if it's UTF-8 (case-insensitive, handle variations) */
    if (cs && (strcasecmp(cs, "UTF-8") == 0 || strcasecmp(cs, "UTF8") == 0)) {
        return 1;
    }
    return 0;
}
#endif

/* Simple singly-linked list to replace GList.
 * GLib's GList is doubly-linked, but plugin.c only uses forward iteration,
 * append, and free operations, so a singly-linked list suffices.
 */
typedef struct FFList {
    void *data;
    struct FFList *next;
} FFList;

/* Append data to end of list. Returns new head of list.
 * Compatible with g_list_append().
 */
static inline FFList *ff_list_append(FFList *list, void *data) {
    FFList *node = (FFList *)malloc(sizeof(FFList));
    if (!node) return list;
    node->data = data;
    node->next = NULL;

    if (list == NULL) {
        return node;
    }

    /* Find the tail */
    FFList *tail = list;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = node;
    return list;
}

/* Free list structure (not the data).
 * Compatible with g_list_free().
 */
static inline void ff_list_free(FFList *list) {
    while (list != NULL) {
        FFList *next = list->next;
        free(list);
        list = next;
    }
}

/* Atomically set pointer to NULL and return old value.
 * Compatible with g_steal_pointer().
 * Note: This is not thread-safe, but neither was the typical usage.
 */
#define ff_steal_pointer(pp) \
    ({ typeof(*(pp)) _tmp = *(pp); *(pp) = NULL; _tmp; })

/* For MSVC which doesn't support statement expressions */
#ifdef _MSC_VER
#undef ff_steal_pointer
static inline void *ff_steal_pointer_impl(void **pp) {
    void *tmp = *pp;
    *pp = NULL;
    return tmp;
}
#define ff_steal_pointer(pp) ff_steal_pointer_impl((void **)(pp))
#endif

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_FFGLIB_COMPAT_H */
