/* GLib compatibility functions - C++ implementation
 *
 * Uses C++ standard library for portability across compilers.
 */

#include "ffglib_compat.h"

#include <cctype>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <random>
#include <regex>
#include <set>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#include <lmcons.h>  // For UNLEN
#else
#include <langinfo.h>
#include <pwd.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

/* ============================================================================
 * String functions
 * ============================================================================
 */

extern "C" size_t ff_utf8_strlen(const char* str, int maxlen) {
    if (str == nullptr) return 0;

    size_t count = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(str);
    const unsigned char* end = (maxlen >= 0) ? p + maxlen : nullptr;

    while (*p && (end == nullptr || p < end)) {
        // Count only lead bytes (not continuation bytes 10xxxxxx)
        if ((*p & 0xC0) != 0x80) {
            count++;
        }
        p++;
    }
    return count;
}

/*
 * Locale-independent strtod for parsing PostScript font data.
 *
 * On most platforms we use std::from_chars which is clean and efficient.
 * However, libc++ (used on macOS) still lacks floating-point from_chars
 * support as of 2024, so we fall back to a manual implementation there.
 * Once libc++ adds support, the manual version can be removed.
 */

#if defined(__APPLE__) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 190000)
#define FF_USE_MANUAL_STRTOD 1
#else
#include <charconv>
#define FF_USE_MANUAL_STRTOD 0
#endif

#if FF_USE_MANUAL_STRTOD

// Manual implementation for macOS/libc++ (no whitespace or leading + handling;
// callers already strip whitespace, and PostScript data has no leading +)
static double ff_strtod_manual(const char* nptr, char** endptr) {
    const char* p = nptr;
    double result = 0.0;
    double fraction = 0.0;
    double divisor = 1.0;
    int sign = 1;
    int exp_sign = 1;
    int exponent = 0;
    bool has_digits = false;

    if (*p == '-') {
        sign = -1;
        p++;
    }

    while (std::isdigit(static_cast<unsigned char>(*p))) {
        result = result * 10.0 + (*p - '0');
        has_digits = true;
        p++;
    }

    if (*p == '.') {
        p++;
        while (std::isdigit(static_cast<unsigned char>(*p))) {
            fraction = fraction * 10.0 + (*p - '0');
            divisor *= 10.0;
            has_digits = true;
            p++;
        }
        result += fraction / divisor;
    }

    if (!has_digits) {
        if (endptr) *endptr = const_cast<char*>(nptr);
        return 0.0;
    }

    if (*p == 'e' || *p == 'E') {
        const char* exp_start = p;
        p++;
        if (*p == '-') {
            exp_sign = -1;
            p++;
        } else if (*p == '+') {
            p++;
        }
        if (std::isdigit(static_cast<unsigned char>(*p))) {
            while (std::isdigit(static_cast<unsigned char>(*p))) {
                exponent = exponent * 10 + (*p - '0');
                p++;
            }
            result *= std::pow(10.0, exp_sign * exponent);
        } else {
            p = exp_start;
        }
    }

    if (endptr) *endptr = const_cast<char*>(p);
    return sign * result;
}

#endif  // FF_USE_MANUAL_STRTOD

extern "C" double ff_strtod(const char* nptr, char** endptr) {
#if FF_USE_MANUAL_STRTOD
    return ff_strtod_manual(nptr, endptr);
#else
    const char* p = nptr;
    const char* end = p;

    while (*end == '-' || *end == '.' || *end == 'e' || *end == 'E' ||
           *end == '+' || std::isdigit(static_cast<unsigned char>(*end))) {
        end++;
    }

    double result = 0.0;
    auto [ptr, ec] = std::from_chars(p, end, result);

    if (ec == std::errc{}) {
        if (endptr) *endptr = const_cast<char*>(ptr);
        return result;
    } else {
        if (endptr) *endptr = const_cast<char*>(nptr);
        return 0.0;
    }
#endif
}

extern "C" char* ff_ascii_formatd(char* dest, size_t dest_len,
                                  const char* format, double value) {
    if (dest == nullptr || dest_len == 0) return dest;

    // Format using snprintf
    std::snprintf(dest, dest_len, format, value);

    // Find what the locale uses as decimal point and replace with '.'
    std::lconv* locale_info = std::localeconv();
    if (locale_info && locale_info->decimal_point &&
        locale_info->decimal_point[0] != '.' &&
        locale_info->decimal_point[0] != '\0') {
        char* decimal_point = std::strchr(dest, locale_info->decimal_point[0]);
        if (decimal_point) {
            *decimal_point = '.';
        }
    }

    return dest;
}

extern "C" char* ff_strstrip(char* str) {
    if (str == nullptr) return nullptr;

    // Skip leading whitespace
    char* start = str;
    while (std::isspace(static_cast<unsigned char>(*start))) start++;

    // If all whitespace, truncate string
    if (*start == '\0') {
        *str = '\0';
        return str;
    }

    // Find end of string
    char* end = start + std::strlen(start) - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(*end))) end--;

    // Terminate after last non-whitespace
    *(end + 1) = '\0';

    // Move to beginning if needed
    if (start != str) {
        std::memmove(str, start, static_cast<size_t>(end - start) + 2);
    }

    return str;
}

extern "C" int ff_ascii_strcasecmp(const char* s1, const char* s2) {
    if (s1 == nullptr && s2 == nullptr) return 0;
    if (s1 == nullptr) return -1;
    if (s2 == nullptr) return 1;

    while (*s1 && *s2) {
        int c1 = std::tolower(static_cast<unsigned char>(*s1));
        int c2 = std::tolower(static_cast<unsigned char>(*s2));
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return std::tolower(static_cast<unsigned char>(*s1)) -
           std::tolower(static_cast<unsigned char>(*s2));
}

extern "C" char* ff_ascii_strdown(const char* str, int len) {
    if (str == nullptr) return nullptr;

    size_t actual_len = (len < 0) ? std::strlen(str) : static_cast<size_t>(len);
    char* result = static_cast<char*>(std::malloc(actual_len + 1));
    if (!result) return nullptr;

    for (size_t i = 0; i < actual_len; i++) {
        result[i] =
            static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
    }
    result[actual_len] = '\0';
    return result;
}

extern "C" char* ff_uri_unescape_string(const char* escaped,
                                        const char* illegal_characters) {
    (void)illegal_characters;  // Not implemented

    if (escaped == nullptr) return nullptr;

    size_t len = std::strlen(escaped);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (!result) return nullptr;

    const char* src = escaped;
    char* dst = result;

    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            // Decode %XX
            char hex[3] = {src[1], src[2], '\0'};
            char* end;
            long val = std::strtol(hex, &end, 16);
            if (end == hex + 2 && val >= 0 && val <= 255) {
                *dst++ = static_cast<char>(val);
                src += 3;
                continue;
            }
        }
        if (*src == '+') {
            // + often represents space in URL encoding
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    return result;
}

/* ============================================================================
 * Random number functions - using C++ <random>
 * ============================================================================
 */

namespace {
std::mt19937& get_random_engine() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
}  // namespace

extern "C" void ff_random_set_seed(unsigned int seed) {
    get_random_engine().seed(seed);
}

extern "C" double ff_random_double(void) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(get_random_engine());
}

extern "C" int ff_random_int(void) {
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    return dist(get_random_engine());
}

extern "C" int ff_random_int_range(int begin, int end) {
    if (begin >= end) return begin;
    std::uniform_int_distribution<int> dist(begin, end - 1);
    return dist(get_random_engine());
}

/* ============================================================================
 * System/locale functions
 * ============================================================================
 */

extern "C" int ff_get_charset(const char** charset) {
#ifdef _WIN32
    static char charset_buf[32];
    UINT cp = GetACP();
    if (cp == 65001) {
        if (charset) *charset = "UTF-8";
        return 1;
    }
    std::snprintf(charset_buf, sizeof(charset_buf), "CP%u", cp);
    if (charset) *charset = charset_buf;
    return 0;
#else
    const char* cs = nl_langinfo(CODESET);
    if (charset) *charset = cs;
    // Check if it's UTF-8 (case-insensitive, handle variations)
    if (cs && (strcasecmp(cs, "UTF-8") == 0 || strcasecmp(cs, "UTF8") == 0)) {
        return 1;
    }
    return 0;
#endif
}

extern "C" long ff_get_utc_offset(long timestamp) {
    std::time_t t = static_cast<std::time_t>(timestamp);
#ifdef _WIN32
    struct tm local_tm, utc_tm;
    localtime_s(&local_tm, &t);
    gmtime_s(&utc_tm, &t);
    std::time_t local_t = std::mktime(&local_tm);
    std::time_t utc_t = std::mktime(&utc_tm);
    return static_cast<long>(std::difftime(local_t, utc_t));
#else
    struct tm local_tm;
    localtime_r(&t, &local_tm);
    return local_tm.tm_gmtoff;
#endif
}

extern "C" const char* ff_get_real_name(void) {
    static char name_buf[256];
    static bool initialized = false;

    if (!initialized) {
#ifdef _WIN32
        DWORD size = sizeof(name_buf);
        if (!GetUserNameA(name_buf, &size)) {
            std::strcpy(name_buf, "Unknown");
        }
#else
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_gecos && pw->pw_gecos[0]) {
            // pw_gecos often contains full name, possibly followed by
            // comma-separated fields
            const char* comma = std::strchr(pw->pw_gecos, ',');
            size_t len = comma ? static_cast<size_t>(comma - pw->pw_gecos)
                               : std::strlen(pw->pw_gecos);
            if (len >= sizeof(name_buf)) len = sizeof(name_buf) - 1;
            std::strncpy(name_buf, pw->pw_gecos, len);
            name_buf[len] = '\0';
        } else if (pw && pw->pw_name) {
            std::strncpy(name_buf, pw->pw_name, sizeof(name_buf) - 1);
            name_buf[sizeof(name_buf) - 1] = '\0';
        } else {
            std::strcpy(name_buf, "Unknown");
        }
#endif
        initialized = true;
    }
    return name_buf;
}

extern "C" const char* ff_get_tmp_dir(void) {
    static char tmp_buf[4096];
    static bool initialized = false;

    if (!initialized) {
#ifdef _WIN32
        DWORD len = GetTempPathA(sizeof(tmp_buf), tmp_buf);
        if (len == 0 || len >= sizeof(tmp_buf)) {
            std::strcpy(tmp_buf, "C:\\Temp");
        } else {
            // Remove trailing backslash if present
            if (len > 0 && tmp_buf[len - 1] == '\\') {
                tmp_buf[len - 1] = '\0';
            }
        }
#else
        const char* tmp = std::getenv("TMPDIR");
        if (!tmp) tmp = std::getenv("TMP");
        if (!tmp) tmp = std::getenv("TEMP");
        if (!tmp) tmp = "/tmp";
        std::strncpy(tmp_buf, tmp, sizeof(tmp_buf) - 1);
        tmp_buf[sizeof(tmp_buf) - 1] = '\0';
#endif
        initialized = true;
    }
    return tmp_buf;
}

/* ============================================================================
 * Dynamic array implementation
 * ============================================================================
 */

struct FFArray {
    char* data;
    size_t len;
    size_t capacity;
    size_t element_size;
    bool clear_new;
};

extern "C" FFArray* ff_array_sized_new(int zero_terminated, int clear,
                                       size_t element_size,
                                       size_t initial_capacity) {
    (void)zero_terminated;  // Not used

    FFArray* arr = new (std::nothrow) FFArray;
    if (!arr) return nullptr;

    arr->element_size = element_size;
    arr->len = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 16;
    arr->clear_new = clear != 0;
    arr->data = static_cast<char*>(std::malloc(arr->capacity * element_size));
    if (!arr->data) {
        delete arr;
        return nullptr;
    }
    if (arr->clear_new) {
        std::memset(arr->data, 0, arr->capacity * element_size);
    }
    return arr;
}

extern "C" void ff_array_append(FFArray* arr, const void* val) {
    if (!arr) return;

    // Grow if needed
    if (arr->len >= arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        char* new_data = static_cast<char*>(
            std::realloc(arr->data, new_capacity * arr->element_size));
        if (!new_data) return;
        if (arr->clear_new) {
            std::memset(new_data + arr->capacity * arr->element_size, 0,
                        (new_capacity - arr->capacity) * arr->element_size);
        }
        arr->data = new_data;
        arr->capacity = new_capacity;
    }

    std::memcpy(arr->data + arr->len * arr->element_size, val,
                arr->element_size);
    arr->len++;
}

extern "C" void* ff_array_data(FFArray* arr) {
    return arr ? arr->data : nullptr;
}

extern "C" size_t ff_array_len(FFArray* arr) { return arr ? arr->len : 0; }

extern "C" void ff_array_free(FFArray** p_arr, int free_data) {
    if (p_arr && *p_arr) {
        FFArray* arr = *p_arr;
        if (free_data && arr->data) {
            std::free(arr->data);
        }
        delete arr;
        *p_arr = nullptr;
    }
}

/* ============================================================================
 * Linked list implementation
 * ============================================================================
 */

extern "C" FFList* ff_list_append(FFList* list, void* data) {
    FFList* node = static_cast<FFList*>(std::malloc(sizeof(FFList)));
    if (!node) return list;
    node->data = data;
    node->next = nullptr;

    if (list == nullptr) {
        return node;
    }

    // Find the tail
    FFList* tail = list;
    while (tail->next != nullptr) {
        tail = tail->next;
    }
    tail->next = node;
    return list;
}

extern "C" void ff_list_free(FFList* list) {
    while (list != nullptr) {
        FFList* next = list->next;
        std::free(list);
        list = next;
    }
}

extern "C" unsigned int ff_list_length(FFList* list) {
    unsigned int count = 0;
    while (list != nullptr) {
        ++count;
        list = list->next;
    }
    return count;
}

extern "C" void* ff_steal_pointer_impl(void** pp) {
    void* tmp = *pp;
    *pp = nullptr;
    return tmp;
}

/* ============================================================================
 * Filesystem functions using std::filesystem
 * ============================================================================
 */

extern "C" int ff_access(const char* path, int mode) {
    if (!path) return -1;

    std::error_code ec;
    fs::path p(path);

    // Check existence first
    if (!fs::exists(p, ec) || ec) {
        return -1;
    }

    // For F_OK (existence), we're done
    if (mode == 0) {
        return 0;
    }

    // Check permissions
    auto status = fs::status(p, ec);
    if (ec) return -1;

    auto perms = status.permissions();

    // Note: std::filesystem permission checks are owner-based on POSIX,
    // but on Windows they're simplified. This is a reasonable approximation.
    if ((mode & 4) && (perms & fs::perms::owner_read) == fs::perms::none) {
        return -1;
    }
    if ((mode & 2) && (perms & fs::perms::owner_write) == fs::perms::none) {
        return -1;
    }
    if ((mode & 1) && (perms & fs::perms::owner_exec) == fs::perms::none) {
        return -1;
    }

    return 0;
}

extern "C" int ff_unlink(const char* path) {
    if (!path) return -1;

    std::error_code ec;
    if (fs::remove(path, ec) && !ec) {
        return 0;
    }
    return -1;
}

extern "C" int ff_rmdir(const char* path) {
    if (!path) return -1;

    std::error_code ec;
    fs::path p(path);

    // Ensure it's a directory
    if (!fs::is_directory(p, ec) || ec) {
        return -1;
    }

    if (fs::remove(p, ec) && !ec) {
        return 0;
    }
    return -1;
}

extern "C" int ff_mkdir(const char* path, int mode) {
    (void)mode;  // std::filesystem doesn't take mode on Windows

    if (!path) return -1;

    std::error_code ec;
    if (fs::create_directory(path, ec) && !ec) {
        return 0;
    }
    // Also succeed if directory already exists
    if (fs::is_directory(path, ec) && !ec) {
        return 0;
    }
    return -1;
}

extern "C" int ff_chdir(const char* path) {
    if (!path) return -1;

    std::error_code ec;
    fs::current_path(path, ec);
    return ec ? -1 : 0;
}

extern "C" char* ff_getcwd(char* buf, size_t size) {
    if (!buf || size == 0) return nullptr;

    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    if (ec) return nullptr;

    std::string cwd_str = cwd.string();
    if (cwd_str.length() >= size) {
        return nullptr;  // Buffer too small
    }

    std::strcpy(buf, cwd_str.c_str());
    return buf;
}

extern "C" char* ff_canonical_path(const char* path) {
    if (!path) return nullptr;

    std::error_code ec;
    fs::path p(path);

    // Use absolute() first to handle non-existent paths,
    // then lexically_normal() to resolve . and ..
    fs::path abs = fs::absolute(p, ec);
    if (ec) return nullptr;

    fs::path result = abs.lexically_normal();

    std::string result_str = result.string();
    char* ret = static_cast<char*>(std::malloc(result_str.length() + 1));
    if (!ret) return nullptr;

    std::strcpy(ret, result_str.c_str());
    return ret;
}

extern "C" int ff_remove_all(const char* path) {
    if (!path) return -1;

    std::error_code ec;
    auto count = fs::remove_all(path, ec);
    if (ec) {
        return -1;
    }
    return static_cast<int>(count);
}

/* ============================================================================
 * Regex functions using std::regex
 * ============================================================================
 */

extern "C" int ff_regex_match(const char* str, const char* pattern) {
    if (!str || !pattern) return 0;

    try {
        std::regex re(pattern, std::regex::ECMAScript | std::regex::icase);
        return std::regex_match(str, re) ? 1 : 0;
    } catch (const std::regex_error&) {
        return 0;
    }
}

/* ============================================================================
 * Sorted string set using std::set
 * ============================================================================
 */

namespace {
struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return ff_ascii_strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};
}  // namespace

struct FFStringSet {
    std::set<std::string, CaseInsensitiveCompare> data;
};

extern "C" FFStringSet* ff_stringset_new(void) {
    return new (std::nothrow) FFStringSet();
}

extern "C" void ff_stringset_insert(FFStringSet* ss, const char* str) {
    if (ss && str) {
        ss->data.insert(str);
    }
}

extern "C" void ff_stringset_foreach(FFStringSet* ss,
                                     void (*fn)(const char*, void*),
                                     void* user_data) {
    if (ss && fn) {
        for (const auto& s : ss->data) {
            fn(s.c_str(), user_data);
        }
    }
}

extern "C" void ff_stringset_free(FFStringSet** p_ss) {
    if (p_ss && *p_ss) {
        delete *p_ss;
        *p_ss = nullptr;
    }
}
