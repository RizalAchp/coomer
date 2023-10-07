#include "util.h"

static inline const char *shift_args(int *c, char ***v) {
    if (c == 0) return NULL;
    char *result = **v;
    (*v) += 1;
    (*c) -= 1;
    return result;
}

static inline bool options_cmp_impl(const char *opt, const char *opt_short, const char *opt_long, size_t opt_len, size_t opt_short_len, size_t opt_long_len) {
    if (opt_len == opt_short_len) return memcmp(opt, opt_short, opt_len) == 0;
    if (opt_len == opt_long_len) return memcmp(opt, opt_long, opt_len) == 0;
    return false;
}

#define options_cmp_arg(opt, optshort_lit, optlong_lit, ...)                                                                \
    if (options_cmp_impl(opt, optshort_lit, optlong_lit, strlen(opt), sizeof(optshort_lit) - 1, sizeof(optlong_lit) - 1)) { \
        const char *arg = shift_args(argc, argv);                                                                           \
        if (arg == NULL) {                                                                                                  \
            coom_error("options '" optlong_lit "," optlong_lit "' required second arguments.");                             \
            print_help(program_name);                                                                                       \
            return 1;                                                                                                       \
        }                                                                                                                   \
        __VA_ARGS__                                                                                                         \
        continue;                                                                                                           \
    }
#define options_cmp(opt, optshort_lit, optlong_lit, ...)                                                                    \
    if (options_cmp_impl(opt, optshort_lit, optlong_lit, strlen(opt), sizeof(optshort_lit) - 1, sizeof(optlong_lit) - 1)) { \
        __VA_ARGS__                                                                                                         \
        continue;                                                                                                           \
    }

typedef struct {
    bool        windowed;
    bool        select;
    f32         delay_second;
    const char *new_config;
    const char *config;
} options_args;

#define OPTIONS_ARGS_DEFAULT ((options_args){.windowed = false, .delay_second = 0, .new_config = NULL, .config = NULL})
