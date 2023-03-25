// Tests PID control of motors

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <robotcontrol.h>


#define LEFT_MOTOR 1
#define LEFT_ENCODER 3
#define RIGHT_MOTOR 2
#define RIGHT_ENCODER 2

#define K_P 0.0001
#define MIN_DUTY_CYCLE 0.07
#define MAX_ACCELERATION 0.01

#define LEFT_FRONT_DETECTOR 1,25
#define RIGHT_FRONT_DETECTOR 1,17
#define LEFT_REAR_DETECTOR 3,20
#define RIGHT_REAR_DETECTOR 3,17

struct MobilityRequest {
    double kp;
    int desired_left;
    int desired_right;
};

struct MobilityResponse {
    double dt;
    int delta_left;
    double rate_left;
    double duty_left;
    int delta_right;
    double rate_right;
    double duty_right;
    double battery_voltage;
    double jack_voltage;
    int left_front_detector;
    int right_front_detector;
    int left_rear_detector;
    int right_rear_detector;
};

struct MotorState {
    int motor;
    int encoder;
    int target;
    int ticks;
    int delta_ticks;
    double rate;
    double duty_cycle;
};


double get_time() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1E9;
}


void update_motor(MotorState &state, double dt, double kp) {
    int ticks = rc_encoder_eqep_read(state.encoder);
    int delta_ticks = ticks - state.ticks;
    double rate = delta_ticks/dt;

    double delta_duty = kp * (state.target - rate);
    if (delta_duty > MAX_ACCELERATION) {
        delta_duty = MAX_ACCELERATION;
    } else if (delta_duty < -MAX_ACCELERATION) {
        delta_duty = -MAX_ACCELERATION;
    }
    double duty_cycle = state.duty_cycle + delta_duty;
    if (state.target == 0) {
        duty_cycle = 0.0;
    } else if (0 <= duty_cycle && duty_cycle < MIN_DUTY_CYCLE) {
        duty_cycle = MIN_DUTY_CYCLE;
    } else if (-MIN_DUTY_CYCLE < duty_cycle && duty_cycle <= 0) {
        duty_cycle = -MIN_DUTY_CYCLE;
    } 

    printf("Motor %d: old=%d new=%d delta=%d dt=%.3f rate=%.1f duty=%.3f\n",
           state.motor, state.ticks, ticks, delta_ticks, dt, rate, duty_cycle);

    state.duty_cycle = duty_cycle;
    state.ticks = ticks;
    state.delta_ticks = delta_ticks;
    state.rate = rate;
    state.duty_cycle = duty_cycle;
}


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: mobility_server left right\n");
        return 1;
    }

    rc_motor_init();
    rc_encoder_eqep_init();
    rc_adc_init();
    rc_gpio_init(LEFT_FRONT_DETECTOR, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(RIGHT_FRONT_DETECTOR, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(LEFT_REAR_DETECTOR, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(RIGHT_REAR_DETECTOR, GPIOHANDLE_REQUEST_INPUT);

    MotorState left;
    left.motor = LEFT_MOTOR;
    left.encoder = LEFT_ENCODER;
    // Left motor is reversed
    left.target = -atoi(argv[1]);
    left.ticks = rc_encoder_eqep_read(LEFT_ENCODER);
    left.duty_cycle = 0.0;

    MotorState right;
    right.motor = RIGHT_MOTOR;
    right.encoder = RIGHT_ENCODER;
    right.target = atoi(argv[2]);
    right.ticks = rc_encoder_eqep_read(RIGHT_ENCODER);
    right.duty_cycle = 0.0;

    double last_time = get_time();
    
    for (;;) {
        usleep(20000);

        double now = get_time();
        double dt = now - last_time;
        update_motor(left, dt, K_P);
        update_motor(right, dt, K_P);

        MobilityResponse res;
        res.dt = dt;
        res.delta_left = left.delta_ticks;
        res.rate_left = left.rate;
        res.duty_left = left.duty_cycle;
        res.delta_right = right.delta_ticks;
        res.rate_right = right.rate;
        res.duty_right = right.duty_cycle;
        res.battery_voltage = rc_adc_batt();
        res.jack_voltage = rc_adc_dc_jack();
        res.left_front_detector = rc_gpio_get_value(LEFT_FRONT_DETECTOR);
        res.right_front_detector = rc_gpio_get_value(RIGHT_FRONT_DETECTOR);
        res.left_rear_detector = rc_gpio_get_value(LEFT_REAR_DETECTOR);
        res.right_rear_detector = rc_gpio_get_value(RIGHT_REAR_DETECTOR);

        rc_motor_set(left.motor, left.duty_cycle);
        rc_motor_set(right.motor, right.duty_cycle);

        last_time = now;
    }

    rc_adc_cleanup();
    return 0;
}
