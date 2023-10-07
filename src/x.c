#include "coomer.h"

#define ERROR_BUFFER_SIZE 2 << 10
int coom_xerror_handler(Display *d, XErrorEvent *e) {
    char buffer[ERROR_BUFFER_SIZE] = {0};
    XGetErrorText(d, e->error_code, buffer, sizeof(buffer));
    coom_error("%s - (X11 Error)", buffer);
    return 0;
}

Window coom_select_window(Display *d) {
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
    XFreeCursor(d, cursor);
    XUngrabPointer(d, CurrentTime);
    XUngrabKeyboard(d, CurrentTime);
    return result;
}

// clang-format off
static const strview frag_shader = SV(
    "#version 130\n"
    "out mediump vec4 color;\n"
    "in mediump vec2 texcoord;\n"
    "uniform sampler2D tex;\n"
    "uniform vec2 cursorPos;\n"
    "uniform vec2 windowSize;\n"
    "uniform float flShadow;\n"
    "uniform float flRadius;\n"
    "uniform float cameraScale;\n"
    "void main() {\n"
    "   vec4 cursor = vec4(cursorPos.x, windowSize.y - cursorPos.y, 0.0, 1.0);\n"
    "   color = mix(texture(tex, texcoord), vec4(0.0, 0.0, 0.0, 0.0), length(cursor - gl_FragCoord) < (flRadius * cameraScale) ? 0.0 : flShadow);\n"
    "}\n\0"
);
static const strview vert_shader = SV(
    "#version 130\n"
    "in vec3 aPos;\n"
    "in vec2 aTexCoord;\n"
    "out vec2 texcoord;\n"
    "uniform vec2 cameraPos;\n"
    "uniform float cameraScale;\n"
    "uniform vec2 windowSize;\n"
    "uniform vec2 screenshotSize;\n"
    "uniform vec2 cursorPos;\n"
    "vec3 to_world(vec3 v) {\n"
    "vec2 ratio = vec2(windowSize.x / screenshotSize.x / cameraScale, windowSize.y / screenshotSize.y / cameraScale);\n"
    "   return vec3((v.x / screenshotSize.x * 2.0 - 1.0) / ratio.x, (v.y / screenshotSize.y * 2.0 - 1.0) / ratio.y, v.z);\n"
    "}\n"
    "void main() {\n"
    "   gl_Position = vec4(to_world((aPos - vec3(cameraPos * vec2(1.0, -1.0), 0.0))), 1.0);\n"
    "   texcoord = aTexCoord;\n"
    "}\n\0"
);
// clang-format on

static GLuint coom_new_shader(const strview shader, GLenum kind) {
    GLuint result = glCreateShader(kind);
    glShaderSource(result, 1, &shader.data, NULL);
    glCompileShader(result);

    GLint success;
    glGetShaderiv(result, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buff[512];
        glGetShaderInfoLog(result, sizeof(buff), NULL, buff);
        coom_error("during shader compilation: %s", buff);
    }
    return result;
}

static GLuint coom_new_shader_prog(const strview vertex, const strview fragment) {
    GLuint prog            = glCreateProgram();
    GLuint vertex_shader   = coom_new_shader(vertex, GL_VERTEX_SHADER);
    GLuint fragment_shader = coom_new_shader(fragment, GL_FRAGMENT_SHADER);
    glAttachShader(prog, vertex_shader);
    glAttachShader(prog, fragment_shader);
    glLinkProgram(prog);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    GLint success;
    char  infolog[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, sizeof(infolog), NULL, infolog);
        coom_error("during linking prog: %s", infolog);
    }
    glUseProgram(prog);
    return prog;
}

GLuint coom_initialize_shader(GLuint *vao, GLuint *vbo, GLuint *ebo, XImage *img) {
    assert(img);
    GLuint  shader_program = coom_new_shader_prog(vert_shader, frag_shader);
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
    glDeleteVertexArrays(1, vao);
    glDeleteBuffers(1, vbo);
    glDeleteBuffers(1, ebo);
    glDeleteProgram(*program);
}
