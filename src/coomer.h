#ifndef __COOMER_H__
#define __COOMER_H__

#include <GL/gl.h>
#include <GL/glx.h>
#include <GLES3/gl3.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>

#include "util.h"

#define ASSERT_EXIT(EXPR, EC, ...) \
    if (!(EXPR)) {                 \
        coom_error(__VA_ARGS__);   \
        exit(EC);                  \
    }

#define ASSERT_NULL_ERROR_DEFER(ASRT, RET, ...) \
    if (ASRT == NULL) {                         \
        coom_error(__VA_ARGS__);                \
        return_defer(RET);                      \
    }

#define OPTIONS_ARGS_DEFAULT ((options_args){.windowed = false, .delay_second = 0, .new_config = NULL, .config = NULL})
typedef struct {
    bool        windowed;
    bool        select;
    f32         delay_second;
    const char *new_config;
    const char *config;
} options_args;

bool parse_args(int *argc, char ***argv, options_args *optargs);

///////////////////////////////////////////////////////////////////////
/// CONFIG
///////////////////////////////////////////////////////////////////////
typedef struct {
    float min_scale;
    float scroll_speed;
    float drag_friction;
    float scale_friction;
} coom_config_t;
float parse_float(const char *str, float dflt);
// return null on error
coom_config_t *coom_load_config(const char *file_path);
void           coom_unload_config(coom_config_t *cfg);
bool           coom_generate_default_config(const char *file_path);

const char    *get_config_path(const char *dir, const char *file);

///////////////////////////////////////////////////////////////////////
/// coomer objects
///////////////////////////////////////////////////////////////////////
#define VELOCITY_THRESHOLD 10.0
typedef struct {
    bool enabled;
    f32  shadow;
    f32  radius;
    f32  delta_radius;
} coom_flashlight;

typedef struct {
    vec2_t curr;
    vec2_t prev;
    bool   drag;
} coom_mouse;

typedef struct {
    vec2_t position;
    vec2_t velocity;
    vec2_t scale_pivot;
    f32    scale;
    f32    deltascale;
} coom_camera;

typedef struct {
    bool            quit;
    bool            windowed;
    short           rate;
    float           dt;

    GLuint          prog, vao, vbo, ebo;
    Atom            delete_msg;
    Window          win;
    vec2_t          winsize;
    coom_camera     cam;
    coom_mouse      mouse;
    coom_flashlight fl;
    coom_config_t  *cfg;
    Display        *dpy;
    XImage         *img;
} coom_t;

coom_t coom_init_coom(options_args args);
void   coom_uninit_coom(coom_t *c);
void   coom_begin(coom_t *c);
void   coom_end(coom_t *c);
void   coom_draw(coom_t *c);

///////////////////////////////////////////////////////////////////////
/// X11
///////////////////////////////////////////////////////////////////////
#define WM_NAME  "coomer"
#define WM_CLASS "Coomer"

Display *coom_open_display(void);
Window   coom_select_window(Display *d);
short    coom_get_monitor_rate(Display *d);
GLuint   coom_initialize_shader(GLuint *vao, GLuint *vbo, GLuint *ebo, XImage *img);
void     coom_uninitialize_shader(GLuint *vao, GLuint *vbo, GLuint *ebo, GLuint *program);

vec2_t   coom_mouse_pos(Display *dpy);

XImage  *coom_new_screenshot(Display *dpy, Window win);
void     coom_delete_screenshot(XImage *img);
bool     coom_save_to_ppm(XImage *img, const char *file_path);
#endif  // __COOMER_H__
