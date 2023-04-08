// Tests PID control of motors

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <robotcontrol.h>
#include <czmq.h>

#include "json.hpp"

using json = nlohmann::json;


zsock_t *status_sock;

double min_duty_cycle;
double max_acceleration;
double kp;
#define MAX_DUTY_CYCLE 1.0

struct MobilityRequest {
    int left_target;
    int right_target;
};

struct MotorState {
    int motor;
    int encoder;
    int scale;
    int target;
    int ticks;
    int delta_ticks;
    double rate;
    double duty_cycle;
};

struct GpioPin {
    int chip;
    int pin;
};

MotorState left;
MotorState right;

GpioPin left_front;
GpioPin right_front;
GpioPin left_rear;
GpioPin right_rear;

double command_timeout_secs;
double last_time;
double last_command;
int command_timeout;


double get_time() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1E9;
}


void update_motor(MotorState &state, double dt) {
    int ticks = rc_encoder_eqep_read(state.encoder);
    int delta_ticks = ticks - state.ticks;
    double rate = delta_ticks/dt;

    double delta_duty = kp * (state.target - rate);
    if (delta_duty > max_acceleration) {
        delta_duty = max_acceleration;
    } else if (delta_duty < -max_acceleration) {
        delta_duty = -max_acceleration;
    }
    double duty_cycle = state.duty_cycle + delta_duty;
    if (state.target == 0) {
        duty_cycle = 0.0;
    } else if (0 <= duty_cycle && duty_cycle < min_duty_cycle) {
        duty_cycle = min_duty_cycle;
    } else if (-min_duty_cycle < duty_cycle && duty_cycle <= 0) {
        duty_cycle = -min_duty_cycle;
    } else if (duty_cycle > MAX_DUTY_CYCLE) {
        duty_cycle = MAX_DUTY_CYCLE;
    } else if (duty_cycle < -MAX_DUTY_CYCLE) {
        duty_cycle = -MAX_DUTY_CYCLE;
    }

    std::cout << "Motor " << state.motor << ": target=" << state.target
            << " old=" << state.ticks << " new=" << ticks
            << " delta=" << delta_ticks << " dt=" << dt
            << " rate=" << rate << " duty=" << duty_cycle << std::endl;

    state.duty_cycle = duty_cycle;
    state.ticks = ticks;
    state.delta_ticks = delta_ticks;
    state.rate = rate;
    state.duty_cycle = duty_cycle;
}


GpioPin get_gpio_pin(zconfig_t *root, const char *prefix) {
    std::string path = prefix;
    
    int chip = atoi(zconfig_get(root, (path + "/chip").c_str(), "-1"));
    int pin = atoi(zconfig_get(root, (path + "/pin").c_str(), "-1"));

    if (chip < 0 || pin < 0) {
        std::cerr << "Pin configuration for " << prefix << " is invalid"
                << std::endl;
        exit(1);
    }

    GpioPin gpio = { chip, pin };
    return gpio;
}


int get_gpio_value(GpioPin &gpio) {
    return rc_gpio_get_value(gpio.chip, gpio.pin);
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


int update_motors(zloop_t *loop, int timer_id, void *arg) {
    double now = get_time();

    // If it's been too long since we received a command, set desired
    // rates to zero.
    if (now - last_command >= command_timeout_secs) {
        if (!command_timeout) {
            std::cout << "Command timeout - setting targets to zero"
                    << std::endl;
        }
        left.target = 0;
        right.target = 0;
        command_timeout = 1;
    }

    double dt = now - last_time;
    update_motor(left, dt);
    update_motor(right, dt);

    rc_motor_set(left.motor, left.duty_cycle);
    rc_motor_set(right.motor, right.duty_cycle);

    json response;
    response["dt"] = dt;
    response["left"] = left.scale * left.ticks;
    response["delta_left"] = left.scale * left.delta_ticks;
    response["rate_left"] = left.scale * left.rate;
    response["duty_left"] = left.scale * left.duty_cycle;
    response["right"] = right.scale * right.ticks;
    response["delta_right"] = right.scale * right.delta_ticks;
    response["rate_right"] = right.scale * right.rate;
    response["duty_right"] = right.scale * right.duty_cycle;
    response["battery_voltage"] = rc_adc_batt();
    response["jack_voltage"] = rc_adc_dc_jack();
    response["left_front_detector"] = get_gpio_value(left_front);
    response["right_front_detector"] = get_gpio_value(right_front);
    response["left_rear_detector"] = get_gpio_value(left_rear);
    response["right_rear_detector"] = get_gpio_value(right_rear);

    std::string status = response.dump();
    zstr_send(status_sock, status.c_str());

    last_time = now;
    return 0;
}


int process_motor_command(zloop_t *loop, zsock_t *reader, void *arg) {
    char *msg = zstr_recv(reader);
    std::cout << "Received command: " << msg << std::endl;
    json obj = json::parse(msg);
    if (obj["left"].is_null()) {
        left.target = 0;
    } else {
        left.target = (int) obj["left"] * left.scale;
    }
    if (obj["right"].is_null()) {
        right.target = 0;
    } else {
        right.target = (int) obj["right"] * right.scale;
    }
    std::cout << "new targets: left=" << left.target
            << " right=" << right.target << std::endl;
    zstr_free(&msg);

    last_command = get_time();
    command_timeout = 0;
    return 0;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "usage: mobility_server config-file" << std::endl;
        return 1;
    }

    zconfig_t *root = zconfig_load(argv[1]);
    if (root == NULL) {
        std::cout << "Cannot load configuration file " << argv[1] << std::endl;
        return 1;
    }

    left_front = get_gpio_pin(root, "/detectors/left_front");
    right_front = get_gpio_pin(root, "/detectors/right_front");
    left_rear = get_gpio_pin(root, "/detectors/left_rear");
    right_rear = get_gpio_pin(root, "/detectors/right_rear");

    rc_motor_init();
    rc_encoder_eqep_init();
    rc_adc_init();
    rc_gpio_init(left_front.chip, left_front.pin, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(right_front.chip, right_front.pin, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(left_rear.chip, left_rear.pin, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(right_rear.chip, right_rear.pin, GPIOHANDLE_REQUEST_INPUT);

    left.motor = get_int_config(root, "/left/motor_index");
    left.encoder = get_int_config(root, "/left/encoder_index");
    left.scale = get_int_config(root, "/left/scale");
    left.target = 0;
    left.ticks = rc_encoder_eqep_read(left.encoder);
    left.duty_cycle = 0.0;

    right.motor = get_int_config(root, "/right/motor_index");
    right.encoder = get_int_config(root, "/right/encoder_index");
    right.scale = get_int_config(root, "/right/scale");
    right.target = 0;
    right.ticks = rc_encoder_eqep_read(right.encoder);
    right.duty_cycle = 0.0;

    int cmd_port = get_int_config(root, "/parameters/cmd_port");
    int status_port = get_int_config(root, "/parameters/status_port");

    std::string cmd_endpoint = "tcp://*:" + std::to_string(cmd_port);
    zsock_t *cmd_sock = zsock_new(ZMQ_SUB);
    zsock_bind(cmd_sock, cmd_endpoint.c_str());
    zsock_set_subscribe(cmd_sock, "");
    std::cout << "Bound command socket to " << cmd_endpoint << std::endl;

    std::string status_endpoint = "tcp://*:" + std::to_string(status_port);
    status_sock = zsock_new_pub(status_endpoint.c_str());
    std::cout << "Bound status socket to " << status_endpoint << std::endl;

    kp = get_double_config(root, "/parameters/kp");
    min_duty_cycle = get_double_config(root, "/parameters/min_duty_cycle");
    max_acceleration = get_double_config(root, "/parameters/max_acceleration");
    int loop_sleep = get_int_config(root, "/parameters/update_interval_ms");
    command_timeout_secs =
            get_double_config(root, "/parameters/command_timeout_secs");

    last_time = get_time();
    last_command = last_time;
    command_timeout = 0;

    zloop_t *loop = zloop_new();
    zloop_reader(loop, cmd_sock, process_motor_command, NULL);
    zloop_timer(loop, loop_sleep, 0, update_motors, NULL);
    zloop_start(loop);

    rc_adc_cleanup();
    return 0;
}
