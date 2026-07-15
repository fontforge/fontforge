/* GLib compatibility functions for C++ migration
 *
 * This header provides portable replacements for commonly used GLib functions
 * to facilitate removal of GLib dependency from FontForge core.
 *
 * Function declarations are C-compatible; implementations in ffglib_compat.cpp
 * use C++ standard library for portability.
 */

#ifndef FONTFORGE_FFGLIB_COMPAT_H
#define FONTFORGE_FFGLIB_COMPAT_H

#include <stddef.h>

/* GLib boolean constants */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * String functions
 * ============================================================================
 */

/* Count UTF-8 characters in a string.
 * maxlen: maximum bytes to scan (-1 for NUL-terminated)
 * Returns: number of UTF-8 characters (not bytes)
 */
size_t ff_utf8_strlen(const char* str, int maxlen);

/* Locale-independent string to double conversion for PostScript data.
 * Always uses '.' as decimal separator regardless of locale.
 * Callers must strip leading whitespace; no leading '+' is expected.
 */
double ff_strtod(const char* nptr, char** endptr);

/* Locale-independent double to string conversion.
 * Always uses '.' as decimal separator regardless of locale.
 * Compatible with g_ascii_formatd().
 * Returns: dest
 */
char* ff_ascii_formatd(char* dest, size_t dest_len, const char* format,
                       double value);

/* Strip leading and trailing whitespace in-place.
 * Compatible with g_strstrip().
 * Returns: str (modified in place)
 */
char* ff_strstrip(char* str);

/* Case-insensitive ASCII string comparison.
 * Compatible with g_ascii_strcasecmp().
 */
int ff_ascii_strcasecmp(const char* s1, const char* s2);

/* Convert ASCII string to lowercase.
 * Compatible with g_ascii_strdown().
 * Returns: newly allocated lowercase string (caller must free)
 */
char* ff_ascii_strdown(const char* str, int len);

/* Decode percent-encoded URI string.
 * Compatible with g_uri_unescape_string().
 * Returns: newly allocated string (caller must free)
 */
char* ff_uri_unescape_string(const char* escaped,
                             const char* illegal_characters);

/* ============================================================================
 * Random number functions
 * ============================================================================
 */

/* Seed the random number generator */
void ff_random_set_seed(unsigned int seed);

/* Return a random double in [0.0, 1.0) */
double ff_random_double(void);

/* Return a random 32-bit integer */
int ff_random_int(void);

/* Return a random integer in [begin, end) */
int ff_random_int_range(int begin, int end);

/* ============================================================================
 * System/locale functions
 * ============================================================================
 */

/* Get the system character set.
 * Sets *charset to the charset name (static buffer, do not free).
 * Returns: 1 if charset is UTF-8, 0 otherwise.
 * Compatible with g_get_charset().
 */
int ff_get_charset(const char** charset);

/* Get UTC offset in seconds for local time.
 * Replaces GDateTime timezone handling.
 */
long ff_get_utc_offset(long timestamp);

/* Get the real name of the current user.
 * Compatible with g_get_real_name().
 * Returns: static buffer containing user's real name (do not free)
 */
const char* ff_get_real_name(void);

/* Get the system temporary directory.
 * Compatible with g_get_tmp_dir().
 * Returns: static buffer containing temp directory path (do not free)
 */
const char* ff_get_tmp_dir(void);

/* ============================================================================
 * POSIX compatibility for MSVC
 * ============================================================================
 */

#ifdef _MSC_VER
/* MSVC uses strtok_s instead of strtok_r (same signature) */
#define strtok_r(str, delim, saveptr) strtok_s(str, delim, saveptr)

/* MSVC uses _stricmp instead of strcasecmp */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

/* ============================================================================
 * Filesystem functions (portable wrappers using std::filesystem)
 * ============================================================================
 */

/* access() mode flags - match POSIX values */
#ifndef F_OK
#define F_OK 0 /* Test for existence */
#define X_OK 1 /* Test for execute permission */
#define W_OK 2 /* Test for write permission */
#define R_OK 4 /* Test for read permission */
#endif

/* Check file accessibility. Returns 0 on success, -1 on failure.
 * Replaces POSIX access().
 */
int ff_access(const char* path, int mode);

/* Delete a file. Returns 0 on success, -1 on failure.
 * Replaces POSIX unlink().
 */
int ff_unlink(const char* path);

/* Remove an empty directory. Returns 0 on success, -1 on failure.
 * Replaces POSIX rmdir().
 */
int ff_rmdir(const char* path);

/* Create a directory. Returns 0 on success, -1 on failure.
 * Replaces POSIX mkdir().
 */
int ff_mkdir(const char* path, int mode);

/* Change current working directory. Returns 0 on success, -1 on failure.
 * Replaces POSIX chdir().
 */
int ff_chdir(const char* path);

/* Get current working directory. Returns buffer on success, NULL on failure.
 * Replaces POSIX getcwd().
 */
char* ff_getcwd(char* buf, size_t size);

/* Get canonical (absolute, normalized) path.
 * Replaces g_canonicalize_filename().
 * Returns: newly allocated string (caller must free), or NULL on error.
 */
char* ff_canonical_path(const char* path);

/* Remove a file or directory recursively.
 * Replaces g_remove() + directory iteration for recursive delete.
 * Returns: number of files/directories removed, or -1 on error.
 */
int ff_remove_all(const char* path);

/* ============================================================================
 * Dynamic array (replaces GArray)
 * ============================================================================
 */

typedef struct FFArray FFArray;

/* Create a new array with specified element size and initial capacity */
FFArray* ff_array_sized_new(int zero_terminated, int clear, size_t element_size,
                            size_t initial_capacity);

/* Append a value to the array.
 * Usage: ff_array_append(arr, &value);
 */
void ff_array_append(FFArray* arr, const void* val);

/* Get pointer to array data */
void* ff_array_data(FFArray* arr);

/* Get number of elements in array */
size_t ff_array_len(FFArray* arr);

/* Free the array and optionally its data, nulls the pointer */
void ff_array_free(FFArray** p_arr, int free_data);

/* ============================================================================
 * Linked list (replaces GList)
 * ============================================================================
 */

typedef struct FFList {
    void* data;
    struct FFList* next;
} FFList;

/* Append data to end of list. Returns new head of list.
 * Compatible with g_list_append().
 */
FFList* ff_list_append(FFList* list, void* data);

/* Free list structure (not the data).
 * Compatible with g_list_free().
 */
void ff_list_free(FFList* list);

/* Count number of elements in list.
 * Compatible with g_list_length().
 */
unsigned int ff_list_length(FFList* list);

/* Set pointer to NULL and return old value.
 * Compatible with g_steal_pointer().
 */
void* ff_steal_pointer_impl(void** pp);

/* ============================================================================
 * Regex functions
 * ============================================================================
 */

/* Check if string matches regex pattern (case-insensitive).
 * Returns 1 if match, 0 if no match or error.
 */
int ff_regex_match(const char* str, const char* pattern);

/* ============================================================================
 * Sorted string set (case-insensitive, no duplicates)
 * ============================================================================
 */

typedef struct FFStringSet FFStringSet;

/* Create a new string set */
FFStringSet* ff_stringset_new(void);

/* Insert a string (duplicates ignored, case-insensitive) */
void ff_stringset_insert(FFStringSet* ss, const char* str);

/* Iterate over strings in sorted order */
void ff_stringset_foreach(FFStringSet* ss, void (*fn)(const char*, void*),
                          void* user_data);

/* Free the string set, nulls the pointer */
void ff_stringset_free(FFStringSet** p_ss);

#ifdef __cplusplus
}

/* C++ type-safe template version */
template <typename T>
inline T* ff_steal_pointer(T** pp) {
    T* tmp = *pp;
    *pp = nullptr;
    return tmp;
}

#else
/* C macro version */
#define ff_steal_pointer(pp) ff_steal_pointer_impl((void**)(pp))
#endif

#endif /* FONTFORGE_FFGLIB_COMPAT_H */
