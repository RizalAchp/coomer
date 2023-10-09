#include "coomer.h"

static int coom_xerror_handler(Display *d, XErrorEvent *e) {
    char *temp = temp_alloc(1 << 10);
    XGetErrorText(d, e->error_code, temp, 1 << 10);
    coom_error("%s - (X11 Error)", temp);
    temp_free(512);
    return 0;
}

Display *coom_open_display(void) {
    Display *dpy = XOpenDisplay(NULL);
    ASSERT_EXIT(dpy != NULL, 1, "Failed to open display");
    XSetErrorHandler(coom_xerror_handler);
    return dpy;
}

Window coom_select_window(Display *d) {
    coom_info("%s", __PRETTY_FUNCTION__);
    Cursor cursor = XCreateFontCursor(d, XC_crosshair);

    Window result = DefaultRootWindow(d);
    XGrabPointer(d, result, 0, ButtonMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, result, cursor, CurrentTime);
    XGrabKeyboard(d, result, 0, GrabModeAsync, GrabModeAsync, CurrentTime);
    XEvent event;
    while (true) {
        XNextEvent(d, &event);
        switch (event.type) {
            case ButtonPress: return_defer(event.xbutton.subwindow);
            case KeyPress: goto defer;
        }
    }

defer:
    XUngrabKeyboard(d, CurrentTime);
    XUngrabPointer(d, CurrentTime);
    XFreeCursor(d, cursor);
    return result;
}
short coom_get_monitor_rate(Display *d) {
    coom_info("%s", __PRETTY_FUNCTION__);
    XRRScreenConfiguration *screen_cfg = XRRGetScreenInfo(d, DefaultRootWindow(d));
    short                   rate       = XRRConfigCurrentRate(screen_cfg);
    XRRFreeScreenConfigInfo(screen_cfg);
    return rate;
}

static GLuint coom_new_shader(const strview shader, GLenum kind) {
    coom_info("%s", __PRETTY_FUNCTION__);
    GLuint result = glCreateShader(kind);
    glShaderSource(result, 1, &shader.data, NULL);
    glCompileShader(result);

    GLint success;
    glGetShaderiv(result, GL_COMPILE_STATUS, &success);
    if (!success) {
        char *temp = temp_alloc(512);
        glGetShaderInfoLog(result, 512, NULL, temp);
        coom_error("during shader compilation: %s", temp);
        temp_free(512);
    }
    return result;
}

static GLuint coom_new_shader_prog(void) {
    coom_info("%s", __PRETTY_FUNCTION__);
    GLuint prog = glCreateProgram();
    GLuint vertex_shader =
        coom_new_shader(SV("#version 130\n"
                           "in vec3 aPos;"
                           "in vec2 aTexCoord;"
                           "out vec2 texcoord;"
                           "uniform vec2 cameraPos;"
                           "uniform float cameraScale;"
                           "uniform vec2 windowSize;"
                           "uniform vec2 screenshotSize;"
                           "uniform vec2 cursorPos;"
                           "vec3 to_world(vec3 v) {"
                           "vec2 ratio = vec2(windowSize.x / screenshotSize.x / cameraScale, windowSize.y / screenshotSize.y / cameraScale);"
                           "   return vec3((v.x / screenshotSize.x * 2.0 - 1.0) / ratio.x, (v.y / screenshotSize.y * 2.0 - 1.0) / ratio.y, v.z);"
                           "}"
                           "void main() {"
                           "   gl_Position = vec4(to_world((aPos - vec3(cameraPos * vec2(1.0, -1.0), 0.0))), 1.0);"
                           "   texcoord = aTexCoord;"
                           "}\0"),
                        GL_VERTEX_SHADER);
    GLuint fragment_shader = coom_new_shader(
        SV("#version 130\n"
           "out mediump vec4 color;"
           "in mediump vec2 texcoord;"
           "uniform sampler2D tex;"
           "uniform vec2 cursorPos;"
           "uniform vec2 windowSize;"
           "uniform float flShadow;"
           "uniform float flRadius;"
           "uniform float cameraScale;"
           "void main() {"
           "   vec4 cursor = vec4(cursorPos.x, windowSize.y - cursorPos.y, 0.0, 1.0);"
           "   color = mix(texture(tex, texcoord), vec4(0.0, 0.0, 0.0, 0.0), length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);"
           "}\0"),

        GL_FRAGMENT_SHADER);

    glAttachShader(prog, vertex_shader);
    glAttachShader(prog, fragment_shader);
    glLinkProgram(prog);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char *temp = temp_alloc(512);
        glGetProgramInfoLog(prog, 512, NULL, temp);
        coom_error("during linking prog: %s", temp);
        temp_free(512);
    }
    glUseProgram(prog);
    return prog;
}

GLuint coom_initialize_shader(GLuint *vao, GLuint *vbo, GLuint *ebo, XImage *img) {
    coom_info("%s", __PRETTY_FUNCTION__);
    assert(img);
    GLuint  shader_program = coom_new_shader_prog();
    GLfloat w              = img->width;
    GLfloat h              = img->height;

    GLfloat vertices[4][5] = {
        {w, 0, 0.0, 1.0, 1.0},  // Top right
        {w, h, 0.0, 1.0, 0.0},  // Bottom right
        {0, h, 0.0, 0.0, 0.0},  // Bottom left
        {0, 0, 0.0, 0.0, 1.0}   // Top left
    };
    GLuint indicies[] = {0, 1, 3, 1, 2, 3};

    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glGenBuffers(1, ebo);

    glBindVertexArray(*vao);

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), &indicies, GL_STATIC_DRAW);
    GLsizei stride = sizeof(vertices[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, stride, (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, img->data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);

    glEnable(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    return shader_program;
}

void coom_uninitialize_shader(GLuint *vao, GLuint *vbo, GLuint *ebo, GLuint *program) {
    coom_info("%s", __PRETTY_FUNCTION__);
    glDeleteVertexArrays(1, vao);
    glDeleteBuffers(1, vbo);
    glDeleteBuffers(1, ebo);
    glDeleteProgram(*program);
}

vec2_t coom_mouse_pos(Display *dpy) {
    coom_info("%s", __PRETTY_FUNCTION__);
    Window       root, child;
    int          root_x, root_y, win_x, win_y;
    unsigned int mask;
    XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
    return vec2(root_x, root_y);
}

XImage *coom_new_screenshot(Display *dpy, Window win) {
    coom_info("%s", __PRETTY_FUNCTION__);
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, win, &attr);
    XImage *img = XGetImage(dpy, win, 0, 0, attr.width, attr.height, AllPlanes, ZPixmap);
    ASSERT_EXIT(img != NULL, 1, "Failed to get screenshot, exiting");
    return img;
}

void coom_delete_screenshot(XImage *img) {
    coom_info("%s", __PRETTY_FUNCTION__);
    if (img) XDestroyImage(img);
}

bool coom_save_to_ppm(XImage *img, const char *file_path) {
    coom_info("%s", __PRETTY_FUNCTION__);
    if (img == NULL) return false;

    bool  result = true;
    FILE *f      = fopen(file_path, "wb");
    if (f == NULL) {
        coom_error("failed to open file: '%s' - %s", file_path, strerror(errno));
        return false;
    }
    fprintf(f, "P6\n%d %d\n255\n", img->width, img->height);
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            unsigned char color[3];
            unsigned long pixel = XGetPixel(img, x, y);
            color[2]            = pixel & img->blue_mask;
            color[1]            = (pixel & img->green_mask) >> 8;
            color[0]            = (pixel & img->red_mask) >> 16;
            fwrite(color, 1, 3, f);
        }
    }

    if (f) fclose(f);
    return result;
}
