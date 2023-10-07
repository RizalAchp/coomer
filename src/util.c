#include "util.h"

#include <assert.h>
#include <ctype.h>

bool verbose = false;

void coom_log(FILE *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
    fprintf(out, "\n");
}

void *coom_alloc(void *old, usize cap) {
    void *result = realloc(old, cap);
    if (result == NULL) {
        coom_error("realloc memory error - returned NULL - %s", strerror(errno));
        exit(1);
    }
    return result;
}
void coom_free(void *ptr) {
    if (ptr) free(ptr);
}

strview sv_trim_left(strview sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) i += 1;
    return SV_FROM(sv.data + i, sv.count - i);
}
strview sv_trim_right(strview sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) i += 1;
    return SV_FROM(sv.data, sv.count - i);
}

bool sv_eq(strview lhs, const char *rhs) {
    size_t rhscount = strlen(rhs);
    return (lhs.count != rhscount) ? false : memcmp(lhs.data, rhs, lhs.count) == 0;
}
bool sv_eq_icase(strview lhs, const char *rhs) {
    size_t rhscount = strlen(rhs);
    if (lhs.count != rhscount) return false;
    char x, y;
    for (size_t i = 0; i < lhs.count; i++) {
        x = 'A' <= lhs.data[i] && lhs.data[i] <= 'Z' ? lhs.data[i] + 32 : lhs.data[i];
        y = 'A' <= rhs[i] && rhs[i] <= 'Z' ? rhs[i] + 32 : rhs[i];
        if (x != y) return false;
    }
    return true;
}
bool sv_chop_by_delim(strview *sv, char delim, strview *chunk) {
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) i += 1;
    strview result = SV_FROM(sv->data, i);
    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data += i + 1;
        if (chunk) {
            *chunk = result;
        }
        return true;
    }
    return false;
}

bool sv_index_of(strview sv, char c, size_t *index) {
    size_t i = 0;
    while (i < sv.count && sv.data[i] != c) i += 1;
    if (i < sv.count) {
        if (index) *index = i;
        return true;
    } else {
        return false;
    }
}

strview sv_chop_left(strview *sv, size_t n) {
    if (n > sv->count) n = sv->count;
    strview result = SV_FROM(sv->data, n);
    sv->data += n;
    sv->count -= n;
    return result;
}
strview sv_chop_right(strview *sv, size_t n) {
    if (n > sv->count) n = sv->count;
    strview result = SV_FROM(sv->data + sv->count - n, n);
    sv->count -= n;
    return result;
}

#ifndef TEMP_CAPACITY
#    define TEMP_CAPACITY (8 * 1024 * 1024)
#endif  // TEMP_CAPACITY
static size_t g_temp_size           = 0;
static char   g_temp[TEMP_CAPACITY] = {0};

char         *temp_strdup(const char *cstr) {
    size_t n      = strlen(cstr);
    char  *result = temp_alloc(n + 1);
    assert(result != NULL && "Increase TEMP_CAPACITY");
    memcpy(result, cstr, n);
    result[n] = '\0';
    return result;
}

void *temp_alloc(size_t size) {
    if (g_temp_size + size > TEMP_CAPACITY) return NULL;
    void *result = &g_temp[g_temp_size];
    g_temp_size += size;
    return result;
}

char *temp_sprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);
    assert(n >= 0);
    char *result = temp_alloc(n + 1);
    assert(result != NULL && "Extend the size of the temporary allocator");
    va_start(args, format);
    vsnprintf(result, n + 1, format, args);
    va_end(args);
    return result;
}
void        temp_reset(void) { g_temp_size = 0; }
size_t      temp_size(void) { return g_temp_size; }
void        temp_free(size_t size) { g_temp_size -= size; }
void        temp_rewind(size_t checkpoint) { g_temp_size = checkpoint; }

const char *sv_to_cstr(strview sv) {
    char *result = temp_alloc(sv.count + 1);
    assert(result != NULL && "Extend the size of the temporary allocator");
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}

vec2_t vec2_normalize(vec2_t v) {
    f32 b = vec2_lenght(v);
    if (b == 0.0) return vec2(0.0, 0.0);
    else return vec2(v.x / b, v.y / b);
}
