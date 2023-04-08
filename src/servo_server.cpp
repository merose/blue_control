// Tests PID control of motors

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <robotcontrol.h>
#include <czmq.h>

#include "json.hpp"

using json = nlohmann::json;

#define SERVO_COUNT 8

int min_pulse;
int max_pulse;
int servo_enabled[SERVO_COUNT] = {0};
int servo_pulse[SERVO_COUNT];
double last_command;
double command_timeout_secs;
int command_timeout;


double get_time() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1E9;
}


double get_double_config(zconfig_t *root, const char *path) {
    const char *value = zconfig_get(root, path, NULL);
    if (value == NULL) {
        std::cerr << "No configuration value for " << path << std::endl;
        exit(1);
    }

    return atof(value);
}


double get_int_config(zconfig_t *root, const char *path) {
    const char *value = zconfig_get(root, path, NULL);
    if (value == NULL) {
        std::cerr << "No configuration value for " << path << std::endl;
        exit(1);
    }

    return atoi(value);
}


static void __signal_handler(__attribute__ ((unused)) int dummy) {
    exit(1);
}


int get_servo_pulse(double relative, int min_pulse, int max_pulse) {
    if (relative < 0) {
        relative = 0;
    } else if (relative > 1) {
        relative = 1;
    }

    int range = max_pulse - min_pulse;
    return (int) (min_pulse + relative*range);
}


int send_servo_pulses(zloop_t *loop, int timer_id, void *arg) {
    double now = get_time();

    // If it's been too long since we received a command, set desired
    // rates to zero.
    if (now - last_command >= command_timeout_secs) {
        if (!command_timeout) {
            std::cout << "Command timeout - setting targets to zero"
                    << std::endl;
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

    return 0;
}


int process_servo_commands(zloop_t *loop, zsock_t *reader, void *arg) {
    char *msg = zstr_recv(reader);
    std::cout << "Received command: " << msg << std::endl;
    json obj = json::parse(msg);
    for (int i=0; i < SERVO_COUNT; ++i) {
        std::string name = "servo" + std::to_string(i+1);
        if (obj[name].is_null()) {
            servo_enabled[i] = 0;
        } else {
            servo_enabled[i] = 1;
            servo_pulse[i] = get_servo_pulse((double) obj[name],
                                             min_pulse, max_pulse);
        }
    }
    zstr_free(&msg);
    last_command = get_time();
    command_timeout = 0;

    return 0;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "usage: servo_server config-file" << std::endl;
        return 1;
    }

    signal(SIGINT, __signal_handler);

    zconfig_t *root = zconfig_load(argv[1]);
    if (root == NULL) {
        std::cerr << "Cannot load configuration file " << argv[1] << std::endl;
        return 1;
    }

    int cmd_port = get_int_config(root, "/servos/cmd_port");

    std::string cmd_endpoint = "tcp://*:" + std::to_string(cmd_port);
    zsock_t *cmd_sock = zsock_new(ZMQ_SUB);
    zsock_bind(cmd_sock, cmd_endpoint.c_str());
    zsock_set_subscribe(cmd_sock, "");
    std::cout << "Bound command socket to " << cmd_endpoint << std::endl;

    min_pulse = get_int_config(root, "/servos/min_pulse");
    max_pulse = get_int_config(root, "/servos/max_pulse");
    std::cout << "Pulse range: " << min_pulse << ".." << max_pulse << std::endl;

    int loop_sleep = get_int_config(root, "/servos/update_interval_ms");
    command_timeout_secs =
        get_double_config(root, "/servos/command_timeout_secs");

    last_command = get_time();
    command_timeout = 0;

    rc_servo_init();
    rc_usleep(500000);
    rc_servo_power_rail_en(1);

    zloop_t *loop = zloop_new();
    zloop_reader(loop, cmd_sock, process_servo_commands, NULL);
    zloop_timer(loop, loop_sleep, 0, send_servo_pulses, NULL);
    zloop_start(loop);

    return 0;
}
