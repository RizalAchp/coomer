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
#define WM_NAME  "coomer"
#define WM_CLASS "Coomer"

int    coom_xerror_handler(Display *d, XErrorEvent *e);
Window coom_select_window(Display *d);

GLuint coom_initialize_shader(GLuint *vao, GLuint *vbo, GLuint *ebo, XImage *img);
void   coom_uninitialize_shader(GLuint *vao, GLuint *vbo, GLuint *ebo, GLuint *program);

#define VELOCITY_THRESHOLD 15.0
///////////////////////////////////////////////////////////////////////
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
    f64    deltascale;
} coom_camera;

XImage *coom_new_screenshot(Display *dpy, Window win);
void    coom_delete_screenshot(XImage *img);
void    coom_image_draw(const XImage *img, const coom_camera *cam, const vec2_t winsize, const coom_mouse *mouse, const coom_flashlight *fl, GLuint shader,
                        GLuint vao);
bool    coom_save_to_ppm(XImage *img, const char *file_path);

void    coom_flashlight_update(coom_flashlight *fl, f32 dt);

vec2_t  coom_mouse_pos(Display *dpy);

vec2_t  coom_camera_world(coom_camera *camera, vec2_t v);
void    coom_camera_update(coom_camera *camera, const coom_config_t *cfg, const coom_mouse *mouse, vec2_t winsize, f32 dt);

#define coom_camera_reset(cam)             \
    do {                                   \
        (cam).scale      = 1.0;            \
        (cam).deltascale = 0.0;            \
        (cam).position   = vec2(0.0, 0.0); \
        (cam).velocity   = vec2(0.0, 0.0); \
    } while (0)
#define coom_camera_scrollup(cam)                              \
    do {                                                       \
        if ((ev.xkey.state & ControlMask) > 0 && fl.enabled) { \
            fl.delta_radius = 250;                             \
        } else {                                               \
            (cam).deltascale += config->scroll_speed;          \
            (cam).scale_pivot = mouse.curr;                    \
        }                                                      \
    } while (0)
#define coom_camera_scrolldown(cam)                            \
    do {                                                       \
        if ((ev.xkey.state & ControlMask) > 0 && fl.enabled) { \
            fl.delta_radius = 250;                             \
        } else {                                               \
            (cam).deltascale -= config->scroll_speed;          \
            (cam).scale_pivot = mouse.curr;                    \
        }                                                      \
    } while (0)

#endif  // __COOMER_H__
