#include <stdio.h>
#include <stdlib.h>

#include <rc/gpio.h>
#include <rc/time.h>


int main(int argc, char **argv) {
    rc_gpio_init(1, 25, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(1, 17, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(3, 20, GPIOHANDLE_REQUEST_INPUT);
    rc_gpio_init(3, 17, GPIOHANDLE_REQUEST_INPUT);
    for (;;) {
        int left_front = rc_gpio_get_value(1, 25);
        int right_front = rc_gpio_get_value(1, 17);
        int left_rear = rc_gpio_get_value(3, 20);
        int right_rear = rc_gpio_get_value(3, 17);
	printf("LF=%d RF=%d LR=%d RR=%d\n",
               left_front, right_front, left_rear, right_rear);
        rc_usleep(50000);
    }

    return 0;
}
