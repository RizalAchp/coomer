
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "arg.h"
#include "coomer.h"
#include "util.h"

static void print_help(const char *program_name) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", program_name);
    fprintf(stderr, "   -d, --delay <seconds: float>  delay execution of the program by provided <seconds>\n");
    fprintf(stderr, "   -h, --help                    show this help and exit\n");
    fprintf(stderr, "       --new-config [filepath]   generate a new default config at [filepath]\n");
    fprintf(stderr, "   -c, --config <filepath>       use config at <filepath>\n");
    fprintf(stderr, "   -V, --version                 show the current version and exit\n");
    fprintf(stderr, "   -w, --windowed                windowed mode instead of fullscreen\n");
    fprintf(stderr, "   -s, --select                  select window mode default root window\n");
    fprintf(stderr, "       --verbose                 make the output more verbose\n");
}

static int parse_args(int *argc, char ***argv, options_args *optargs) {
    const char *program_name = shift_args(argc, argv);
    if (argc == 0) {
        print_help(program_name);
        return 0;
    }
    const char *opt;
    // clang-format off
    while ((opt = shift_args(argc, argv)) != NULL) {
        options_cmp_arg(opt, "-d",  "--delay",      { optargs->delay_second      = parse_float(arg, 0); });
        options_cmp_arg(opt, "-c",  "--config",     { optargs->config = arg; });
        options_cmp_arg(opt, "",    "--new-config", { optargs->new_config = arg; });
        options_cmp(opt, "-w", "--windowed", { optargs->windowed = true; });
        options_cmp(opt, "-s", "--select", { optargs->select = true; });
        options_cmp(opt, "-h", "--help", {
            print_help(program_name);
            exit(0);
        });
        options_cmp(opt, "-v", "--version", {
            fprintf(stderr, "coomer " COOMER_VERSION "\n");
            exit(0);
        });
        options_cmp(opt, "", "--verbose", { verbose = true; });
    }
    // clang-format on
    return 0;
}

static inline void mssleep(u32 ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[]) {
    int                     result      = 0;
    coom_config_t          *config      = NULL;
    const char             *config_file = NULL;
    Display                *dpy         = NULL;
    XRRScreenConfiguration *screen_cfg  = NULL;
    XImage                 *ss          = NULL;

    options_args            args        = OPTIONS_ARGS_DEFAULT;
    if (parse_args(&argc, &argv, &args) != 0) return_defer(1);

    if (args.new_config != NULL) return !coom_generate_default_config(args.new_config);
    if (args.config != NULL) config_file = args.config;
    else config_file = get_config_path("coomer", "config.cfg");
    if (config_file == NULL) {
        coom_error("failed get confg file path open coomer_test.cfg");
        return_defer(1);
    }
    if (args.delay_second) mssleep(args.delay_second * 1000);

    config = coom_load_config(config_file);
    if (config == NULL) {
        coom_error("failed to open coomer_test.cfg");
        return_defer(1);
    }

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) coom_error("Failed to open display");
    XSetErrorHandler(coom_xerror_handler);

    Window tracking_window = (args.select) ? coom_select_window(dpy) : DefaultRootWindow(dpy);
    screen_cfg             = XRRGetScreenInfo(dpy, DefaultRootWindow(dpy));
    short rate             = XRRConfigCurrentRate(screen_cfg);
    coom_info("screen rate: %d", rate);

    int screen = XDefaultScreen(dpy);
    int glxMajor, glxMinor;
    if (!glXQueryVersion(dpy, &glxMajor, &glxMinor) || (glxMajor == 1 && glxMinor < 3) || (glxMajor < 1)) {
        coom_error("Invalid GLX version. expected >=1.3");
        return_defer(1);
    }
    coom_info("GLX Version   = %d.%d", glxMajor, glxMinor);
    coom_info("GLX Extension = %s", glXQueryExtensionsString(dpy, screen));

    int attr[] = {
        GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None,
    };
    XVisualInfo *vi = glXChooseVisual(dpy, 0, &attr[0]);
    if (vi == NULL) {
        coom_error("Failed to choose appropriate visual - glXChooseVisual");
        return_defer(1);
    }
    coom_info("Visual %zu selected", vi->visualid);

    XSetWindowAttributes swa = {0};
    swa.colormap             = XCreateColormap(dpy, DefaultRootWindow(dpy), vi->visual, AllocNone);
    swa.event_mask           = (ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ExposureMask | ClientMessage);
    if (!args.windowed) {
        swa.override_redirect = 1;
        swa.save_under        = 1;
    }

    XWindowAttributes attributes = {0};
    XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &attributes);
    Window win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, attributes.width, attributes.height, 0, vi->depth, InputOutput, vi->visual,
                               CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder, &swa);
    XMapWindow(dpy, win);
    char      *wmName  = WM_NAME;
    char      *wmClass = WM_CLASS;
    XClassHint hint    = {wmName, wmClass};

    XStoreName(dpy, win, wmName);
    XSetClassHint(dpy, win, &hint);

    Atom wm_delete_msg = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(dpy, win, &wm_delete_msg, 1);

    GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);

    ss = coom_new_screenshot(dpy, tracking_window);
    if (ss == NULL) {
        coom_error("Failed to get screenshot, exiting");
        return_defer(1);
    }
    GLuint          vao, vbo, ebo;
    GLuint          shader_program = coom_initialize_shader(&vao, &vbo, &ebo, ss);

    bool            quitting       = false;
    coom_camera     cam            = {.scale = 1.0};
    vec2_t          pos            = coom_mouse_pos(dpy);
    coom_mouse      mouse          = {.curr = pos, .prev = pos};
    coom_flashlight fl             = {.enabled = false, .radius = 200.0};
    f32             dt             = 1.0 / rate;

    XEvent          ev;

    while (!quitting) {
        if (!args.windowed) XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
        XWindowAttributes wa;
        XGetWindowAttributes(dpy, win, &wa);
        glViewport(0, 0, wa.width, wa.height);

        while (XPending(dpy) > 0) {
            XNextEvent(dpy, &ev);
            if (XFilterEvent(&ev, None)) continue;
            switch (ev.type) {
                case Expose:
                case MotionNotify: {
                    mouse.curr = vec2(ev.xmotion.x, ev.xmotion.y);
                    if (mouse.drag) {
                        vec2_t prev  = coom_camera_world(&cam, mouse.prev);
                        vec2_t curr  = coom_camera_world(&cam, mouse.curr);
                        vec2_t delta = vec2_sub(prev, curr);
                        vec2_sub_assign(&cam.position, delta);
                        cam.velocity = vec2_mulf(delta, rate);
                    }
                    mouse.prev = mouse.curr;
                } break;
                case ClientMessage: quitting = ((Atom)ev.xclient.data.l[0]) == wm_delete_msg; break;
                case KeyPress: {
                    switch (XLookupKeysym((XKeyEvent *)&ev, 0)) {
                        case XK_equal: coom_camera_scrollup(cam); break;
                        case XK_minus: coom_camera_scrolldown(cam); break;
                        case XK_0: coom_camera_reset(cam); break;
                        case XK_q:
                        case XK_Escape: quitting = true; break;
                        case XK_f: fl.enabled = !fl.enabled; break;
                        default: break;
                    }
                } break;
                case ButtonPress: {
                    if (ev.xbutton.button == Button1) {
                        mouse.prev   = mouse.curr;
                        mouse.drag   = true;
                        cam.velocity = vec2(0.0, 0.0);
                    } else if (ev.xbutton.button == Button4) coom_camera_scrollup(cam);
                    else if (ev.xbutton.button == Button5) coom_camera_scrolldown(cam);
                } break;
                default: break;
            }
            vec2_t winsize = vec2(wa.width, wa.height);

            coom_camera_update(&cam, config, &mouse, winsize, dt);
            coom_flashlight_update(&fl, dt);
            coom_image_draw(ss, &cam, winsize, &mouse, &fl, shader_program, vao);

            glXSwapBuffers(dpy, win);
            glFlush();
            XSync(dpy, 1);
        }
    }

defer:
    coom_uninitialize_shader(&vao, &vbo, &ebo, &shader_program);
    coom_delete_screenshot(ss);
    if (screen_cfg) XRRFreeScreenConfigInfo(screen_cfg);
    if (dpy) XCloseDisplay(dpy);
    coom_unload_config(config);
    return result;
}
