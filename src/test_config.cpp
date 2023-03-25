#include <stdio.h>
#include <stdlib.h>
#include <czmq.h>


void show_config(zconfig_t *root, const char *name, const char *default_value) {
    const char *value = zconfig_get(root, name, default_value);
    printf("%s=%s\n", name, value);
}


int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: test_config config-file\n");
        return 1;
    }

    zconfig_t *root = zconfig_load(argv[1]);
    if (root == NULL) {
        fprintf(stderr, "Cannot load configuration file %s\n", argv[1]);
        return 1;
    }

    show_config(root, "/parameters/kp", "<123>");
    show_config(root, "/left/scale", "<1>");
    show_config(root, "/right/scale", "<1>");
    show_config(root, "/parameters/nonexistent", "<default-value>");

    return 0;
}
