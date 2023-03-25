// Tests PID control of motors

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <robotcontrol.h>


#define MIN_DUTY_CYCLE 0.7
#define MAX_ACCELERATION 0.01


double get_time() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1E9;
}


int main(int argc, char **argv) {
    if (argc != 5) {
        printf("usage: test_pid motor_no encoder_no desired Kp\n");
        return 1;
    }

    int motor = atoi(argv[1]);
    int encoder = atoi(argv[2]);
    int desired = atoi(argv[3]);
    double kp = atof(argv[4]);

    rc_motor_init();
    rc_encoder_init();

    int last_encoder_value = 0;
    double last_time = get_time();
    double duty_cycle = 0.0;
    rc_motor_set(motor, duty_cycle);

    for (;;) {
        rc_motor_set(motor, duty_cycle);
        usleep(20000);

        int new_encoder_value = rc_encoder_read(encoder);
        int delta = new_encoder_value - last_encoder_value;
        double new_time = get_time();
        double dt = new_time - last_time;
        double rate = delta/dt;

        double delta_rate = desired - rate;
        double delta_duty = kp * delta_rate;
        if (delta_duty > MAX_ACCELERATION) {
            delta_duty = MAX_ACCELERATION;
        } else if (delta_duty < -MAX_ACCELERATION) {
            delta_duty = -MAX_ACCELERATION;
        }
        duty_cycle += delta_duty;

        printf("Motor %d: old=%d new=%d delta=%d dt=%.3f rate=%.1f duty=%.3f\n",
               motor, last_encoder_value, new_encoder_value, delta, dt,
               rate, duty_cycle);
        last_time = new_time;
        last_encoder_value = new_encoder_value;
    }


    return 0;
}
