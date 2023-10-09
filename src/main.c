#include "coomer.h"
#include "util.h"

int main(int argc, char *argv[]) {
    int          result = 0;
    options_args args   = OPTIONS_ARGS_DEFAULT;

    if (!parse_args(&argc, &argv, &args)) return 1;
    if (args.new_config != NULL) return !coom_generate_default_config(args.new_config);
    if (args.delay_second) mssleep(args.delay_second * 1000);

    coom_t coom = coom_init_coom(args);

    while (!coom.quit) {
        coom_begin(&coom);
        coom_draw(&coom);
        coom_end(&coom);
    }

    coom_uninit_coom(&coom);
    return result;
}
