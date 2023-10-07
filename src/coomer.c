#include "coomer.h"

#include <math.h>

XImage *coom_new_screenshot(Display *dpy, Window win) {
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, win, &attr);
    return XGetImage(dpy, win, 0, 0, attr.width, attr.height, AllPlanes, ZPixmap);
}

void coom_delete_screenshot(XImage *img) {
    if (img) XDestroyImage(img);
}

bool coom_save_to_ppm(XImage *img, const char *file_path) {
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

void coom_flashlight_update(coom_flashlight *fl, f32 dt) {
    if (fabsf(fl->delta_radius) > 1.0) {
        fl->radius = fmaxf(0.0, fl->radius + fl->delta_radius * dt);
        fl->delta_radius -= fl->delta_radius * 10.0 * dt;
    }
    if (fl->enabled) fl->shadow = fminf(fl->shadow + 6.0 * dt, 0.8);
    else fl->shadow = fmaxf(fl->shadow - 6.0 * dt, 0.0);
}

vec2_t coom_mouse_pos(Display *dpy) {
    Window       root, child;
    int          root_x, root_y, win_x, win_y;
    unsigned int mask;
    XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
    return vec2(root_x, root_y);
}

vec2_t coom_camera_world(coom_camera *camera, vec2_t v) { return vec2_divf(v, camera->scale); }

void   coom_camera_update(coom_camera *camera, const coom_config_t *cfg, const coom_mouse *mouse, vec2_t winsize, f32 dt) {
    if (fabs(camera->deltascale) > 0.5) {
        vec2_t half_winsize = vec2_mulf(winsize, 0.5);
        vec2_t _p0          = vec2_sub(camera->scale_pivot, half_winsize);
        vec2_t p0           = vec2_divf(_p0, camera->scale);
        camera->scale       = fmax(camera->scale + (camera->deltascale * dt), cfg->min_scale);
        vec2_t _p1          = vec2_sub(camera->scale_pivot, half_winsize);
        vec2_t p1           = vec2_divf(_p1, camera->scale);

        vec2_t pdelta       = vec2_sub(p0, p1);
        vec2_add_assign(&camera->position, pdelta);
        camera->deltascale -= camera->deltascale * dt * cfg->scale_friction;
    }

    if (!mouse->drag && (vec2_lenght(camera->velocity) > VELOCITY_THRESHOLD)) {
        vec2_t velocty_dt = vec2_mulf(camera->velocity, dt);
        vec2_add_assign(&camera->position, velocty_dt);
        vec2_t velocty_friction = vec2_mulf(velocty_dt, cfg->drag_friction);
        vec2_add_assign(&camera->velocity, velocty_friction);
    }
}

void coom_image_draw(const XImage *img, const coom_camera *cam, const vec2_t winsize, const coom_mouse *mouse, const coom_flashlight *fl, GLuint shader,
                     GLuint vao) {
    if (img == NULL) return;
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);
    glUniform2f(glGetUniformLocation(shader, "cameraPos"), cam->position.x, cam->position.y);
    glUniform1f(glGetUniformLocation(shader, "cameraScale"), cam->scale);
    glUniform2f(glGetUniformLocation(shader, "screenshotSize"), img->width, img->height);
    glUniform2f(glGetUniformLocation(shader, "windowSize"), winsize.x, winsize.y);
    glUniform2f(glGetUniformLocation(shader, "cursorPos"), mouse->curr.x, mouse->curr.y);
    glUniform1f(glGetUniformLocation(shader, "flShadow"), fl->shadow);
    glUniform1f(glGetUniformLocation(shader, "flRadius"), fl->radius);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}
