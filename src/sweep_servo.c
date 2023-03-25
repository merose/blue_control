#include <stdio.h>
#include <stdlib.h>

#include <rc/servo.h>
#include <rc/time.h>


int send_pulse(int ch, int ms) {
    for (int i=0; i < 50; ++i) {
        rc_servo_send_pulse_us(ch, ms);
	rc_usleep(20000);
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("usage: sweep_server servo_no low_us high_us\n");
	return 1;
    }

    int channel = atoi(argv[1]);
    int low_us = atoi(argv[2]);
    int high_us = atoi(argv[3]);
    printf("channel=%d low=%d high=%d\n", channel, low_us, high_us);

    rc_servo_init();
    rc_usleep(500000);
    rc_servo_power_rail_en(1);
    for (;;) {
	send_pulse(channel, low_us);
	send_pulse(channel, high_us);
    }
}
