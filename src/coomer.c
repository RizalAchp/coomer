#include "coomer.h"

#include <assert.h>
#include <math.h>

#include "arg.h"

#define coom_camera_world(cam, v) vec2_divf(v, (cam).scale)

#define coom_camera_reset(c)              \
    (c)->cam.scale      = 1.0;            \
    (c)->cam.deltascale = 0.0;            \
    (c)->cam.position   = vec2(0.0, 0.0); \
    (c)->cam.velocity   = vec2(0.0, 0.0);

#define coom_camera_scrollup(c)                                 \
    if ((ev.xkey.state & ControlMask) > 0 && (c)->fl.enabled) { \
        (c)->fl.delta_radius -= 250;                            \
    } else {                                                    \
        (c)->cam.deltascale += (c)->cfg->scroll_speed;          \
        (c)->cam.scale_pivot = (c)->mouse.curr;                 \
    }

#define coom_camera_scrolldown(c)                               \
    if ((ev.xkey.state & ControlMask) > 0 && (c)->fl.enabled) { \
        (c)->fl.delta_radius += 250;                            \
    } else {                                                    \
        (c)->cam.deltascale -= (c)->cfg->scroll_speed;          \
        (c)->cam.scale_pivot = (c)->mouse.curr;                 \
    }

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

bool parse_args(int *argc, char ***argv, options_args *optargs) {
    const char *program_name = shift_args(argc, argv);
    if (argc == 0) {
        print_help(program_name);
        return true;
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
        
        coom_error("Unknown options: '%s', see -h,--help for more info", opt);
        return false;
    }
    // clang-format on
    return true;
}

static void coom_initialize_window(coom_t *c) {
    coom_info("%s", __PRETTY_FUNCTION__);
    coom_info("%s", __PRETTY_FUNCTION__);
    int screen = XDefaultScreen(c->dpy);
    int glxMajor, glxMinor;
    if (!glXQueryVersion(c->dpy, &glxMajor, &glxMinor) || (glxMajor == 1 && glxMinor < 3) || (glxMajor < 1)) {
        coom_error("Invalid GLX version. expected >=1.3");
        exit(1);
    }
    coom_info("GLX Version   = %d.%d", glxMajor, glxMinor);
    coom_info("GLX Extension = %s", glXQueryExtensionsString(c->dpy, screen));

    int          attr[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    XVisualInfo *vi     = glXChooseVisual(c->dpy, 0, &attr[0]);
    if (vi == NULL) {
        coom_error("Failed to choose appropriate visual - glXChooseVisual");
        exit(1);
    }
    coom_info("Visual %zu selected", vi->visualid);

    XSetWindowAttributes swa = {0};
    swa.colormap             = XCreateColormap(c->dpy, DefaultRootWindow(c->dpy), vi->visual, AllocNone);
    swa.event_mask           = (ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ExposureMask | ClientMessage);
    if (!c->windowed) {
        swa.override_redirect = 1;
        swa.save_under        = 1;
    }
    XWindowAttributes attributes = {0};
    XGetWindowAttributes(c->dpy, DefaultRootWindow(c->dpy), &attributes);
    c->win = XCreateWindow(c->dpy, DefaultRootWindow(c->dpy), 0, 0, attributes.width, attributes.height, 0, vi->depth, InputOutput, vi->visual,
                           CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder, &swa);
    XMapWindow(c->dpy, c->win);
    XClassHint hint = {WM_NAME, WM_CLASS};
    XStoreName(c->dpy, c->win, WM_NAME);
    XSetClassHint(c->dpy, c->win, &hint);

    c->delete_msg = XInternAtom(c->dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(c->dpy, c->win, &c->delete_msg, 1);

    GLXContext glc = glXCreateContext(c->dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(c->dpy, c->win, glc);
}

coom_t coom_init_coom(options_args args) {
    coom_info("%s", __PRETTY_FUNCTION__);
    coom_t      c = {.quit = false, .windowed = args.windowed};
    const char *cfgpath;
    if (args.config != NULL) cfgpath = args.config;
    else cfgpath = get_config_path("coomer", "config.cfg");

    c.cfg  = coom_load_config(cfgpath);
    c.dpy  = coom_open_display();
    c.img  = coom_new_screenshot(c.dpy, (args.select) ? coom_select_window(c.dpy) : DefaultRootWindow(c.dpy));
    c.rate = coom_get_monitor_rate(c.dpy);
    c.dt   = 1.0 / c.rate;

    coom_initialize_window(&c);

    c.prog     = coom_initialize_shader(&c.vao, &c.vbo, &c.ebo, c.img);

    c.cam      = (coom_camera){.scale = 1.0};
    vec2_t pos = coom_mouse_pos(c.dpy);
    c.mouse    = (coom_mouse){.curr = pos, .prev = pos};
    c.fl       = (coom_flashlight){.enabled = false, .radius = 200.0};

    return c;
}

void coom_uninit_coom(coom_t *c) {
    coom_info("%s", __PRETTY_FUNCTION__);
    coom_uninitialize_shader(&c->vao, &c->vbo, &c->ebo, &c->prog);
    coom_delete_screenshot(c->img);
    if (c->dpy) XCloseDisplay(c->dpy);
    coom_unload_config(c->cfg);
}

static void coom_process_events(coom_t *c, XEvent ev) {
    switch (ev.type) {
        case Expose: break;
        case MotionNotify: {
            c->mouse.curr = vec2(ev.xmotion.x, ev.xmotion.y);
            if (c->mouse.drag) {
                vec2_t prev  = coom_camera_world(c->cam, c->mouse.prev);
                vec2_t curr  = coom_camera_world(c->cam, c->mouse.curr);
                vec2_t delta = vec2_sub(prev, curr);
                vec2_sub_assign(&c->cam.position, delta);
                c->cam.velocity = vec2_mulf(delta, c->rate);
            }
            c->mouse.prev = c->mouse.curr;
        } break;
        case ClientMessage: c->quit = ((Atom)ev.xclient.data.l[0]) == c->delete_msg; break;
        case KeyPress:
            switch (XLookupKeysym((XKeyEvent *)&ev, 0)) {
                case XK_equal: coom_camera_scrollup(c); break;
                case XK_minus: coom_camera_scrolldown(c); break;
                case XK_0: coom_camera_reset(c); break;
                case XK_f: c->fl.enabled = !c->fl.enabled; break;
                case XK_q:
                case XK_Escape: c->quit = true; break;
                default: break;
            }
            break;
        case ButtonPress:
            switch (ev.xbutton.button) {
                case Button1:
                    c->mouse.prev   = c->mouse.curr;
                    c->mouse.drag   = true;
                    c->cam.velocity = vec2(0.0, 0.0);
                    break;
                case Button4: coom_camera_scrollup(c); break;
                case Button5: coom_camera_scrolldown(c); break;
                default: break;
            }
            break;
        default: break;
    }
}

void coom_begin(coom_t *c) {
    if (!c->windowed) XSetInputFocus(c->dpy, c->win, RevertToParent, CurrentTime);
    XWindowAttributes attr = {0};
    XGetWindowAttributes(c->dpy, c->win, &attr);
    glViewport(0, 0, attr.width, attr.height);
    c->winsize = vec2(attr.width, attr.height);

    XEvent ev;
    while (XPending(c->dpy) > 0) {
        XNextEvent(c->dpy, &ev);
        if (XFilterEvent(&ev, None)) continue;
        coom_process_events(c, ev);
    }
}

void coom_end(coom_t *c) {
    glXSwapBuffers(c->dpy, c->win);
    glFlush();
    XSync(c->dpy, 0);
}

void coom_draw(coom_t *c) {
    if (fabs(c->cam.deltascale) > 0.5) {
        vec2_t half_winsize = vec2_mulf(c->winsize, 0.5);
        vec2_t _p0          = vec2_sub(c->cam.scale_pivot, half_winsize);
        vec2_t p0           = vec2_divf(_p0, c->cam.scale);
        c->cam.scale        = fmax(c->cam.scale + (c->cam.deltascale * c->dt), c->cfg->min_scale);
        vec2_t _p1          = vec2_sub(c->cam.scale_pivot, half_winsize);
        vec2_t p1           = vec2_divf(_p1, c->cam.scale);

        vec2_t pdelta       = vec2_sub(p0, p1);
        vec2_add_assign(&c->cam.position, pdelta);
        c->cam.deltascale -= c->cam.deltascale * c->dt * c->cfg->scale_friction;
    }
    if (!c->mouse.drag && (vec2_lenght(c->cam.velocity) > VELOCITY_THRESHOLD)) {
        vec2_t velocty_dt = vec2_mulf(c->cam.velocity, c->dt);
        vec2_add_assign(&c->cam.position, velocty_dt);
        vec2_t velocty_friction = vec2_mulf(velocty_dt, c->cfg->drag_friction);
        vec2_add_assign(&c->cam.velocity, velocty_friction);
    }

    if (fabsf(c->fl.delta_radius) > 1.0) {
        c->fl.radius = fmaxf(0.0, c->fl.radius + c->fl.delta_radius * c->dt);
        c->fl.delta_radius -= c->fl.delta_radius * 10.0 * c->dt;
    }
    if (c->fl.enabled) c->fl.shadow = fminf(c->fl.shadow + 6.0 * c->dt, 0.8);
    else c->fl.shadow = fmaxf(c->fl.shadow - 6.0 * c->dt, 0.0);

    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(c->prog);
    glUniform2f(glGetUniformLocation(c->prog, "cameraPos"), c->cam.position.x, c->cam.position.y);
    glUniform1f(glGetUniformLocation(c->prog, "cameraScale"), c->cam.scale);
    glUniform2f(glGetUniformLocation(c->prog, "screenshotSize"), c->img->width, c->img->height);
    glUniform2f(glGetUniformLocation(c->prog, "windowSize"), c->winsize.x, c->winsize.y);
    glUniform2f(glGetUniformLocation(c->prog, "cursorPos"), c->mouse.curr.x, c->mouse.curr.y);
    glUniform1f(glGetUniformLocation(c->prog, "flShadow"), c->fl.shadow);
    glUniform1f(glGetUniformLocation(c->prog, "flRadius"), c->fl.radius);
    glBindVertexArray(c->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}
