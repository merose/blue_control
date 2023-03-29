// Tests PID control of motors

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <robotcontrol.h>
#include <czmq.h>

#include "json.hpp"

using json = nlohmann::json;

#define SERVO_COUNT 8


double get_time() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1E9;
}


double get_double_config(zconfig_t *root, const char *path) {
    const char *value = zconfig_get(root, path, NULL);
    if (value == NULL) {
        fprintf(stderr, "No configuration value for %s", path);
        exit(1);
    }

    return atof(value);
}


double get_int_config(zconfig_t *root, const char *path) {
    const char *value = zconfig_get(root, path, NULL);
    if (value == NULL) {
        fprintf(stderr, "No configuration value for %s", path);
        exit(1);
    }

    return atoi(value);
}


static void __signal_handler(__attribute__ ((unused)) int dummy) {
    exit(1);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: servo_server config-file\n");
        return 1;
    }

    signal(SIGINT, __signal_handler);

    zconfig_t *root = zconfig_load(argv[1]);
    if (root == NULL) {
        fprintf(stderr, "Cannot load configuration file %s\n", argv[1]);
        return 1;
    }

    int cmd_port = get_int_config(root, "/servos/cmd_port");
    int min_pulse = get_int_config(root, "/servos/min_pulse");
    int max_pulse = get_int_config(root, "/servos/max_pulse");

    char cmd_endpoint[] = "tcp://*:99999";
    sprintf(cmd_endpoint, "tcp://*:%d", cmd_port);
    zsock_t *cmd_sock = zsock_new(ZMQ_SUB);
    zsock_bind(cmd_sock, cmd_endpoint);
    zsock_set_subscribe(cmd_sock, "");
    printf("Bound command socket to %s\n", cmd_endpoint);

    int loop_sleep = get_int_config(root, "/servos/update_interval_us");
    double command_timeout_secs =
            get_double_config(root, "/servos/command_timeout_secs");

    double last_time = get_time();
    double last_command = last_time;
    int command_timeout = 0;

    int servo_enabled[SERVO_COUNT] = {0};
    int servo_pulse[SERVO_COUNT];
    
    rc_servo_init();
    rc_usleep(500000);
    rc_servo_power_rail_en(1);

    for (;;) {
        char *msg = zstr_recv_nowait(cmd_sock);
        if (msg != NULL) {
            json obj = json::parse(msg);
            for (int i=0; i < SERVO_COUNT; ++i) {
                char name[] = "servoN";
                sprintf(name, "servo%d", i+1);
                if (!obj[name].is_null()) {
                    servo_enabled[i] = 0;
                } else {
                    servo_enabled[i] = 1;
                    servo_pulse[i] = (int) obj[name];
                }
            }
            zstr_free(&msg);
            last_command = get_time();
            command_timeout = 0;
        }

        double now = get_time();

        // If it's been too long since we received a command, set desired
        // rates to zero.
        if (now - last_command >= command_timeout_secs) {
            if (!command_timeout) {
                printf("Command timeout - setting targets to zero\n");
            }
            for (int i=0; i < SERVO_COUNT; ++i) {
                servo_enabled[i] = 0;
            }
            command_timeout = 1;
        }

        for (int i=0; i < SERVO_COUNT; ++i) {
            if (servo_enabled[i]) {
                rc_servo_send_pulse_us(i+1, servo_pulse[i]);
            }
        }

        last_time = now;
    }

    return 0;
}
