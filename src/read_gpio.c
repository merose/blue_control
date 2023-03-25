#include <stdio.h>
#include <stdlib.h>

#include <rc/gpio.h>
#include <rc/time.h>


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: read_gpio <chipno> <pinno>\n");
	return 1;
    }

    int chip_no = atoi(argv[1]);
    int pin_no = atoi(argv[2]);

    rc_gpio_init(chip_no, pin_no, GPIOHANDLE_REQUEST_INPUT);
    for (;;) {
        int value = rc_gpio_get_value(chip_no, pin_no);
        printf("value: %d\n", value);
        rc_usleep(50000);
    }

    return 0;
}
