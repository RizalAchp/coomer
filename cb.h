#pragma once

#ifndef CB_H_
#    define CB_H_

// #    ifdef CB_TESTS
#    define CB_IMPLEMENTATION
// #    endif

#    define CB_ASSERT(...)        assert(__VA_ARGS__)
#    define CB_REALLOC(old, size) realloc(old, size)
#    define CB_FREE(ptr)           \
        do {                       \
            if (ptr != NULL) {     \
                free((void *)ptr); \
                (ptr) = NULL;      \
            }                      \
        } while (0)
#    define CB_ASSERT_ALLOC(PTR) CB_ASSERT((PTR) && "Buy more RAM lol")

#    include <assert.h>
#    include <ctype.h>
#    include <errno.h>
#    include <stdarg.h>
#    include <stdbool.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

#    if defined(__APPLE__) || defined(__MACH__)
#        define CB_MACOS
#        define CB_DEFAULT_PLATFORM CB_PLATFORM_MACOS
#        define CB_PATH_SEPARATOR   ':'
#        define CB_DIR_SEPARATOR    '/'
#        define CB_LINE_END         "\n"
#    elif defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)
#        define CB_WINDOWS
#        define CB_DEFAULT_PLATFORM CB_PLATFORM_WINDOWS
#        define WIN32_LEAN_AND_MEAN
#        include <direct.h>
#        include <shellapi.h>
#        include <windows.h>
#        define getcwd(buff, size) GetCurrentDirectory(size, buff)
#        define access             _access
#        define F_OK               0
#        define CB_PATH_SEPARATOR  ';'
#        define CB_DIR_SEPARATOR   '\\'
#        define CB_LINE_END        "\r\n"
#        define mkdir(p, _)        mkdir(p)
#        define realpath(N, R)     _fullpath((R), (N), MAX_PATH)
typedef HANDLE cb_proc_t;
#        define CB_INVALID_PROC    INVALID_HANDLE_VALUE
struct dirent {
    char d_name[MAX_PATH + 1];
};
typedef struct DIR DIR;
DIR               *opendir(const char *dirpath);
struct dirent     *readdir(DIR *dirp);
int                closedir(DIR *dirp);
#    elif defined(__linux__) && defined(__unix__)
#        define CB_UNIX
#        define CB_DEFAULT_PLATFORM CB_PLATFORM_UNIX
#        include <dirent.h>
#        include <fcntl.h>
#        include <limits.h>
#        include <sys/stat.h>
#        include <sys/types.h>
#        include <sys/wait.h>
#        include <unistd.h>
#        define MAX_PATH          PATH_MAX
#        define CB_PATH_SEPARATOR ':'
#        define CB_DIR_SEPARATOR  '/'
#        define CB_LINE_END       "\n"
typedef int cb_proc_t;
#        define CB_INVALID_PROC   (-1)
#    else
#        error "Platform: Unknown platform, not supported platform"
#    endif
#    if defined(__GNUC__)
#        define CB_DEFAULT_COMPILER CB_COMPILER_GNU
#    elif defined(__clang__)
#        define CB_DEFAULT_COMPILER CB_COMPILER_CLANG
#    elif defined(_MSC_VER)
#        error "msvc not supported"
#    endif

#    ifndef CB_FNDEF
#        define CB_FNDEF
#    endif
// clang-format off
#    define CB_ARR_LEN(array)         (sizeof(array) / sizeof(array[0]))
#    define CB_ARR_GET(array, index)  (CB_ASSERT(index >= 0), CB_ASSERT(index < CB_ARR_LEN(array)), array[index])

#    define CB_FATAL(...)           cb_log(CB_LOG_FATAL, __VA_ARGS__)
#    define CB_ERROR(...)           if (g_log_level <= CB_LOG_ERROR) cb_log(CB_LOG_ERROR, __VA_ARGS__)
#    define CB_WARNING(...)         if (g_log_level <= CB_LOG_WARNING) cb_log(CB_LOG_WARNING, __VA_ARGS__)
#    define CB_INFO(...)            if (g_log_level <= CB_LOG_INFO) cb_log(CB_LOG_INFO, __VA_ARGS__)
#    define CB_BAIL_ERROR(RET, ...) { CB_ERROR(__VA_ARGS__);  RET; }
#    define CB_WHITESPACE           ' \t\r\n'

#    define DECL_ARR(TYPE, ITEMTYPE) typedef struct { size_t count; size_t capacity; ITEMTYPE *items; } TYPE

#    define CB_DA_INIT_CAP                256
#    define cb_da_first(da)               (CB_ASSERT((da)->count >= 0 && (da)->items != NULL),  &(da)->items[0])
#    define cb_da_last(da)                (CB_ASSERT((da)->count > 0 && (da)->items != NULL),   &(da)->items[(da)->count - 1])
#    define cb_da_begin(da)               (&(da)->items[0])
#    define cb_da_end(da)                 (&(da)->items[(da)->count])
#    define cb_da_empty(da)               ((da)->count == 0 || (da)->items == NULL)
#    define cb_da_foreach(da, type, ...)  for (size_t idx = 0; idx < (da)->count; ++idx) { type *item = &(da)->items[idx]; __VA_ARGS__; }

#    define cb_da_append(da, item)  do { if ((da)->count >= (da)->capacity) { (da)->capacity = (da)->capacity == 0 ? CB_DA_INIT_CAP : (da)->capacity * 2; (da)->items    = CB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); CB_ASSERT_ALLOC((da)->items);}(da)->items[(da)->count++] = (item); } while (0)
#    define cb_da_free(da)          do { CB_FREE((da).items); (da).capacity = 0; (da).count = 0; } while(0)
// Append several items to a dynamic array
#    define cb_da_append_many(da, new_items, new_items_count)                                                                                         \
        do {                                                                                                                                          \
            if ((da)->count + new_items_count > (da)->capacity) {                                                                                     \
                if ((da)->capacity == 0) (da)->capacity = CB_DA_INIT_CAP; while ((da)->count + new_items_count > (da)->capacity) (da)->capacity *= 2; \
                (da)->items = CB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); CB_ASSERT_ALLOC((da)->items);                           \
            }                                                                                                                                         \
            memcpy((da)->items + (da)->count, new_items, new_items_count * sizeof(*(da)->items)); (da)->count += new_items_count;                     \
        } while (0)
#    define cb_return_defer(value)  do { result = (value); goto defer; } while (0)
#    define NOT_IMPLEMENTED         CB_BAIL_ERROR(exit(1), "Not Implemented - %s", __PRETTY_FUNCTION__)

// TODO: add MinGW support
#    ifndef CB_REBUILD_ARGS
#        if CB_WINDOWS
#            if defined(__GNUC__)
#                define CB_REBUILD_ARGS(binary_path, source_path) "gcc", "-o", binary_path, source_path
#            elif defined(__clang__)
#                define CB_REBUILD_ARGS(binary_path, source_path) "clang", "-o", binary_path, source_path
#            elif defined(_MSC_VER)
#                define CB_REBUILD_ARGS(binary_path, source_path) "cl.exe", source_path
#            endif
#        else
#            define CB_REBUILD_ARGS(binary_path, source_path) "cc", "-o", binary_path, source_path
#        endif
#    endif
#    define CB_REBUILD_SELF(argc, argv)                                                     \
        do {                                                                                \
            const char *source_path = __FILE__;                                             \
            assert(argc >= 1);                                                              \
            const char *binary_path       = argv[0];                                        \
            int         rebuild_is_needed = cb_needs_rebuild(binary_path, &source_path, 1); \
            if (rebuild_is_needed < 0) exit(1);                                             \
            if (rebuild_is_needed) {                                                        \
                cb_str_builder_t sb = {0};                                                  \
                cb_sb_append_cstr(&sb, binary_path);                                        \
                cb_sb_append_cstr(&sb, ".old");                                             \
                cb_sb_append_null(&sb);                                                     \
                                                                                            \
                if (!cb_rename_path(binary_path, sb.items)) exit(1);                             \
                cb_cmd_t rebuild = {0};                                                     \
                cb_cmd_append(&rebuild, CB_REBUILD_ARGS(binary_path, source_path));         \
                bool rebuild_succeeded = cb_cmd_run_sync(rebuild);                          \
                cb_cmd_free(rebuild);                                                       \
                if (!rebuild_succeeded) {                                                   \
                    cb_rename_path(sb.items, binary_path);                                       \
                    exit(1);                                                                \
                }                                                                           \
                cb_cmd_t cmd = {0};                                                         \
                cb_da_append_many(&cmd, argv, argc);                                        \
                if (!cb_cmd_run_sync(cmd)) exit(1);                                         \
                exit(0);                                                                    \
            }                                                                               \
        } while (0)
// clang-format on

typedef enum { CB_ERR, CB_OK, CB_FAIL } cb_status_t;
typedef enum { CB_LOG_NONE, CB_LOG_INFO, CB_LOG_WARNING, CB_LOG_ERROR, CB_LOG_FATAL, CB_LOG_LEVEL_MAX } cb_log_level_t;
typedef enum { CB_FILE_TYPE_ERROR = -1, CB_FILE_REGULAR = 0, CB_FILE_DIRECTORY, CB_FILE_SYMLINK, CB_FILE_OTHER } cb_file_type_t;

typedef enum {
    CB_SUBCMD_NOOP,
    CB_SUBCMD_BUILD,
    CB_SUBCMD_CONFIG,
    CB_SUBCMD_TESTS,
    CB_SUBCMD_CLEAN,
    CB_SUBCMD_INSTALL,
    CB_SUBCMD_MAX,
} cb_subcmd_t;
// clang-format off
typedef enum { CB_BUILD_TYPE_DEBUG, CB_BUILD_TYPE_RELEASE,      CB_BUILD_TYPE_RELEASEDEBUG, CB_BUILD_TYPE_MAX                   } cb_build_t;
typedef enum { CB_PLATFORM_UNKNOWN, CB_PLATFORM_WINDOWS,        CB_PLATFORM_MACOS,          CB_PLATFORM_UNIX,   CB_PLATFORM_MAX } cb_platform_t;
typedef enum { CB_ARCH_UNKNOWN,     CB_ARCH_X64,                CB_ARCH_X86, CB_ARCH_ARM64, CB_ARCH_ARM32,      CB_ARCH_MAX     } cb_arch_t;
typedef enum { CB_COMPILER_UNKNOWN, CB_COMPILER_CLANG,          CB_COMPILER_GNU,            CB_COMPILER_MAX                     } cb_compiler_t;
typedef enum { CB_PROGRAM_UNKNOWN,  CB_PROGRAM_C,               CB_PROGRAM_CPP,             CB_PROGRAM_MAX                      } cb_program_t;
typedef enum { 
    CB_TARGET_TYPE_EXEC = 0,
    CB_TARGET_TYPE_STATIC_LIB,
    CB_TARGET_TYPE_DYNAMIC_LIB,
    CB_TARGET_TYPE_TESTS,
    CB_TARGET_TYPE_SYSTEM_LIB,
    CB_TARGET_TYPE_MAX
} cb_target_type_t;
// clang-format on

CB_FNDEF void cb_log(cb_log_level_t level, const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));
// It is an equivalent of shift command from bash. It basically pops a command line
// argument from the beginning.

typedef struct cb_path_t        cb_path_t;
typedef struct cb_str_builder_t cb_str_builder_t;
typedef struct cb_procs_t       cb_procs_t;
// A command - the main workhorse of Cb. Cb is all about building commands an running them
typedef struct cb_cmd_t        cb_cmd_t;
typedef struct cb_temp_alloc_t cb_temp_alloc_t;
typedef struct cb_strview_t    cb_strview_t;
typedef struct cb_set_t        cb_set_t;

typedef struct cb_config_t     cb_config_t;
typedef struct cb_target_t     cb_target_t;
typedef struct cb_t            cb_t;

/// cb_path_t /////////////////////////////////////////////
struct cb_path_t {
    size_t count;
    char   data[MAX_PATH];
};
DECL_ARR(cb_paths_t, cb_path_t);
CB_FNDEF void         cb_path_copy(cb_path_t *dest, cb_path_t src);
CB_FNDEF void         cb_path_move(cb_path_t *dest, cb_path_t *src);

CB_FNDEF cb_path_t    cb_path(const char *str);
CB_FNDEF cb_path_t    cb_path_parts(const char *str, size_t len);
CB_FNDEF cb_strview_t cb_path_extension(cb_path_t *path);
CB_FNDEF cb_strview_t cb_path_filename(cb_path_t *path);
CB_FNDEF cb_status_t  cb_path_append(cb_path_t *path, cb_strview_t other);
CB_FNDEF bool         cb_path_with_extension(cb_path_t *path, char *ext);
CB_FNDEF bool         cb_path_has_extension(cb_path_t *path);
CB_FNDEF bool         cb_path_to_absolute_path(cb_path_t *path);
#    define cb_path_append_cstr(p, s)  cb_path_append(p, cb_sv(s))
#    define cb_path_is_regular_file(p) (cb_get_file_type((p)->data) == CB_FILE_REGULAR)
#    define cb_path_is_directory(p)    (cb_get_file_type((p)->data) == CB_FILE_DIRECTORY)
#    define cb_path_is_symlink(p)      (cb_get_file_type((p)->data) == CB_FILE_SYMLINK)
#    define cb_path_exists(p)          cb_file_exists((p)->data)
#    define cb_path_to_cstr(p)         ((p)->data[(p)->count] = 0, (p)->data)
#    define cb_path_to_strview(p)      cb_sv_from_parts((p)->data, (p)->count)
#    define cb_path_empty(p)           ((p)->count == 0)

/// os operation /////////////////////////////////////////////
CB_FNDEF cb_file_type_t cb_get_file_type(const char *path);
CB_FNDEF bool           cb_mkdir_if_not_exists(const char *path);
CB_FNDEF bool           cb_remove_dir_if_exists(const char *path);
CB_FNDEF bool           cb_rename_path(const char *old_path, const char *new_path);
CB_FNDEF bool           cb_copy_file(const char *dest_path, const char *src_path);
CB_FNDEF bool           cb_current_dir(cb_path_t *out_path, char *optional_append_path);
CB_FNDEF bool           cb_home_dir(cb_path_t *out_path, char *optional_append_path);
typedef bool (*on_dirent_cb)(cb_file_type_t ftype, cb_path_t *, void *args);
CB_FNDEF bool cb_walkdir(const char *parent, bool recursive, on_dirent_cb on_dirent_calback, void *args);

// RETURNS:
//  0 - does not to be needs rebuild
//  1 - does needs rebuild
// -1 - error. The error is logged
CB_FNDEF int cb_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
// RETURNS:
//  0 - file does not exists
//  1 - file exists
// -1 - error while checking if file exists. The error is logged
CB_FNDEF int cb_file_exists(const char *file_path);

/// cb_str_builder_t /////////////////////////////////////////////
struct cb_str_builder_t {
    size_t capacity;
    size_t count;
    char  *items;
};
// clang-format off
#    define cb_sb_append_buf(sb, buf, size) cb_da_append_many(sb, buf, size)
#    define cb_sb_append_cstr(sb, cstr)  do {const char *s = (cstr); size_t n = strlen(s); cb_da_append_many(sb, s, n); } while (0)
#    define cb_sb_append_null(sb) cb_da_append_many(sb, "", 1)
#    define cb_sb_free(sb)        CB_FREE((sb).items)
// clang-format on

/// cb_proc_t | cb_procs_t /////////////////////////////////////////////
struct cb_procs_t {
    size_t     capacity;
    size_t     count;
    cb_proc_t *items;
};

// Wait until the process has finished
CB_FNDEF bool cb_procs_wait(cb_procs_t procs);
CB_FNDEF bool cb_proc_wait(cb_proc_t proc);

/// cb_cmd_t /////////////////////////////////////////////
struct cb_cmd_t {
    size_t       capacity;
    size_t       count;
    const char **items;
};

#    define cb_cmd_append(cmd, ...) cb_da_append_many(cmd, ((const char *[]){__VA_ARGS__}), (sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char *)))
#    define cb_cmd_free(cmd)        CB_FREE(cmd.items)
CB_FNDEF cb_proc_t   cb_cmd_run_async(cb_cmd_t cmd);
CB_FNDEF bool        cb_cmd_run_sync(cb_cmd_t cmd);
CB_FNDEF cb_status_t cb_popen_stdout(const char *cmd, cb_str_builder_t *stdout_content);

/// temp allocator /////////////////////////////////////////////
#    ifndef CB_TEMP_CAPACITY
#        define CB_TEMP_CAPACITY (8 << 10 << 10)
#    endif  // CB_TEMP_CAPACITY
struct cb_temp_alloc_t {
    size_t size;
    size_t last;
    char   data[CB_TEMP_CAPACITY];
};
CB_FNDEF char  *cb_temp_strdup(const char *cstr);
CB_FNDEF void  *cb_temp_alloc(size_t size);
CB_FNDEF char  *cb_temp_sprintf(const char *format, ...) __attribute__((__format__(__printf__, 1, 2)));
CB_FNDEF size_t cb_temp_save(void);
CB_FNDEF void   cb_temp_rewind(size_t checkpoint);
CB_FNDEF void   cb_temp_reset(void);
CB_FNDEF void   cb_temp_reset_last(void);

/// cb_strview_t /////////////////////////////////////////////
struct cb_strview_t {
    size_t count;
    char  *data;
};
#    define cb_sv_from_parts(D, C)  ((cb_strview_t){.count = C, .data = (char *)D})
#    define cb_sv(cstr)             cb_sv_from_parts(cstr, ((sizeof(cstr) != sizeof(void *)) ? (sizeof(cstr) - 1) : strlen(cstr)))
#    define cb_sv_start_with(sv, c) ((sv)->data[0] == c)
#    define cb_sv_end_with(sv, c)   ((sv)->data[(sv)->count - 1] == c)

CB_FNDEF cb_strview_t cb_sv_chop_right_by_delim(cb_strview_t *sv, char delim);
CB_FNDEF cb_strview_t cb_sv_chop_left_by_delim(cb_strview_t *sv, char delim);
CB_FNDEF bool         cb_sv_eq(cb_strview_t a, cb_strview_t b);
CB_FNDEF const char  *cb_sv_to_cstr(cb_strview_t sv);

#    define SVFmt     "%.*s"
#    define SVArg(sv) (int)(sv).count, (sv).data

/// cb_set_t /////////////////////////////////////////////
///
typedef struct {
    cb_strview_t item;
    int64_t      hash;
} cb_set_item_t;
struct cb_set_t {
    size_t         capacity;
    size_t         count;
    cb_set_item_t *items;
};
#    define cb_set_empty(set) ((set)->count == 0 || (set)->items == NULL)
#    define cb_set_begin      cb_da_begin
#    define cb_set_end        cb_da_end
CB_FNDEF cb_set_t    cb_set_create(void);
CB_FNDEF void        cb_set_delete(cb_set_t *set);

CB_FNDEF cb_status_t cb_set_copy(cb_set_t *set_dst, cb_set_t *set_src);
CB_FNDEF cb_status_t cb_set_move(cb_set_t *set_dst, cb_set_t *set_src);
CB_FNDEF cb_status_t cb_set_swap(cb_set_t *set_dst, cb_set_t *set_src);
CB_FNDEF cb_status_t cb_set_remove(cb_set_t *set, cb_strview_t key);
CB_FNDEF cb_status_t cb_set_insert(cb_set_t *set, cb_strview_t key);
CB_FNDEF cb_status_t cb_set_contains(cb_set_t *set, cb_strview_t key);
CB_FNDEF cb_status_t cb_set_insert_many_impl(cb_set_t *set, ...);
// helper macro for inserting item with sized tipe or cstr
#    define cb_set_remove_cstr(set, key)   (cb_set_remove(set, cb_sv_from_parts(key, strlen(key) + 1)))
#    define cb_set_insert_cstr(set, key)   (cb_set_insert(set, cb_sv_from_parts(key, strlen(key) + 1)))
#    define cb_set_inserts_cstr(set, ...)  (cb_set_insert_many_impl(set, key, __VA_ARGS__, NULL))
#    define cb_set_contains_cstr(set, key) (cb_set_contains(set, cb_sv_from_parts(key, strlen(key) + 1)))
#    define cb_set_hash_key_cstr(set, key) (cb_set_hash_key_parts(set, cb_sv_from_parts(key, strlen(key) + 1)))
#    define cb_set_index(set, idx)         (CB_ASSERT(set), CB_ASSERT((idx <= (set)->count) && (0 <= idx)), &set->items[idx])
#    define cb_set_index_key(set, idx)     (cb_set_index(set))->item

extern cb_log_level_t g_log_level;
/// cb_config_t /////////////////////////////////////////////
struct cb_config_t {
    cb_build_t    build_type;
    cb_platform_t platform;
    cb_arch_t     arch;
    cb_compiler_t compiler_type;
    cb_program_t  program_type;

    cb_path_t     project_path;
    cb_path_t     build_path;
    cb_path_t     build_artifact_path;
    cb_path_t     compiler_path;
    cb_path_t     config_path;
    cb_path_t     targets_path;

    cb_path_t     install_prefix;
};

/// cb_target_t /////////////////////////////////////////////
typedef struct {
    cb_path_t source;
    cb_path_t output;
} cb_source_t;
DECL_ARR(cb_target_sources_t, cb_source_t);

struct cb_target_t {
    cb_target_type_t    type;
    cb_strview_t        name;
    cb_path_t           output_dir;
    cb_path_t           output;

    cb_set_t            flags;
    cb_set_t            includes;
    cb_set_t            ldflags;
    cb_target_sources_t sources;
};

CB_FNDEF cb_target_t cb_target_create(cb_strview_t name, cb_target_type_t type);
CB_FNDEF void        cb_target_delete(cb_target_t *tg);

CB_FNDEF cb_status_t cb_target_add_sources_with_ext(cb_target_t *tg, const char *dir, char *ext, bool recursive);
CB_FNDEF cb_status_t cb_target_add_sources(cb_target_t *tg, ...);
CB_FNDEF cb_status_t cb_target_add_flags(cb_target_t *tg, ...);
CB_FNDEF cb_status_t cb_target_add_includes(cb_target_t *tg, ...);
CB_FNDEF cb_status_t cb_target_add_defines(cb_target_t *tg, ...);
CB_FNDEF cb_status_t cb_target_link_library(cb_target_t *tg, ...);
CB_FNDEF cb_status_t cb_target_as_cmd(cb_target_t *tg, cb_cmd_t *cmd);
CB_FNDEF bool        cb_target_need_rebuild(cb_target_t *tg);

CB_FNDEF bool        cb_target_run(cb_target_t *tg);
typedef cb_status_t (*cb_callback_fn)(cb_t *cb, cb_config_t *cfg);
/// cb_t /////////////////////////////////////////////
struct cb_t {
    size_t         capacity;
    size_t         count;
    cb_target_t   *items;

    cb_callback_fn on_pre_build;
    cb_callback_fn on_post_build;
};
// init cb_t returning pointer of the `cb_t`, if error, return `NULL`
// `cb_t` is created in head, it should `cb_deinit` after using it
CB_FNDEF cb_t        *cb_init(int argc, char **argv);
CB_FNDEF cb_status_t  cb_run(cb_t *cb);
CB_FNDEF cb_status_t  cb_dump_compile_commands(cb_t *cb);
CB_FNDEF void         cb_deinit(cb_t *cb);
CB_FNDEF cb_target_t *cb_create_target_impl(cb_t *cb, cb_strview_t name, cb_target_type_t type);
#    define cb_create_target(cb, name, type) cb_create_target_impl(cb, cb_sv(name), type)
#    define cb_create_exec(cb, name)         cb_create_target(cb, name, CB_TARGET_TYPE_EXEC)
#    define cb_create_tests(cb, name)        cb_create_target(cb, name, CB_TARGET_TYPE_TESTS)
#    define cb_create_static_lib(cb, name)   cb_create_target(cb, name, CB_TARGET_TYPE_STATIC_LIB)
#    define cb_create_dynamic_lib(cb, name)  cb_create_target(cb, name, CB_TARGET_TYPE_DYNAMIC_LIB)

CB_FNDEF void         cb_add_on_pre_build_callback(cb_t *cb, cb_callback_fn callback);
CB_FNDEF void         cb_add_on_post_build_callback(cb_t *cb, cb_callback_fn callback);
CB_FNDEF cb_target_t *cb_create_target_pkgconf(cb_t *cb, cb_strview_t name);

// user self implementation function for configuring build
CB_FNDEF cb_status_t on_configure(cb_t *cb, cb_config_t *cfg);
#endif  // CB_H_

////////////////////////////////////////////////////////////////////////////////
#ifdef CB_IMPLEMENTATION

#    ifdef CB_WINDOWS
struct DIR {
    HANDLE          hFind;
    WIN32_FIND_DATA data;
    struct dirent  *dirent;
};
#    endif

const char *CB_BUILD_TYPE_DISPLAY[CB_BUILD_TYPE_MAX]   = {"DEBUG", "RELEASE", "RELDEBUG"};
const char *CB_PLATFORM_DISPLAY[CB_PLATFORM_MAX]       = {"N/A", "WINDOWS", "MACOS", "UNIX"};
const char *CB_ARCH_DISPLAY[CB_ARCH_MAX]               = {"N/A", "X64", "X86", "ARM64", "ARM32"};
const char *CB_COMPILER_DISPLAY[CB_COMPILER_MAX]       = {"N/A", "CLANG", "GNU"};
const char *CB_PROGRAM_DISPLAY[CB_PROGRAM_MAX]         = {"N/A", "C", "CPP"};
const char *CB_TARGET_TYPE_DISPLAY[CB_TARGET_TYPE_MAX] = {"executable", "staticlib", "dynamiclib", "tests", "systemlib"};
#    define ARRLEN(ARR)           (sizeof((ARR)) / sizeof((ARR)[0]))
#    define CB_DISPLAY(NAME, IDX) NAME##_DISPLAY[(IDX) % NAME##_MAX]

static cb_status_t cb_find_compiler(cb_path_t *compiler_path);

static const char *CB_LOG_LEVEL_DISPLAY[] = {"", "INFO", "WARN", "ERROR", "FATAL"};
static const char *program_name           = NULL;
static bool        g_display_config       = false;
static cb_subcmd_t g_subcmd               = CB_SUBCMD_NOOP;

cb_log_level_t     g_log_level            = CB_LOG_INFO;
static cb_config_t g_cfg                  = {
                     .build_type    = CB_BUILD_TYPE_DEBUG,
                     .platform      = CB_DEFAULT_PLATFORM,
                     .arch          = CB_ARCH_X64,
                     .compiler_type = CB_DEFAULT_COMPILER,
                     .program_type  = CB_PROGRAM_C,
};

#    define is_debug()          (g_cfg.build_type == CB_BUILD_TYPE_DEBUG || g_cfg.build_type == CB_BUILD_TYPE_RELEASEDEBUG)
#    define is_release()        (g_cfg.build_type == CB_BUILD_TYPE_RELEASE || g_cfg.build_type == CB_BUILD_TYPE_RELEASEDEBUG)
#    define is_compiler_gnu()   (g_cfg.compiler_type == CB_COMPILER_GNU)
#    define is_compiler_clang() (g_cfg.compiler_type == CB_COMPILER_CLANG)

static inline void cb_config_display(void) {
    printf("config.build_type          = %s" CB_LINE_END, CB_DISPLAY(CB_BUILD_TYPE, g_cfg.build_type));
    printf("config.platform            = %s" CB_LINE_END, CB_DISPLAY(CB_PLATFORM, g_cfg.platform));
    printf("config.arch                = %s" CB_LINE_END, CB_DISPLAY(CB_ARCH, g_cfg.arch));
    printf("config.compiler_type       = %s" CB_LINE_END, CB_DISPLAY(CB_COMPILER, g_cfg.compiler_type));
    printf("config.program_type        = %s" CB_LINE_END, CB_DISPLAY(CB_PROGRAM, g_cfg.program_type));
    printf("config.project_path        = '%*s'" CB_LINE_END, SVArg(g_cfg.project_path));
    printf("config.build_path          = '%*s'" CB_LINE_END, SVArg(g_cfg.build_path));
    printf("config.build_artifact_path = '%*s'" CB_LINE_END, SVArg(g_cfg.build_artifact_path));
    printf("config.compiler_path       = '%*s'" CB_LINE_END, SVArg(g_cfg.compiler_path));
    printf("config.config_path         = '%*s'" CB_LINE_END, SVArg(g_cfg.config_path));
    printf("config.install_prefix      = '%*s'" CB_LINE_END, SVArg(g_cfg.install_prefix));
}

static inline char *cb_config_get_ext(cb_target_type_t type) {
    switch (type) {
        case CB_TARGET_TYPE_STATIC_LIB: return "a";
        case CB_TARGET_TYPE_DYNAMIC_LIB: return (g_cfg.platform == CB_PLATFORM_WINDOWS) ? "dll" : "so";
        case CB_TARGET_TYPE_EXEC: return (g_cfg.platform == CB_PLATFORM_WINDOWS) ? "exe" : "";
        default: return "";
    }
}

static const char CB_HEADER_BYTE[] = {'C', 'B', '6', '9', '4', '2', '0'};
#    define CB_HEADER_BYTE_SIZE     sizeof(CB_HEADER_BYTE)
#    define cb_bin_write_header(fp) fwrite(CB_HEADER_BYTE, CB_HEADER_BYTE_SIZE, 1, fp)
static inline bool cb_bin_read_header(FILE *fp) {
    char temp[CB_HEADER_BYTE_SIZE] = {0};
    fread(temp, CB_HEADER_BYTE_SIZE, 1, fp);
    return (memcmp(CB_HEADER_BYTE, temp, CB_HEADER_BYTE_SIZE) == 0);
}
#    define cb_bin_write_prim(fp, prim) fwrite(&(prim), 1, sizeof(prim), fp)
#    define cb_bin_read_prim(fp, prim)  fread(&(prim), 1, sizeof(prim), fp)
#    define cb_bin_read_path(fp, path)  (cb_bin_read_prim(fp, (path)->count), fread((path)->data, 1, (path)->count, fp))
#    define cb_bin_write_path(fp, path) (cb_bin_write_prim(fp, (path)->count), fwrite((path)->data, 1, (path)->count, fp))
#    define cb_bin_read_sv(fp, sv)      (cb_bin_read_prim(fp, (sv)->count), (sv)->data = cb_temp_alloc((sv)->count + 1), fread((sv)->data, (sv)->count, 1, fp))
#    define cb_bin_write_sv(fp, sv)     (cb_bin_write_prim(fp, (sv)->count), fwrite((sv)->data, (sv)->count, 1, fp))

static inline cb_status_t cb_bin_read_set(FILE *fp, cb_set_t *set) {
    cb_bin_read_prim(fp, set->count);
    set->items = CB_REALLOC(NULL, set->count * sizeof(cb_set_item_t));
    CB_ASSERT_ALLOC(set->items);
    set->capacity = set->count;
    for (size_t i = 0; i < set->count; ++i) {
        cb_set_item_t *it = &set->items[i];
        cb_bin_read_prim(fp, it->hash);
        cb_bin_read_sv(fp, &it->item);
    }
    if (errno) CB_BAIL_ERROR(return CB_ERR, "Failed to write in bin: %s", strerror(errno));
    return CB_OK;
}
static inline cb_status_t cb_bin_write_set(FILE *fp, cb_set_t *set) {
    cb_bin_write_prim(fp, set->count);
    for (size_t i = 0; i < set->count; ++i) {
        cb_set_item_t *it = &set->items[i];
        cb_bin_write_prim(fp, it->hash);
        cb_bin_write_sv(fp, &it->item);
    }
    if (errno) CB_BAIL_ERROR(return CB_ERR, "Failed to write in bin: %s", strerror(errno));
    return CB_OK;
}

void cb_log(cb_log_level_t level, const char *fmt, ...) {
    fprintf(stderr, "[%s]: ", CB_LOG_LEVEL_DISPLAY[level % CB_LOG_LEVEL_MAX]);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static inline char *cb_shift_args(int *argc, char ***argv) {
    if (*argc == 0) return NULL;
    char *result = **argv;
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}

/// impl cb_path_t /////////////////////////////////////////////
void cb_path_copy(cb_path_t *dest, cb_path_t src) { memcpy(dest, &src, sizeof(cb_path_t)); }
void cb_path_move(cb_path_t *dest, cb_path_t *src) {
    cb_path_copy(dest, *src);
    src->count = 0;
}

cb_path_t cb_path(const char *str) { return cb_path_parts(str, strlen(str)); }
cb_path_t cb_path_parts(const char *str, size_t len) {
    CB_ASSERT((len < MAX_PATH) && "len of path is greather than MAX_PATH");
    cb_path_t p = {0};
    memset(p.data, 0, sizeof(p.data));
    p.count = len;
    memcpy(p.data, str, len);
    return p;
}
cb_strview_t cb_path_extension(cb_path_t *path) {
    cb_strview_t p = cb_sv_from_parts(path->data, path->count);
    return cb_sv_chop_right_by_delim(&p, '.');
}

cb_strview_t cb_path_filename(cb_path_t *path) {
    cb_strview_t temp = cb_sv_from_parts(path->data, path->count);
    if (cb_sv_end_with(&temp, CB_DIR_SEPARATOR)) temp.count -= 1;
    return cb_sv_chop_right_by_delim(&temp, CB_DIR_SEPARATOR);
}

cb_status_t cb_path_append(cb_path_t *path, cb_strview_t other) {
    if ((other.count + path->count) >= MAX_PATH) CB_BAIL_ERROR(return CB_ERR, "cb_path_append - is exceeded the (MAX_PATH=%d)", MAX_PATH);
    if (!cb_sv_end_with(path, CB_DIR_SEPARATOR)) {
        path->data[path->count] = CB_DIR_SEPARATOR;
        path->count++;
    }
    memcpy(&path->data[path->count], other.data, other.count);
    path->count             = path->count + other.count;
    path->data[path->count] = 0;
    return CB_OK;
}

bool cb_path_with_extension(cb_path_t *path, char *ext) {
    size_t       ext_len = strlen(ext);
    cb_strview_t tempp   = cb_sv_from_parts(path->data, path->count);
    cb_strview_t p       = cb_sv_chop_right_by_delim(&tempp, '.');
    if (p.count == 0)
        return false;  // doests has extension
                       //
    path->data[tempp.count] = '.';
    memcpy(path->data + (tempp.count + 1), ext, ext_len);
    path->count             = (tempp.count + 1) + ext_len;
    path->data[path->count] = 0;
    return true;
}
bool cb_path_has_extension(cb_path_t *path) { return cb_path_extension(path).count != 0; }
bool cb_path_to_absolute_path(cb_path_t *path) {
    bool result = true;
    if (realpath(path->data, path->data) == NULL) CB_BAIL_ERROR(return false, "Failed to get realpath (absolute path) from path '%s'", path->data);
    size_t bufflen = strlen(path->data);
    path->count    = bufflen;
    return result;
}

/// impl os operation /////////////////////////////////////////////
bool cb_mkdir_if_not_exists(const char *path) {
    if (mkdir(path, 0755) < 0) {
        if (errno == EEXIST) return true;
        return false;
    }
    CB_INFO("created directory `%s`", path);
    return true;
}

bool cb_remove_dir_if_exists(const char *dirpath) {
    bool result = true;
    DIR *dir    = opendir(dirpath);
    if (dir == NULL) CB_BAIL_ERROR(return false, "Could not open directory %s: %s", dirpath, strerror(errno));
    cb_file_type_t d_type = CB_FILE_TYPE_ERROR;
    cb_path_t      path   = {0};
    struct dirent *ent    = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0)) continue;
        path.count = snprintf(path.data, sizeof(path.data), "%s/%s", dirpath, ent->d_name);
        d_type     = cb_get_file_type(path.data);
        if (d_type == CB_FILE_TYPE_ERROR) continue;
        if (d_type == CB_FILE_DIRECTORY) result &= cb_remove_dir_if_exists(path.data);
        if (unlink(path.data) == 0) {
            CB_INFO("Removed file: '%*s'", SVArg(path));
        } else {
            CB_INFO("Can`t remove a file: %s\n", dirpath);
        }
    }
    if (rmdir(dirpath) == 0) {
        CB_INFO("Removed a directory: %s\n", dirpath);
    } else {
        CB_INFO("Can`t remove a directory: %s\n", dirpath);
    }

    if (dir) closedir(dir);
    return result;
}

bool cb_rename_path(const char *old_path, const char *new_path) {
    CB_INFO("renaming %s -> %s", old_path, new_path);
#    ifdef CB_WINDOWS
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING))
        CB_BAIL_ERROR(return false, "could not rename %s to %s: %lu", old_path, new_path, GetLastError());
#    else
    if (rename(old_path, new_path) < 0) CB_BAIL_ERROR(return false, "could not rename %s to %s: %s", old_path, new_path, strerror(errno));
#    endif  // CB_WINDOWS
    return true;
}

bool cb_copy_file(const char *dst_path, const char *src_path) {
    CB_INFO("Copying file from: %s => %s", src_path, dst_path);
#    ifdef CB_WINDOWS
    if (!CopyFile(src_path, dst_path, FALSE)) CB_BAIL_ERROR(return false, "Could not copy file: %lu", GetLastError());
    return true;
#    else
    bool   result   = true;
    int    src_fd   = -1;
    int    dst_fd   = -1;

    size_t buf_size = 32 * 1024;
    char  *buf      = CB_REALLOC(NULL, buf_size);
    CB_ASSERT_ALLOC(buf);

    src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) CB_BAIL_ERROR(cb_return_defer(false), "Could not open file %s: %s", src_path, strerror(errno));

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) CB_BAIL_ERROR(cb_return_defer(false), "Could not get mode of file %s: %s", src_path, strerror(errno));

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) CB_BAIL_ERROR(cb_return_defer(false), "Could not create file %s: %s", dst_path, strerror(errno));

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) break;
        if (n < 0) CB_BAIL_ERROR(cb_return_defer(false), "Could not read from file %s: %s", src_path, strerror(errno));

        char *buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) CB_BAIL_ERROR(cb_return_defer(false), "Could not write to file %s: %s", dst_path, strerror(errno));
            n -= m;
            buf2 += m;
        }
    }

defer:
    free(buf);
    close(src_fd);
    close(dst_fd);
    return result;
#    endif
}
bool cb_current_dir(cb_path_t *out_path, char *optional_append_path) {
    memset(out_path->data, 0, sizeof(out_path->data));
    if (getcwd(out_path->data, sizeof(out_path->data)) == NULL)
        CB_BAIL_ERROR(return false, "failed to get CWD (current working directory) - %s", strerror(errno));
    out_path->count = strlen(out_path->data);
    return (optional_append_path != NULL) ? (cb_path_append_cstr(out_path, optional_append_path) == CB_OK) : true;
}

bool cb_home_dir(cb_path_t *out_path, char *optional_append_path) {
    // TODO: implement windows equivalent
    char *home = NULL;
#    if !defined(CB_WINDOWS)
    if ((home = getenv("HOME")) == NULL) CB_BAIL_ERROR(return false, "Failed to get Home directory!");
#    else
    if ((home = getenv("USERPROFILE")) == NULL) {
        CB_ERROR("Failed to get env `USERPROFILE` try to get env `HOMEDRIVE` and `HOMEPATH`");
        char *drive = getenv("HOMEDRIVE");
        char *path  = getenv("HOMEPATH");
        if ((drive == NULL) || (path == NULL)) CB_BAIL_ERROR(return false, "Failed to get env `HOMEDRIVE` or `HOMEPATH` to get fullpath to home directory");
        *out_path = cb_path(drive);
        cb_path_append_cstr(out_path, path);
        return (optional_append_path != NULL) ? (cb_path_append_cstr(out_path, optional_append_path) == CB_OK) : true;
    }
#    endif  // CB_WINDOWS
    *out_path = cb_path(home);
    return (optional_append_path != NULL) ? (cb_path_append_cstr(out_path, optional_append_path) == CB_OK) : true;
}

bool cb_walkdir(const char *parent, bool recursive, on_dirent_cb on_dirent_calback, void *args) {
    bool result = true;
    DIR *dir    = opendir(parent);
    if (dir == NULL) CB_BAIL_ERROR(return false, "Could not open directory %s: %s", parent, strerror(errno));
    cb_file_type_t d_type = CB_FILE_TYPE_ERROR;
    cb_path_t      path   = {0};
    struct dirent *ent    = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0)) continue;
        path.count = snprintf(path.data, sizeof(path.data), "%s/%s", parent, ent->d_name);
        d_type     = cb_get_file_type(path.data);
        if (d_type == CB_FILE_TYPE_ERROR) continue;
        if (on_dirent_calback(d_type, &path, args) == false) cb_return_defer(false);
        if ((d_type == CB_FILE_DIRECTORY) && (recursive == true)) result &= cb_walkdir(path.data, recursive, on_dirent_calback, args);
    }
defer:
    if (dir) closedir(dir);
    return result;
}

int cb_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count) {
#    ifdef CB_WINDOWS
    BOOL   bSuccess;

    HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        CB_BAIL_ERROR(return -1, "Could not open file %s: %lu", output_path, GetLastError());
        return -1;
    }
    FILETIME output_path_time;
    bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!bSuccess) CB_BAIL_ERROR(return -1, "Could not get time of %s: %lu", output_path, GetLastError());

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path    = input_paths[i];
        HANDLE      input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        // NOTE: non-existing input is an error cause it is needed for building in the first place
        if (input_path_fd == INVALID_HANDLE_VALUE) CB_BAIL_ERROR(return -1, "Could not open file %s: %lu", input_path, GetLastError());
        FILETIME input_path_time;
        bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        CloseHandle(input_path_fd);
        if (!bSuccess) CB_BAIL_ERROR(return -1, "Could not get time of %s: %lu", input_path, GetLastError());
        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
    }

    return 0;
#    else
    struct stat statbuf = {0};

    if (stat(output_path, &statbuf) < 0) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (errno == ENOENT) return 1;
        CB_BAIL_ERROR(return -1, "could not stat %s: %s", output_path, strerror(errno));
    }
    int output_path_time = statbuf.st_mtime;
    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        if (stat(input_path, &statbuf) < 0) CB_BAIL_ERROR(return -1, "could not stat %s: %s", input_path, strerror(errno));
        int input_path_time = statbuf.st_mtime;
        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (input_path_time > output_path_time) return 1;
    }
    return 0;
#    endif
}
int cb_file_exists(const char *file_path) {
#    if CB_WINDOWS
    // TODO: distinguish between "does not exists" and other errors
    DWORD dwAttrib = GetFileAttributesA(file_path);
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
#    else
    struct stat statbuf;
    if (stat(file_path, &statbuf) < 0) {
        if (errno == ENOENT) return 0;
        CB_BAIL_ERROR(return -1, "Could not check if file %s exists: %s", file_path, strerror(errno));
    }
    return 1;
#    endif
}

/// impl cb_proc_t | cb_procs_t /////////////////////////////////////////////
bool cb_procs_wait(cb_procs_t procs) {
    bool success = true;
    for (size_t i = 0; i < procs.count; ++i) success &= cb_proc_wait(procs.items[i]);
    return success;
}
bool cb_proc_wait(cb_proc_t proc) {
    if (proc == CB_INVALID_PROC) return false;
#    ifdef CB_WINDOWS
    DWORD result = WaitForSingleObject(proc, INFINITE);
    if (result == WAIT_FAILED) CB_BAIL_ERROR(return false, "could not wait on child process: %lu", GetLastError());
    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) CB_BAIL_ERROR(return false, "could not get process exit code: %lu", GetLastError());
    if (exit_status != 0) CB_BAIL_ERROR(return false, "command exited with exit code %lu", exit_status);
    CloseHandle(proc);
#    else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) CB_BAIL_ERROR(return false, "could not wait on command (pid %d): %s", proc, strerror(errno));
        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) CB_BAIL_ERROR(return false, "command exited with exit code %d", exit_status);
            break;
        }
        if (WIFSIGNALED(wstatus)) CB_BAIL_ERROR(return false, "command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
    }
    return true;
#    endif
}

/// impl cb_cmd_t /////////////////////////////////////////////
cb_proc_t cb_cmd_run_async(cb_cmd_t cmd) {
    if (cmd.count < 1) CB_BAIL_ERROR(return CB_INVALID_PROC, "Could not run empty command");

    fprintf(stderr, "CMD: ");
    for (size_t i = 0; i < cmd.count; ++i) {
        const char *arg = cmd.items[i];
        if (arg == NULL) break;
        if (i > 0) fprintf(stderr, " ");
        if (!strchr(arg, ' ')) {
            fprintf(stderr, "%s", arg);
        } else {
            fprintf(stderr, "'%s'", arg);
        }
    }
    fprintf(stderr, CB_LINE_END);

#    ifdef CB_WINDOWS
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    cb_sb_append_null(&sb);
    BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    cb_sb_free(sb);

    if (!bSuccess) CB_BAIL_ERROR(return CB_INVALID_PROC, "Could not create child process: %lu", GetLastError());
    CloseHandle(piProcInfo.hThread);
    return piProcInfo.hProcess;
#    else
    pid_t cpid = fork();
    if (cpid < 0) CB_BAIL_ERROR(return CB_INVALID_PROC, "Could not fork child process: %s", strerror(errno));

    if (cpid == 0) {
        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        cb_cmd_t cmd_null = {0};
        cb_da_append_many(&cmd_null, cmd.items, cmd.count);
        cb_cmd_append(&cmd_null, NULL);

        if (execvp(cmd.items[0], (char *const *)cmd_null.items) < 0) CB_BAIL_ERROR(exit(1), "Could not exec child process: %s", strerror(errno));
        CB_ASSERT(0 && "unreachable");
    }

    return cpid;
#    endif
}

bool cb_cmd_run_sync(cb_cmd_t cmd) {
    cb_proc_t p = cb_cmd_run_async(cmd);
    if (p == CB_INVALID_PROC) return false;
    return cb_proc_wait(p);
}

cb_status_t cb_popen_stdout(const char *cmd, cb_str_builder_t *stdout_content) {
    cb_status_t result = CB_OK;
    char       *buffer = NULL;
#    ifdef CB_WINDOWS
    HANDLE              stdout_read_;
    HANDLE              stdout_write_;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi_info;

    // Create pipes for stdout
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    if (!CreatePipe(&stdout_read_, &stdout_write_, &sa, 0)) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Error creating pipes on Windows");

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb         = sizeof(STARTUPINFO);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = stdout_write_;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi_info, sizeof(PROCESS_INFORMATION));
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi_info))
        CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Could not create child process: %lu", GetLastError());

    buffer          = cb_temp_alloc(1 << 10);
    DWORD bytesRead = 0;
    while (ReadFile(stdout_read_, buffer, 1 << 10, &bytesRead, NULL) && bytesRead > 0) cb_sb_append_buf(stdout_content, buffer, bytesRead);

    CloseHandle(stdout_read_);
    CloseHandle(stdout_write_);
    CloseHandle(pi_info.hProcess);
    CloseHandle(pi_info.hThread);
#    else
    int   stdout_pipe[2];
    pid_t pid_proc;
    if (pipe(stdout_pipe) == -1) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Error creating pipes on Linux")
    if ((pid_proc = fork()) < 0) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Error forking process on Linux");

    if (pid_proc == 0) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        // Close unused pipe ends
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        char *const cmds[] = {"/bin/sh", "-c", (char *const)cmd, NULL};
        if (execvp(cmds[0], cmds) < 0) CB_BAIL_ERROR(exit(EXIT_FAILURE), "Could not exec child process: %s", strerror(errno));
        exit(EXIT_SUCCESS);
    } else {  // Parent process
        close(stdout_pipe[1]);
    }
    buffer            = cb_temp_alloc(2 << 10);
    ssize_t bytesRead = 0;
    while ((bytesRead = read(stdout_pipe[0], buffer, 2 << 10)) > 0) cb_sb_append_buf(stdout_content, buffer, bytesRead);
    close(stdout_pipe[0]);
    waitpid(pid_proc, NULL, 0);
#    endif  // CB_WINDOWS
defer:
    if (buffer) cb_temp_reset_last();
    return result;
}

cb_file_type_t cb_get_file_type(const char *path) {
#    ifdef CB_WINDOWS
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) CB_BAIL_ERROR(return CB_FILE_TYPE_ERROR, "Could not get file attributes of %s: %lu", path, GetLastError());
    if (attr & FILE_ATTRIBUTE_DIRECTORY) return CB_FILE_DIRECTORY;
    return CB_FILE_REGULAR;
#    else   // CB_WINDOWS
    struct stat statbuf;
    if (stat(path, &statbuf) < 0) CB_BAIL_ERROR(return CB_FILE_TYPE_ERROR, "Could not get stat of %s: %s", path, strerror(errno));
    switch (statbuf.st_mode & S_IFMT) {
        case S_IFDIR: return CB_FILE_DIRECTORY;
        case S_IFREG: return CB_FILE_REGULAR;
        case S_IFLNK: return CB_FILE_SYMLINK;
        default: return CB_FILE_OTHER;
    }
#    endif  // CB_WINDOWS
}

/// impl cb_temp_alloc_t /////////////////////////////////////////////
static cb_temp_alloc_t g_temp_alloc = {.size = 0, .last = 0, .data = {0}};

char                  *cb_temp_strdup(const char *cstr) {
    size_t n      = strlen(cstr);
    char  *result = cb_temp_alloc(n + 1);
    memcpy(result, cstr, n);
    result[n] = '\0';
    return result;
}

void *cb_temp_alloc(size_t size) {
    CB_ASSERT(((g_temp_alloc.size + size) < CB_TEMP_CAPACITY) && "[cb_temp_alloc] Extend the size of the predifined preprocessor `CB_TEMP_CAPACITY`");
    void *result      = &g_temp_alloc.data[g_temp_alloc.size];
    g_temp_alloc.last = size;
    g_temp_alloc.size += size;
    return result;
}

char *cb_temp_sprintf(const char *fmt, ...) {
    va_list args1;
    va_list args2;
    va_start(args1, fmt);
    int num = 1 + vsnprintf(NULL, 0, fmt, args1);
    va_end(args1);
    char *result = cb_temp_alloc(num + 1);
    va_start(args2, fmt);
    vsnprintf(result, num, fmt, args2);
    va_end(args2);
    return result;
}

size_t cb_temp_save(void) { return g_temp_alloc.size; }
void   cb_temp_rewind(size_t checkpoint) { g_temp_alloc.size = checkpoint; }
void   cb_temp_reset(void) { g_temp_alloc.size = 0; }
void   cb_temp_reset_last(void) {
    if (g_temp_alloc.size >= g_temp_alloc.last) g_temp_alloc.size -= g_temp_alloc.last;
}

/// impl cb_strview_t /////////////////////////////////////////////
cb_strview_t cb_sv_chop_right_by_delim(cb_strview_t *sv, char delim) {
    size_t i = sv->count;
    while (i-- && sv->data[i] != delim)
        ;
    cb_strview_t result = cb_sv_from_parts(sv->data + i + 1, sv->count - i - 1);
    sv->count -= (i < sv->count) ? (result.count + 1) : 0;
    return result;
}

bool        cb_sv_eq(cb_strview_t a, cb_strview_t b) { return (a.count != b.count) ? false : (memcmp(a.data, b.data, a.count) == 0); }
const char *cb_sv_to_cstr(cb_strview_t sv) {
    char *result = cb_temp_alloc(sv.count + 1);
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}
/// hash function for data structure `cb_set_t`
static inline int64_t __cb_hash(const char *key, size_t sz) {
    // FNV-1a hash (http://www.isthe.com/chongo/tech/comp/fnv/)
    int64_t h = 14695981039346656037ULL;  // FNV_OFFSET 64 bit
    for (size_t i = 0; i < sz; ++i) {
        h = h ^ (key)[i];
        h = h * 1099511628211ULL;  // FNV_PRIME 64 bit
    }
    return h;
}

/// impl cb_set_t /////////////////////////////////////////////
static inline cb_status_t __cb_set_contains_internal(cb_set_t *set, cb_strview_t item, int64_t hash, size_t *idxout) {
    if (cb_set_empty(set)) return CB_ERR;
    for (size_t i = 0; i < set->count; i++) {
        if (hash == set->items[i].hash && set->items[i].item.count == item.count) {
            if (cb_sv_eq(set->items[i].item, item)) {
                if (idxout) *idxout = i;
                return CB_OK;
            }
        }
    }
    return CB_ERR;
}
static inline cb_status_t __cb_set_insert_internal(cb_set_t *set, cb_set_item_t item) {
    if (__cb_set_contains_internal(set, item.item, item.hash, NULL) == CB_OK) return CB_ERR;
    cb_da_append(set, item);
    return CB_OK;
}
cb_set_t    cb_set_create(void) { return (cb_set_t){.capacity = 0, .count = 0, .items = NULL}; }
void        cb_set_delete(cb_set_t *set) { cb_da_free(*set); }
cb_status_t cb_set_copy(cb_set_t *set_dst, cb_set_t *set_src) {
    CB_ASSERT(set_dst && set_src && "src and dst is should not be NULL");
    if (set_src->count == 0) return CB_OK;
    for (size_t i = 0; i < set_src->count; i++)
        if (__cb_set_insert_internal(set_dst, set_src->items[i]) == CB_ERR) return CB_ERR;
    return CB_OK;
}
cb_status_t cb_set_move(cb_set_t *set_dst, cb_set_t *set_src) {
    CB_ASSERT(set_dst && set_src && "src and dst is should not be NULL");
    cb_status_t status = CB_OK;
    status &= cb_set_copy(set_dst, set_src);
    cb_set_delete(set_src);
    return status;
}
cb_status_t cb_set_swap(cb_set_t *set_dst, cb_set_t *set_src) {
    CB_ASSERT(set_dst && set_src && "src and dst is should not be NULL");
    cb_set_t temp = *set_dst;
    *set_dst      = *set_src;
    *set_src      = temp;
    return CB_OK;
}
cb_status_t cb_set_remove(cb_set_t *set, cb_strview_t key) {
    if (cb_set_empty(set)) return CB_ERR;
    int64_t hash = __cb_hash(key.data, key.count);
    size_t  idx  = 0;
    if (__cb_set_contains_internal(set, key, hash, &idx) == CB_ERR) return CB_ERR;
    for (; idx < (set->count - 1); idx++) set->items[idx] = set->items[idx + 1];
    set->count--;
    return CB_OK;
}
cb_status_t cb_set_insert(cb_set_t *set, cb_strview_t key) {
    CB_ASSERT(set && "`cb_set_t set` is NULL");
    if (key.count == 0 || key.data == NULL) return CB_ERR;
    return __cb_set_insert_internal(set, (cb_set_item_t){.hash = __cb_hash(key.data, key.count), .item = key});
}
cb_status_t cb_set_contains(cb_set_t *set, cb_strview_t key) {
    CB_ASSERT(set && "`cb_set_t set` is NULL");
    if (key.count == 0 || key.data == NULL) return CB_ERR;
    return __cb_set_contains_internal(set, key, __cb_hash(key.data, key.count), NULL);
}
cb_status_t cb_set_insert_many_impl(cb_set_t *set, ...) {
    cb_status_t status = CB_OK;
    va_list     args;
    va_start(args, set);
    // const char *arg;
    char *arg;
    while ((arg = va_arg(args, char *)) != NULL) status &= cb_set_insert_cstr(set, arg);
    va_end(args);
    return status;
}

#    define CB_STRCMP_LIT(s1, litcstr) strncmp(s1, litcstr, sizeof(litcstr) - 1)
static inline bool cb_config_parse_subcommand(const char *subcommand) {
    if (subcommand == NULL) return false;
    if (CB_STRCMP_LIT(subcommand, "build") == 0) g_subcmd = CB_SUBCMD_BUILD;
    else if (CB_STRCMP_LIT(subcommand, "config") == 0) g_subcmd = CB_SUBCMD_CONFIG;
    else if (CB_STRCMP_LIT(subcommand, "tests") == 0) g_subcmd = CB_SUBCMD_TESTS;
    else if (CB_STRCMP_LIT(subcommand, "clean") == 0) g_subcmd = CB_SUBCMD_CLEAN;
    else if (CB_STRCMP_LIT(subcommand, "install") == 0) g_subcmd = CB_SUBCMD_INSTALL;
    else CB_BAIL_ERROR(return false, "[Config] - Unknown Subcommand: %s", subcommand);
    return true;
}

int parse_enum_from_display(const char *str, const char **displays, size_t sizedisplay) {
    size_t str_len = strlen(str);
    char  *tempstr = cb_temp_alloc(str_len + 1);
    strncpy(tempstr, str, str_len);
    for (size_t i = 0; i < str_len; i++) tempstr[i] = toupper(tempstr[i]);

    for (size_t i = 0; i < sizedisplay; i++) {
        if (strncmp(tempstr, displays[i], str_len) == 0) return i;
    }
    cb_temp_reset_last();
    return 0;
}

static inline cb_status_t cb_config_save(cb_config_t *cfg, cb_path_t path) {
    FILE       *fp     = NULL;
    cb_status_t result = CB_OK;
    const char *p      = cb_path_to_cstr(&path);
    fp                 = fopen(p, "w");
    if (fp == NULL) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to open file: '%s' - %s", p, strerror(errno));
    cb_bin_write_header(fp);
    errno = 0;
    fwrite(cfg, sizeof(cb_config_t), 1, fp);
    if (errno) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to write config file: '%s' - %s", p, strerror(errno));
defer:
    if (fp != NULL) fclose(fp);
    return result;
}

static inline cb_status_t cb_config_load(cb_config_t *cfg, cb_path_t path) {
    cb_status_t result = CB_OK;
    if (!cb_path_exists(&path)) return CB_ERR;
    const char *p  = cb_path_to_cstr(&path);
    FILE       *fp = fopen(p, "rb");
    if (fp == NULL) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to open file: '%s' - %s", p, strerror(errno));
    if (!cb_bin_read_header(fp)) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Header of bin config file: '%s' is not valid", p);
    errno = 0;
    fread(cfg, sizeof(cb_config_t), 1, fp);
    if (errno) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to read config file: '%s' - %s", p, strerror(errno));
defer:
    if (fp) fclose(fp);
    return result;
}

static inline void cb_print_help(void) {
    fprintf(stderr, "USAGE: %s <SUBCOMMAND> [OPTIONS]" CB_LINE_END, program_name);
    fprintf(stderr, CB_LINE_END "SUBCOMMAND: " CB_LINE_END);
    fprintf(stderr, "    build          build the project" CB_LINE_END);
    fprintf(stderr, "    config         configure with the options provided, will create config file in build dir" CB_LINE_END);
    fprintf(stderr, "    tests          tests the tests case for the project" CB_LINE_END);
    fprintf(stderr, "    clean          clean the build artifact" CB_LINE_END);
    fprintf(stderr, "    install        install project to cfg.install_prefix:`%*s`" CB_LINE_END, SVArg(g_cfg.install_prefix));
    fprintf(stderr, CB_LINE_END "OPTIONS: " CB_LINE_END);
    fprintf(stderr, "   -ct, --compiler_type    set compiler type   [clang, gnu]                      (defualt same as `cb.h` compiles to)" CB_LINE_END);
    fprintf(stderr, "   -cc, --compiler         set compiler        [path_to_compiler]                (default will search compiler type)" CB_LINE_END);
    fprintf(stderr, "   -b,  --build            set build type      [debug, release, relwithdebinfo]  (default to 'debug')" CB_LINE_END);
    fprintf(stderr, "   -p,  --program          set program type    [C, CPP]                          (default to 'C')" CB_LINE_END);
    fprintf(stderr, "   -t,  --target           set target OS type  [windows, macos, unix]            (default to current run OS)" CB_LINE_END);
    fprintf(stderr, "   -a,  --arch             set architecture    [X64, X86, ARM64, ARM32]          (default to current run arch)" CB_LINE_END);
    fprintf(stderr, "   -h,  --help             print this help text" CB_LINE_END);
    fprintf(stderr, "   -q,  --quite            set output to quite" CB_LINE_END);
    fprintf(stderr, "   -d,  --display          display config  and target, will not start process" CB_LINE_END);
    fprintf(stderr, "        --release          set build type to release" CB_LINE_END);
    fprintf(stderr, "        --debug            set build type to debug" CB_LINE_END);
}

static inline bool cb_config_parse_from_args(int *c, char **v) {
    bool result  = true;
    program_name = cb_shift_args(c, &v);
    if (!*c) {
        CB_ERROR("Missing arguments of <SUBCOMMAND>");
        cb_print_help();
        return false;
    }
    result &= cb_current_dir(&g_cfg.project_path, NULL);
    cb_path_copy(&g_cfg.build_path, g_cfg.project_path);
    result &= cb_path_append_cstr(&g_cfg.build_path, "build");
    cb_path_copy(&g_cfg.config_path, g_cfg.build_path);
    result &= cb_path_append_cstr(&g_cfg.config_path, "config.cb");
    cb_path_copy(&g_cfg.targets_path, g_cfg.build_path);
    result &= cb_path_append_cstr(&g_cfg.targets_path, "targets.cb");

    cb_config_t temp_cfg;
    if (cb_path_exists(&g_cfg.config_path)) {
        // if config file is exists and the subcommand is not 'config' load the config
        // else continue parse args
        result &= cb_config_load(&g_cfg, g_cfg.config_path);
        memcpy(&temp_cfg, &g_cfg, sizeof(cb_config_t));
    }

    char *args = cb_shift_args(c, &v);
    if (!cb_config_parse_subcommand(args)) return false;

#    define opts(s, l) ((strncmp(args, s, argslen) == 0) || (strncmp(args, l, argslen) == 0))
#    define opt(l)     (strncmp(args, l, argslen) == 0)
#    define opt_next_arg(s, l)                                             \
        const char *arg = cb_shift_args(c, &v);                            \
        if (arg == NULL) {                                                 \
            CB_ERROR("options '" s ", " l "' required second arguments."); \
            cb_print_help();                                               \
            return false;                                                  \
        }
    size_t argslen;
    args = cb_shift_args(c, &v);
    while (args != NULL) {
        argslen = strlen(args);
        // clang-format off
        if opt ("--release") g_cfg.build_type = CB_BUILD_TYPE_RELEASE;
        else if opt ("--debug") g_cfg.build_type = CB_BUILD_TYPE_DEBUG;
        else if opts ("-d", "--display") g_display_config = true;
        else if opts ("-h", "--help") {
            g_subcmd = CB_SUBCMD_NOOP;
            cb_print_help();
            return true;
        } else if opts ("-q", "--quite") g_log_level = CB_LOG_ERROR;
        else if opts ("-c", "--compiler") {
            opt_next_arg("-cc", "--compiler");
            g_cfg.compiler_path = cb_path(arg);
        } else if opts ("-ct", "--compiler_type") {
            opt_next_arg("-ct", "--compiler_type");
            g_cfg.compiler_type = parse_enum_from_display(arg, CB_COMPILER_DISPLAY, ARRLEN(CB_COMPILER_DISPLAY));
        } else if opts ("-cc", "--compiler") {
            opt_next_arg("-cc", "--compiler");
            g_cfg.compiler_path = cb_path(arg);
        } else if opts ("-", "--build") {
            opt_next_arg("-ct", "--compiler_type");
            g_cfg.build_type = parse_enum_from_display(arg, CB_BUILD_TYPE_DISPLAY, ARRLEN(CB_BUILD_TYPE_DISPLAY));
        } else if opts ("-p", "--program") {
            opt_next_arg("-p", "--program");
            g_cfg.program_type = parse_enum_from_display(arg, CB_PROGRAM_DISPLAY, ARRLEN(CB_PROGRAM_DISPLAY));
        } else if opts ("-t", "--target") {
            opt_next_arg("-t", "--target");
            g_cfg.platform = parse_enum_from_display(arg, CB_PLATFORM_DISPLAY, ARRLEN(CB_PLATFORM_DISPLAY));
        } else if opts ("-a", "--arch") {
            opt_next_arg("-a", "--arch");
            g_cfg.arch     = parse_enum_from_display(arg, CB_ARCH_DISPLAY, ARRLEN(CB_ARCH_DISPLAY));
        }
        // clang-format on
        args = cb_shift_args(c, &v);
    }
    result &= cb_find_compiler(&g_cfg.compiler_path);

    if (memcmp(&temp_cfg, &g_cfg, sizeof(cb_config_t)) != 0 && g_subcmd != CB_SUBCMD_CONFIG) {
        result &= cb_config_save(&g_cfg, g_cfg.config_path);
    }

    return result;
}

cb_target_t cb_target_create(cb_strview_t name, cb_target_type_t type) {
    cb_target_t tgt = {0};
    tgt.name        = name;
    tgt.type        = type;
    tgt.flags       = cb_set_create();
    tgt.includes    = cb_set_create();
    tgt.ldflags     = cb_set_create();
    tgt.sources     = (cb_target_sources_t){0};
    if (tgt.type == CB_TARGET_TYPE_SYSTEM_LIB) return tgt;
    tgt.output_dir = cb_path(g_cfg.build_artifact_path.data);
    cb_path_append(&tgt.output_dir, name);
    tgt.output = cb_path(tgt.output_dir.data);

    char *ext  = cb_config_get_ext(type);
    if (CB_TARGET_TYPE_STATIC_LIB == type || type == CB_TARGET_TYPE_DYNAMIC_LIB) {
        char *fname = cb_temp_sprintf("lib" SVFmt, SVArg(name));
        cb_path_append_cstr(&tgt.output, fname);
        cb_temp_reset_last();
        cb_path_with_extension(&tgt.output, ext);
    } else {
        cb_path_append(&tgt.output, name);
    }

    return tgt;
}

void cb_target_delete(cb_target_t *tg) {
    if (!cb_da_empty(&tg->flags)) cb_set_delete(&tg->flags);
    if (!cb_da_empty(&tg->includes)) cb_set_delete(&tg->includes);
    if (!cb_da_empty(&tg->ldflags)) cb_set_delete(&tg->includes);
    if (!cb_da_empty(&tg->sources)) cb_da_free(tg->sources);
}

static inline cb_status_t __cb_target_add_sources_impl(cb_target_t *tg, cb_path_t source) {
    cb_status_t status        = CB_OK;
    cb_source_t target_source = {0};
    status &= cb_path_to_absolute_path(&source);
    cb_path_copy(&target_source.output, tg->output_dir);
    status &= cb_path_append(&target_source.output, cb_path_filename(&source));
    status &= cb_path_with_extension(&target_source.output, "o");
    target_source.source = source;
    cb_da_append(&tg->sources, target_source);
    return status;
}
static inline cb_status_t __cb_target_add_includes_impl(cb_target_t *tg, const char *include) {
    return cb_set_insert_cstr(&tg->includes, cb_temp_sprintf("-I%s", include));
}
static inline cb_status_t __cb_target_add_defines_impl(cb_target_t *tg, const char *def) {
    return cb_set_insert_cstr(&tg->flags, cb_temp_sprintf("-D%s", def));
}
static inline cb_status_t __cb_target_link_library_impl(cb_target_t *tg, cb_target_t *lib) {
    cb_status_t status = CB_OK;
    switch (lib->type) {
        case CB_TARGET_TYPE_SYSTEM_LIB: status &= cb_set_copy(&tg->ldflags, &lib->ldflags); break;
        case CB_TARGET_TYPE_STATIC_LIB: status &= cb_target_add_flags(tg, "-static");
        case CB_TARGET_TYPE_DYNAMIC_LIB: {
            size_t checkpoint = cb_temp_save();
            status &= cb_set_copy(&tg->ldflags, &lib->ldflags);
            status &= cb_set_copy(&tg->flags, &lib->flags);
            status &= cb_set_copy(&tg->includes, &lib->includes);
            char *flag_linkdir = cb_temp_sprintf("-L%s", cb_path_to_cstr(&lib->output));
            char *flag_link    = cb_temp_sprintf("-l" SVFmt, SVArg(lib->name));
            status &= cb_target_add_flags(tg, flag_linkdir, flag_link);
            cb_temp_rewind(checkpoint);
        } break;

        default: CB_BAIL_ERROR(exit(1), "cb_target_link_library does not accept lib->type thats not equal to library type"); break;
    }

    return status;
}

typedef struct {
    char         *ext;
    cb_target_t **tgt;
} __cb_target_add_sources_with_ext_t;
static inline bool __cb_target_add_sources_with_ext(cb_file_type_t ftype, cb_path_t *path, void *data) {
    CB_ASSERT(data);
    CB_ASSERT(path);
    __cb_target_add_sources_with_ext_t *data_struct = data;
    if (ftype != CB_FILE_DIRECTORY) {
        if (cb_sv_eq(cb_path_extension(path), cb_sv(data_struct->ext))) {
            CB_INFO("Found file path: %*s", (int)path->count, path->data);
            __cb_target_add_sources_impl(*data_struct->tgt, *path);
        }
    }
    return true;
}

#    define __CB_VA_LISTS_IMPL(tg, type, ...)                   \
        cb_status_t status = CB_OK;                             \
        va_list     args;                                       \
        va_start(args, tg);                                     \
        type arg;                                               \
        while ((arg = va_arg(args, type)) != NULL) __VA_ARGS__; \
        va_end(args);                                           \
        return status;

cb_status_t cb_target_add_sources_with_ext(cb_target_t *tg, const char *dir, char *ext, bool recursive) {
    cb_status_t                        result = CB_OK;
    __cb_target_add_sources_with_ext_t data   = {.ext = ext, .tgt = &tg};
    result &= cb_walkdir(dir, recursive, __cb_target_add_sources_with_ext, &data);
    return result;
}
cb_status_t cb_target_add_sources(cb_target_t *tg, ...) { __CB_VA_LISTS_IMPL(tg, const char *, __cb_target_add_sources_impl(tg, cb_path(arg))); }
cb_status_t cb_target_add_flags(cb_target_t *tg, ...) { __CB_VA_LISTS_IMPL(tg, const char *, cb_set_insert_cstr(&tg->flags, arg)); }
cb_status_t cb_target_add_ldflags(cb_target_t *tg, ...) { __CB_VA_LISTS_IMPL(tg, const char *, cb_set_insert_cstr(&tg->ldflags, arg)); }
cb_status_t cb_target_add_includes(cb_target_t *tg, ...) { __CB_VA_LISTS_IMPL(tg, const char *, __cb_target_add_includes_impl(tg, arg)); }
cb_status_t cb_target_add_defines(cb_target_t *tg, ...) { __CB_VA_LISTS_IMPL(tg, const char *, __cb_target_add_defines_impl(tg, arg)); }
cb_status_t cb_target_link_library(cb_target_t *tg, ...) { __CB_VA_LISTS_IMPL(tg, cb_target_t *, __cb_target_link_library_impl(tg, arg)); }
cb_status_t cb_target_as_cmd(cb_target_t *tg, cb_cmd_t *cmd) {
    cb_status_t status = CB_OK;
    if (tg->type == CB_TARGET_TYPE_SYSTEM_LIB) return status;

    cb_da_append(cmd, cb_path_to_cstr(&g_cfg.compiler_path));

    cb_da_foreach(&tg->flags, cb_set_item_t, cb_da_append(cmd, cb_sv_to_cstr(item->item)));
    cb_da_foreach(&tg->includes, cb_set_item_t, cb_da_append(cmd, cb_sv_to_cstr(item->item)));

    cb_cmd_append(cmd, "-o", cb_path_to_cstr(&tg->output));
    cb_da_foreach(&tg->sources, cb_source_t, cb_da_append(cmd, cb_path_to_cstr(&item->output)));

    cb_da_foreach(&tg->ldflags, cb_set_item_t, cb_da_append(cmd, cb_sv_to_cstr(item->item)));
    return status;
}
bool cb_target_need_rebuild(cb_target_t *tg) {
    char **sources = (char **)CB_REALLOC(NULL, tg->sources.count * sizeof(char *));
    CB_ASSERT(sources != NULL && "Extend the size of the temporary allocator");
    for (size_t i = 0; i < tg->sources.count; i++) sources[i] = cb_path_to_cstr(&tg->sources.items[i].output);
    int ret = cb_needs_rebuild(tg->output.data, (const char **)sources, tg->sources.count);
    CB_FREE(sources);
    return ret > 0;
}
bool cb_target_run(cb_target_t *tg) {
    if (!cb_target_need_rebuild(tg)) return true;
    bool        result      = true;
    size_t      rewind_temp = cb_temp_save();
    const char *compiler    = cb_path_to_cstr(&g_cfg.compiler_path);
    cb_cmd_t    cmd         = {0};
    cb_procs_t  procs       = {0};

    cb_da_append(&cmd, compiler);
    for (size_t s = 0; s < tg->includes.count; s++) cb_da_append(&cmd, cb_sv_to_cstr(tg->includes.items[s].item));
    for (size_t s = 0; s < tg->flags.count; s++) cb_da_append(&cmd, cb_sv_to_cstr(tg->flags.items[s].item));
    size_t save_idx = cmd.count;

    for (size_t idx = 0; idx < tg->sources.count; idx++) {
        cb_cmd_append(&cmd, "-o", cb_path_to_cstr(&tg->sources.items[idx].output), "-c", cb_path_to_cstr(&tg->sources.items[idx].source));

        cb_proc_t p = cb_cmd_run_async(cmd);
        if (p == CB_INVALID_PROC) {
            CB_ERROR("cb_cmd_run_async returned invalid proc");
        } else {
            cb_da_append(&procs, p);
        }

        cmd.count = save_idx;
        cb_temp_rewind(rewind_temp);
    }

    if (tg->sources.count != procs.count) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "procs count is not equal to sources count");
    if (!cb_procs_wait(procs)) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "failed to wait process for procs");

    cmd.count = 0;
    if (cb_target_as_cmd(tg, &cmd) == CB_ERR) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "failed to target as cmd");
    if (cb_cmd_run_sync(cmd) == CB_ERR) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "failed to run sync cmd");

defer:
    cb_temp_rewind(rewind_temp);
    cb_cmd_free(cmd);
    cb_da_free(procs);
    return result;
}
// init cb_t returning pointer of the `cb_t`, if error, return `NULL`
// `cb_t` is created in head, it should `cb_deinit` after using it
cb_t *cb_init(int argc, char **argv) {
    cb_config_parse_from_args(&argc, argv);

    cb_t *cb          = (cb_t *)CB_REALLOC(NULL, sizeof(cb_t));
    cb->count         = 0;
    cb->items         = NULL;
    cb->capacity      = 0;
    cb->on_pre_build  = NULL;
    cb->on_post_build = NULL;
    return cb;
}

static inline cb_status_t cb_targets_save(cb_t *cb) {
    cb_path_t   path   = {0};
    FILE       *fp     = NULL;
    cb_status_t result = CB_OK;
    cb_path_copy(&path, g_cfg.build_path);
    cb_path_append_cstr(&path, "targets.cb");
    const char *p = cb_path_to_cstr(&path);
    fp            = fopen(p, "wb");
    if (fp == NULL) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to open file: '%s' - %s", p, strerror(errno));
    errno = 0;
    cb_bin_write_header(fp);
    cb_bin_write_prim(fp, cb->count);
    for (size_t t = 0; t < cb->count; t++) {
        cb_target_t *it = &cb->items[t];
        cb_bin_write_prim(fp, it->type);
        cb_bin_write_sv(fp, &it->name);
        cb_bin_write_path(fp, &it->output_dir);
        cb_bin_write_path(fp, &it->output);

        cb_bin_write_prim(fp, it->flags.count);
        cb_bin_write_set(fp, &it->flags);

        cb_bin_write_prim(fp, it->includes.count);
        cb_bin_write_set(fp, &it->includes);

        cb_bin_write_prim(fp, it->ldflags.count);
        cb_bin_write_set(fp, &it->ldflags);

        cb_bin_write_prim(fp, it->sources.count);
        fwrite(it->sources.items, sizeof(cb_source_t), it->sources.count, fp);
    }

    if (errno) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to save targets file: '%s' - %s", p, strerror(errno));
defer:
    if (fp != NULL) fclose(fp);
    return result;
}
static inline cb_status_t cb_targets_load(cb_t *cb) {
    if (!cb_path_exists(&g_cfg.targets_path))
        CB_BAIL_ERROR(return CB_ERR, "no targets available, use command 'config' as subcommand to initiate project configuration");

    cb_path_t   path   = {0};
    FILE       *fp     = NULL;
    cb_status_t result = CB_OK;
    cb_path_copy(&path, g_cfg.build_path);
    cb_path_append_cstr(&path, "targets.cb");
    const char *p = cb_path_to_cstr(&path);
    fp            = fopen(p, "rb");
    if (fp == NULL) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to open file: '%s' - %s", p, strerror(errno));
    if (!cb_bin_read_header(fp)) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Header of bin config file: '%s' is not valid", p);

    errno = 0;
    cb_bin_read_prim(fp, cb->count);
    cb->items = (cb_target_t *)CB_REALLOC(NULL, cb->count * sizeof(cb_target_t));
    CB_ASSERT_ALLOC(cb->items);
    for (size_t t = 0; t < cb->count; t++) {
        cb_target_t *it = &cb->items[t];
        cb_bin_read_prim(fp, it->type);
        cb_bin_read_sv(fp, &it->name);
        cb_bin_read_path(fp, &it->output_dir);
        cb_bin_read_path(fp, &it->output);

        cb_bin_read_prim(fp, it->flags.count);
        cb_bin_read_set(fp, &it->flags);

        cb_bin_read_prim(fp, it->includes.count);
        cb_bin_read_set(fp, &it->includes);

        cb_bin_read_prim(fp, it->ldflags.count);
        cb_bin_read_set(fp, &it->ldflags);

        cb_bin_read_prim(fp, it->sources.count);
        it->sources.items = CB_REALLOC(NULL, it->sources.count * sizeof(cb_source_t));
        CB_ASSERT_ALLOC(it->sources.items);
        fread(it->sources.items, sizeof(cb_source_t), it->sources.count, fp);
    }

    if (errno) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "Failed to load targets file: '%s' - %s", p, strerror(errno));
defer:
    if (fp) fclose(fp);
    return result;
}

static inline void cb_targets_display(cb_t *cb) {
    printf(CB_LINE_END);
    printf("cb.targets.count    = %zu" CB_LINE_END, cb->count);
    for (size_t i = 0; i < cb->count; i++) {
        cb_target_t *t = &cb->items[i];
        printf("cb.targets[%zu].type       = %s" CB_LINE_END, i, CB_DISPLAY(CB_TARGET_TYPE, t->type));
        printf("cb.targets[%zu].name       = %*s" CB_LINE_END, i, SVArg(t->name));
        if (t->type != CB_TARGET_TYPE_SYSTEM_LIB) {
            printf("cb.targets[%zu].output_dir = '%*s'" CB_LINE_END, i, SVArg(t->output_dir));
            printf("cb.targets[%zu].output     = '%*s'" CB_LINE_END, i, SVArg(t->output));
        }

        printf("cb.targets[%zu].flags      = [", i);
        for (size_t fl = 0; fl < t->flags.count; fl++) {
            printf("%*s", SVArg(t->flags.items[fl].item));
            if (fl != t->flags.count - 1) printf(", ");
        }
        printf("]" CB_LINE_END);
        printf("cb.targets[%zu].ldflags    = [", i);
        for (size_t fl = 0; fl < t->ldflags.count; fl++) {
            printf("%*s", SVArg(t->ldflags.items[fl].item));
            if (fl != t->ldflags.count - 1) printf(", ");
        }
        printf("]" CB_LINE_END);
        printf("cb.targets[%zu].includes   = [", i);
        for (size_t fl = 0; fl < t->includes.count; fl++) {
            printf("%*s", SVArg(t->includes.items[fl].item));
            if (fl != t->includes.count - 1) printf(", ");
        }
        printf("]" CB_LINE_END);
        if (t->type != CB_TARGET_TYPE_SYSTEM_LIB) {
            printf("cb.targets[%zu].sources    = [", i);
            for (size_t fl = 0; fl < t->sources.count; fl++) {
                printf("(%*s : %*s)", SVArg(t->sources.items[fl].source), SVArg(t->sources.items[fl].output));
                if (fl != t->sources.count - 1) printf(", ");
            }
            printf("]" CB_LINE_END);
        }
        printf(CB_LINE_END);
    }
    printf(CB_LINE_END);
}
static inline cb_status_t __cb_do_build_target(cb_t *cb, cb_target_type_t type) {
    cb_status_t result = CB_OK;
    for (size_t i = 0; i < cb->count; i++) {
        cb_target_t *it = &cb->items[i];
        cb_mkdir_if_not_exists(cb_path_to_cstr(&it->output_dir));
        if (CB_TARGET_TYPE_DYNAMIC_LIB == it->type || it->type == CB_TARGET_TYPE_DYNAMIC_LIB) {
            if ((result &= cb_target_run(it)) == CB_ERR) CB_BAIL_ERROR(return result, "failed to run target: '%*s'", SVArg(it->name));
        }
    }
    for (size_t i = 0; i < cb->count; i++) {
        cb_target_t *it = &cb->items[i];
        if (it->type == type) {
            if ((result &= cb_target_run(it)) == CB_ERR) CB_BAIL_ERROR(return result, "failed to run target: '%*s'", SVArg(it->name));
        }
    }
    return result;
}

static inline cb_status_t __cb_on_build_target(cb_t *cb, cb_target_type_t type) {
    cb_status_t result = CB_OK;
    if (g_display_config) {
        cb_config_display();
        cb_targets_display(cb);
    } else {
        if (cb->on_pre_build) result &= cb->on_pre_build(cb, &g_cfg);
        result &= __cb_do_build_target(cb, type);
        if (cb->on_post_build) result &= cb->on_post_build(cb, &g_cfg);
        if (!result) CB_BAIL_ERROR(return result, "Failed building projects");
    }
    return result;
}
static inline cb_status_t __cb_on_config_target(cb_t *cb) {
    cb_status_t result = CB_OK;
    result &= cb_home_dir(&g_cfg.install_prefix, ".local");
    cb_path_copy(&g_cfg.build_artifact_path, g_cfg.build_path);
    result &= cb_path_append_cstr(&g_cfg.build_artifact_path, ((is_release()) ? "release" : "debug"));

    result &= on_configure(cb, &g_cfg);
    if (result == CB_ERR) CB_BAIL_ERROR(return result, "Failed to configuring project, return got CB_ERR");
    CB_INFO("(CB) - Saving Configuration to '%*s'", SVArg(g_cfg.config_path));
    result &= cb_config_save(&g_cfg, g_cfg.config_path);
    if (result == CB_ERR) CB_BAIL_ERROR(return result, "Failed to save config");
    CB_INFO("(CB) - Saving Targets Information to '%*s'", SVArg(g_cfg.targets_path));
    result &= cb_targets_save(cb);
    if (result == CB_ERR) CB_BAIL_ERROR(return result, "Failed to save targets");
    if (g_display_config) {
        cb_config_display();
        cb_targets_display(cb);
    }
    return result;
}

cb_status_t cb_run(cb_t *cb) {
    CB_ASSERT(cb != NULL);
    cb_status_t result = CB_OK;

    result &= cb_mkdir_if_not_exists(cb_path_to_cstr(&g_cfg.build_path));
    result &= cb_mkdir_if_not_exists(cb_path_to_cstr(&g_cfg.build_artifact_path));

    switch (g_subcmd) {
        case CB_SUBCMD_BUILD: {
            if ((result &= cb_targets_load(cb)) == CB_ERR) return CB_ERR;
            CB_INFO("(CB) - Running Build");
            result &= __cb_on_build_target(cb, CB_TARGET_TYPE_EXEC);
            CB_INFO("(CB) - Finish Build");
        } break;

        case CB_SUBCMD_CONFIG: {
            CB_INFO("(CB) - Running Configure");
            result &= __cb_on_config_target(cb);
            CB_INFO("(CB) - Success Configuring Project, run `build` or `tests` to build and run tests");
        } break;

        case CB_SUBCMD_TESTS: {
            if ((result &= cb_targets_load(cb)) == CB_ERR) return CB_ERR;
            CB_INFO("(CB) - Running Tests");
            result &= __cb_on_build_target(cb, CB_TARGET_TYPE_TESTS);
            CB_INFO("(CB) - Success on Running Tests");
        } break;

        case CB_SUBCMD_CLEAN: {
            result &= cb_remove_dir_if_exists(cb_path_to_cstr(&g_cfg.build_artifact_path));
            if (result == CB_ERR) CB_BAIL_ERROR(return result, "Failed to remove directory");
        } break;

        case CB_SUBCMD_INSTALL: {
            CB_INFO("(CB) - Running Install");
            g_cfg.build_type = CB_BUILD_TYPE_RELEASE;
            if ((result &= __cb_on_config_target(cb)) == CB_ERR) return CB_ERR;
            if ((result &= __cb_on_build_target(cb, CB_TARGET_TYPE_EXEC)) == CB_ERR) return CB_ERR;
            cb_mkdir_if_not_exists(cb_path_to_cstr(&g_cfg.install_prefix));
            cb_path_t lib_path = {0};
            cb_path_copy(&lib_path, g_cfg.install_prefix);
            cb_path_append_cstr(&lib_path, "lib");
            size_t    lib_path_len = lib_path.count;

            cb_path_t bin_path     = {0};
            cb_path_copy(&bin_path, g_cfg.install_prefix);
            cb_path_append_cstr(&bin_path, "bin");
            size_t bin_path_len = bin_path.count;

            for (size_t i = 0; i < cb->count; i++) {
                cb_target_t *tg = &cb->items[i];
                if (tg->type == CB_TARGET_TYPE_EXEC) {
                    result &= cb_mkdir_if_not_exists(cb_path_to_cstr(&bin_path));
                    if ((result &= cb_path_append(&bin_path, tg->name)) == CB_ERR) return CB_ERR;
                    if (!cb_copy_file(cb_path_to_cstr(&bin_path), cb_path_to_cstr(&tg->output))) return CB_ERR;
                    bin_path.count = bin_path_len;
                } else if (CB_TARGET_TYPE_DYNAMIC_LIB == tg->type || tg->type == CB_TARGET_TYPE_STATIC_LIB) {
                    result &= cb_mkdir_if_not_exists(cb_path_to_cstr(&lib_path));
                    if ((result &= cb_path_append(&lib_path, tg->name)) == CB_ERR) return CB_ERR;
                    if (!cb_copy_file(cb_path_to_cstr(&lib_path), cb_path_to_cstr(&tg->output))) return CB_ERR;
                    lib_path.count = lib_path_len;
                }
            }
            CB_INFO("(CB) - Success Running Install");
        } break;

        case CB_SUBCMD_NOOP:
        case CB_SUBCMD_MAX: break;
    }

    return result;
}
cb_status_t cb_dump_compile_commands(cb_t *cb) {
    (void)cb;
    return CB_ERR;
}

void cb_deinit(cb_t *cb) {
    if (cb != NULL) {
        if (!cb_da_empty(cb)) {
            for (size_t i = 0; i < cb->count; ++i) cb_target_delete(&cb->items[i]);
            cb_da_free(*cb);
        }
        CB_FREE(cb);
        cb = NULL;
    }
}

cb_target_t *cb_create_target_impl(cb_t *cb, cb_strview_t name, cb_target_type_t type) {
    CB_ASSERT(cb != NULL);
    cb_da_append(cb, cb_target_create(name, type));
    return cb_da_last(cb);
}

void         cb_add_on_pre_build_callback(cb_t *cb, cb_callback_fn callback) { cb->on_pre_build = callback; }
void         cb_add_on_post_build_callback(cb_t *cb, cb_callback_fn callback) { cb->on_post_build = callback; }
cb_target_t *cb_create_target_pkgconf(cb_t *cb, cb_strview_t name) {
#    ifdef CB_WINDOWS
    CB_ERROR("cb_create_target_pkgconf is not supported in windows");
    return NULL;
#    else
    const char      *cmd           = NULL;
    cb_target_t      tgt           = cb_target_create(name, CB_TARGET_TYPE_SYSTEM_LIB);
    cb_str_builder_t contents_buff = {0};

    /// --libs
    cmd = cb_temp_sprintf("/usr/bin/pkg-config --cflags --libs %*s", SVArg(name));
    if (cb_popen_stdout(cmd, &contents_buff) != CB_OK) CB_BAIL_ERROR(return NULL, "Failed to procecss open cmd: '%s'", cmd);
    cb_temp_reset_last();

    char *token = NULL;
    token       = strtok(contents_buff.items, " \n\t");
    while (token != NULL) {
        if (!(token[0] == '\n' || token[0] == '\t' || token[0] == ' ')) {
            cb_set_insert_cstr(&tgt.ldflags, cb_temp_strdup(token));
            CB_INFO("token: %s", token);
        }
        token = strtok(NULL, " ");
    }

    cb_sb_free(contents_buff);
    cb_da_append(cb, tgt);
    return cb_da_last(cb);
#    endif  // CB_WINDOWS
}

#    ifdef CB_WINDOWS
DIR *opendir(const char *dirpath) {
    assert(dirpath);
    char buffer[MAX_PATH];
    snprintf(buffer, MAX_PATH, "%s\\*", dirpath);
    DIR *dir   = (DIR *)calloc(1, sizeof(DIR));
    dir->hFind = FindFirstFile(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }
    return dir;

fail:
    if (dir) CB_FREE(dir);
    return NULL;
}

struct dirent *readdir(DIR *dirp) {
    assert(dirp);
    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent *)calloc(1, sizeof(struct dirent));
    } else {
        if (!FindNextFile(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }
            return NULL;
        }
    }
    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));
    strncpy(dirp->dirent->d_name, dirp->data.cFileName, sizeof(dirp->dirent->d_name) - 1);
    return dirp->dirent;
}

int closedir(DIR *dirp) {
    assert(dirp);
    if (!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }
    if (dirp->dirent) CB_FREE(dirp->dirent);
    CB_FREE(dirp);
    return 0;
}
#    endif  // CB_WINDOWS
typedef struct {
    cb_strview_t exec;
    cb_path_t  **p;
} __which_exec_walkdir_t;
static inline bool __which_exec_walkdir(cb_file_type_t ftype, cb_path_t *rent, void *data) {
    (void)ftype;
    __which_exec_walkdir_t *which_data = (__which_exec_walkdir_t *)data;
    if (cb_sv_eq(which_data->exec, cb_path_filename(rent))) {
        CB_INFO("Found which path: %*s", SVArg(*rent));
        cb_path_move(*which_data->p, rent);
        return false;
    }
    return true;
}

static cb_status_t which_exec(const cb_strview_t exec, cb_path_t *out) {
    CB_ASSERT(out && "out param should not be NULL");
    CB_INFO("Searching full path of executable: `%*s`", SVArg(exec));
    cb_status_t result = CB_OK;
    char       *paths  = NULL;
    out->count         = 0;

    paths              = getenv("PATH");
    if (paths == NULL) CB_BAIL_ERROR(cb_return_defer(CB_ERR), "PATH environment variable not set.\n");
    __which_exec_walkdir_t data = {exec, &out};
    char                  *token;
    while ((token = strsep(&paths, ":")) != NULL) {
        result &= cb_walkdir(token, false, __which_exec_walkdir, &data);
        if (out->count) cb_return_defer(CB_OK);
    }

defer:
    if (result == CB_ERR) CB_ERROR("Failed to get full path for executable: `%*s`", SVArg(exec));
    return result;
}

#    define CB_WHICH_COMPILER(P, CC, CXX)                                                   \
        do {                                                                                \
            switch (g_cfg.program_type) {                                                   \
                case CB_PROGRAM_C: result &= which_exec(cb_sv(CC), P); break;               \
                case CB_PROGRAM_CPP: result &= which_exec(cb_sv(CXX), P); break;            \
                default: CB_ERROR("Program is Unknown, should be c or c++"); return CB_ERR; \
            }                                                                               \
        } while (0)
cb_status_t cb_find_compiler(cb_path_t *compiler_path) {
    CB_ASSERT(compiler_path);
    if (compiler_path->count != 0) return CB_OK;

    cb_status_t result = CB_OK;
    switch (g_cfg.compiler_type) {
        case CB_COMPILER_CLANG: CB_WHICH_COMPILER(compiler_path, "clang", "clang++"); break;
        case CB_COMPILER_GNU: CB_WHICH_COMPILER(compiler_path, "gcc", "g++"); break;
        default: break;
    }
    if (result == CB_ERR) {
        CB_INFO("Failed to get program path for: compiler `%s`", CB_COMPILER_DISPLAY[g_cfg.compiler_type]);
        CB_INFO("Trying to get compiler path from environment variable `PATH`=`CC`");
        char *compiler = getenv("CC");
        if (compiler == NULL) {
            CB_INFO("There is no compiler path from environment variable `PATH`=`CC`");
            return CB_ERR;
        }
        *compiler_path = cb_path(compiler);
    }

    return CB_OK;
}

////////////////////////////////////////////////////////////////////////////////
#endif  // CB_IMPLEMENTATION
