#ifndef __COOMER_UTIL_H__
#define __COOMER_UTIL_H__
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DA_INIT_CAP  256
#define da_first(da) (ASSERT((da)->count), &(da)->items[0])
#define da_last(da)  (ASSERT((da)->count), &(da)->items[(da)->count - 1])
#define da_append(da, item)                                                               \
    do {                                                                                  \
        if ((da)->count >= (da)->capacity) {                                              \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity * 2;      \
            (da)->items    = REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            ASSERT_ALLOC((da)->items);                                                    \
        }                                                                                 \
        (da)->items[(da)->count++] = (item);                                              \
    } while (0)
#define da_free(da)            \
    do {                       \
        coom_free((da).items); \
        (da).items    = NULL;  \
        (da).capacity = 0;     \
        (da).count    = 0;     \
    } while (0)
#define da_append_many(da, new_items, new_items_count)                                        \
    do {                                                                                      \
        if ((da)->count + new_items_count > (da)->capacity) {                                 \
            if ((da)->capacity == 0) (da)->capacity = DA_INIT_CAP;                            \
            while ((da)->count + new_items_count > (da)->capacity) (da)->capacity *= 2;       \
            (da)->items = coom_alloc((da)->items, (da)->capacity * sizeof(*(da)->items));     \
        }                                                                                     \
        memcpy((da)->items + (da)->count, new_items, new_items_count * sizeof(*(da)->items)); \
        (da)->count += new_items_count;                                                       \
    } while (0)

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;
typedef size_t   usize;

#define return_defer(ret) \
    {                     \
        result = ret;     \
        goto defer;       \
    }

extern bool verbose;

void        coom_log(FILE *out, const char *fmt, ...);
#define coom_error(...) coom_log(stderr, "[ERROR]: " __VA_ARGS__)
#define coom_warning(...) \
    if (verbose) coom_log(stderr, "[WARN]: " __VA_ARGS__)
#define coom_info(...) \
    if (verbose) coom_log(stderr, "[INFO]: " __VA_ARGS__)

void *coom_alloc(void *old, usize cap);
void  coom_free(void *ptr);

typedef struct {
    size_t      count;
    const char *data;
} strview;
#define SV_FROM(D, C) ((strview){.count = C, .data = D})
#define SV_NULL       SV_FROM(NULL, 0)
#define SV_CSTR(cstr) SV_FROM(cstr, strlen(cstr))
#define SV(cstr_lit)  SV_FROM(cstr_lit, sizeof(cstr_lit) - 1)

#define SV_FMT        "%.*s"
#define SV_ARG(sv)    (int)(sv).count, (sv).data

strview sv_trim_left(strview sv);
strview sv_trim_right(strview sv);
#define sv_trim(sv) sv_trim_right(sv_trim_left(sv))
bool        sv_eq(strview lhs, const char *rhs);
bool        sv_eq_icase(strview lhs, const char *rhs);
bool        sv_chop_by_delim(strview *sv, char delim, strview *chunk);
bool        sv_index_of(strview sv, char c, size_t *index);
strview     sv_chop_left(strview *sv, size_t n);
strview     sv_chop_right(strview *sv, size_t n);
const char *sv_to_cstr(strview sv);

typedef struct {
    size_t capacity;
    size_t count;
    char  *items;
} strbuilder;

#define sb_append_buf(sb, buf, size) da_append_many(sb, buf, size)
#define sb_append_cstr(sb, cstr)   \
    do {                           \
        const char *s = (cstr);    \
        size_t      n = strlen(s); \
        da_append_many(sb, s, n);  \
    } while (0)
#define sb_append_null(sb) da_append_many(sb, "", 1)
#define sb_free(sb)        coom_free((sb).items)
#define sb_as_sv(sb)       SV_FROM((sb).items, (sb).count)

#define TEMP_CAPACITY      (1 * 1024 * 1024)

char  *temp_strdup(const char *cstr);
void  *temp_alloc(size_t size);
char  *temp_sprintf(const char *format, ...);
void   temp_reset(void);
size_t temp_size(void);
void   temp_free(size_t size);
void   temp_rewind(size_t checkpoint);

typedef struct {
    f32 x;
    f32 y;
} vec2_t;
#define vec2(X, Y)              ((vec2_t){.x = X, .y = Y})
#define vec2_add(vl, vr)        vec2((vl).x + (vr).x, (vl).y + (vr).y)
#define vec2_sub(vl, vr)        vec2((vl).x - (vr).x, (vl).y - (vr).y)
#define vec2_mul(vl, vr)        vec2((vl).x *(vr).x, (vl).y *(vr).y)
#define vec2_div(vl, vr)        vec2((vl).x / (vr).x, (vl).y / (vr).y)
#define vec2_addf(vl, r)        vec2((vl).x + (r), (vl).y + (r))
#define vec2_subf(vl, r)        vec2((vl).x - (r), (vl).y - (r))
#define vec2_mulf(vl, r)        vec2((vl).x *(r), (vl).y *(r))
#define vec2_divf(vl, r)        vec2((vl).x / (r), (vl).y / (r))
#define vec2_add_assign(vl, vr) ((vl)->x += (vr).x, (vl)->y += (vr).y)
#define vec2_sub_assign(vl, vr) ((vl)->x -= (vr).x, (vl)->y -= (vr).y)
#define vec2_mul_assign(vl, vr) ((vl)->x *= (vr).x, (vl)->y *= (vr).y)
#define vec2_div_assign(vl, vr) ((vl)->x /= (vr).x, (vl)->y /= (vr).y)
#define vec2_add_assignf(vl, r) ((vl)->x += (r), (vl)->y += (r))
#define vec2_sub_assignf(vl, r) ((vl)->x -= (r), (vl)->y -= (r))
#define vec2_mul_assignf(vl, r) ((vl)->x *= (r), (vl)->y *= (r))
#define vec2_div_assignf(vl, r) ((vl)->x /= (r), (vl)->y /= (r))
#define vec2_lenght(v)          sqrt((v).x *(v).x + (v).y * (v).y)
vec2_t vec2_normalize(vec2_t v);

void   mssleep(u32 ms);
#endif
