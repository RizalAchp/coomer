#define CB_IMPLEMENTATION
#include "cb.h"

cb_status_t add_libraries(cb_t *cb, cb_target_t *target) {
    cb_target_t *libxext   = cb_create_target_pkgconf(cb, cb_sv("xext"));
    cb_target_t *libx11    = cb_create_target_pkgconf(cb, cb_sv("x11"));
    cb_target_t *libxrandr = cb_create_target_pkgconf(cb, cb_sv("xrandr"));
    cb_target_t *libgl     = cb_create_target_pkgconf(cb, cb_sv("gl"));
    return cb_target_link_library(target, libxext, libx11, libxrandr, libgl, NULL);
}

cb_status_t on_configure(cb_t *cb, cb_config_t *cfg) {
    (void)cfg;
    cb_status_t  status = CB_OK;

    cb_target_t *coomer = cb_create_exec(cb, "coomer");
    status &= cb_target_add_includes(coomer, "./include", NULL);
    status &= cb_target_add_defines(coomer, "COOMER_VERSION=\"0.1.0\"", NULL);
    status &= cb_target_add_flags(coomer, "-Wall", "-Wextra", "-pedantic", "-O2", "-ffast-math", NULL);
    status &= cb_target_add_ldflags(coomer, "-lm", NULL);
    status &= cb_target_add_sources_with_ext(coomer, "./src", "c", false);
    status &= add_libraries(cb, coomer);

    return status;
}

int main(int argc, char *argv[]) {
    CB_REBUILD_SELF(argc, argv);
    cb_t *cb = cb_init(argc, argv);
    if (cb_run(cb) == CB_ERR) {
        cb_deinit(cb);
        return EXIT_FAILURE;
    }
    cb_deinit(cb);
    return EXIT_SUCCESS;
}
