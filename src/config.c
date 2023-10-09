#include <linux/limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "coomer.h"

const coom_config_t default_config = {
    .min_scale      = 0.01,
    .scroll_speed   = 1.5,
    .drag_friction  = 6.0,
    .scale_friction = 4.0,
};

float parse_float(const char *str, float dflt) {
    errno        = 0;
    char *end    = NULL;
    float result = strtof(str, &end);
    if (errno) {
        coom_error("Conversion failed: %s", strerror(errno));
        return_defer(dflt);
    }
    if (str == end) {
        coom_error("Conversion failed: No valid float found.");
        return_defer(dflt);
    } else if (*end != '\0') {
        coom_error("Conversion failed: Invalid characters after the number: %s", end);
        return_defer(dflt);
    }
defer:
    return result;
}

coom_config_t *coom_load_config(const char *file_path) {
    assert(file_path && "file_path should not be NULL");
    coom_config_t *result = (coom_config_t *)coom_alloc(NULL, sizeof(coom_config_t));
    ASSERT_EXIT(result != NULL, 1, "Failed to allocate memory");
    memcpy(result, &default_config, sizeof(coom_config_t));

    FILE *f = fopen(file_path, "rb");
    if (f == NULL) {
        coom_error("failed to fopen file - %s", strerror(errno));
        goto defer;
    }
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = temp_alloc(fsize + 1);
    fread(buf, fsize, 1, f);

    strview contents = SV_FROM(buf, fsize);
    strview line     = {0};
    size_t  idx      = 0;
    while (sv_chop_by_delim(&contents, '\n', &line)) {
        line = sv_trim(line);
        if (line.count == 0 || line.data[0] == '#') continue;
        if (!sv_index_of(line, '=', &idx)) continue;
        strview key   = sv_trim(sv_chop_left(&line, idx));
        strview value = sv_trim(SV_FROM(line.data + 1, line.count - 1));
        coom_info("got config [key = value] : ['" SV_FMT "' = '" SV_FMT "']", SV_ARG(key), SV_ARG(value));

        if (sv_eq(key, "min_scale")) result->min_scale = parse_float(sv_to_cstr(value), default_config.min_scale);
        else if (sv_eq(key, "scroll_speed")) result->scroll_speed = parse_float(sv_to_cstr(value), default_config.scroll_speed);
        else if (sv_eq(key, "drag_friction")) result->drag_friction = parse_float(sv_to_cstr(value), default_config.drag_friction);
        else if (sv_eq(key, "scale_friction")) result->scale_friction = parse_float(sv_to_cstr(value), default_config.scale_friction);
        else coom_error("Unknown config key: `" SV_FMT "`", SV_ARG(key));

        temp_free(value.count);
    }
defer:
    temp_reset();
    if (f) fclose(f);
    return result;
}
void coom_unload_config(coom_config_t *cfg) {
    if (cfg) coom_free(cfg);
}

bool coom_generate_default_config(const char *file_path) {
    bool  result = 0;
    FILE *f      = fopen(file_path, "wb");
    if (f == NULL) {
        coom_error("failed to fopen file - %s", strerror(errno));
        return_defer(NULL)
    }
    fprintf(f, "min_scale = %.4f\n", default_config.min_scale);
    fprintf(f, "scroll_speed = %.4f\n", default_config.scroll_speed);
    fprintf(f, "drag_friction = %.4f\n", default_config.drag_friction);
    fprintf(f, "scale_friction = %.4f\n", default_config.scale_friction);
defer:
    temp_reset();
    if (f) fclose(f);
    return result;
}

const char  CONFIG_DIR[] = "/.config/";

const char *get_config_path(const char *dir, const char *file) {
    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd == NULL) {
            coom_error("failed to get pwuid to get HOME dir - %s", strerror(errno));
            return NULL;
        }
        homedir = pwd->pw_dir;
    }
    char *path_buff = temp_alloc(PATH_MAX);
    memset(path_buff, 0, PATH_MAX);

    snprintf(path_buff, PATH_MAX, "%s/.config/%s", homedir, dir);
    int result = mkdir(path_buff, 0755);
    if (result < 0 && errno == EEXIST) {
        coom_info("directory `%s` already exists, ignoring..", path_buff);
    } else {
        coom_error("could not create directory `%s`: %s", path_buff, strerror(errno));
        return NULL;
    }
    strncat(path_buff, "/", 1);
    strncat(path_buff, file, strlen(file) + 1);

    coom_info("got config path: '%s'", path_buff);
    return path_buff;
}
