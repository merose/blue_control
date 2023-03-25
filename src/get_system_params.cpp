// Show system integer sizes, endian-ness, and clock parameters.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>


#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))


struct ClockId {
    clockid_t clock_id;
    const char *name;
};


int main() {
    printf("sizeof(int): %ld\n", (long) sizeof(int));
    printf("sizeof(long): %ld\n", (long) sizeof(long));

    long value = 0x12345678;
    const char *p = (const char *) &value;
    for (int i=0; i < sizeof(value); ++i) {
        printf(" %02X", p[i]);
    }
    printf("\n");
    if (*p == 0x12) {
        printf("Big-endian\n");
    } else {
        printf("Little-endian\n");
    }

    ClockId clock_ids[] = {
        { CLOCK_REALTIME, "CLOCK_REALTIME" },
        { CLOCK_REALTIME_ALARM, "CLOCK_REALTIME_ALARM" },
        { CLOCK_REALTIME_COARSE, "CLOCK_REALTIME_COARSE" },
        { CLOCK_TAI, "CLOCK_TAI" },
        { CLOCK_MONOTONIC, "CLOCK_MONOTONIC" },
        { CLOCK_MONOTONIC_COARSE, "CLOCK_MONOTONIC_COARSE" },
        { CLOCK_MONOTONIC_RAW, "CLOCK_MONOTONIC_RAW" },
        { CLOCK_BOOTTIME, "CLOCK_BOOTTIME" },
        { CLOCK_BOOTTIME_ALARM, "CLOCK_BOOTTIME_ALARM" },
        { CLOCK_PROCESS_CPUTIME_ID, "CLOCK_PROCESS_CPUTIME_ID" },
        { CLOCK_THREAD_CPUTIME_ID, "CLOCK_THREAD_CPUTIME_ID" }
    };
    for (int i=0; i < ARRAY_LENGTH(clock_ids); ++i) {
        struct timespec res;

        if (clock_getres(clock_ids[i].clock_id, &res) == -1) {
            printf("Clock %s does not exist\n", clock_ids[i].name);
        } else {
            printf("Clock %s resolution: %10jd.%09ld\n", clock_ids[i].name,
                   (intmax_t) res.tv_sec, res.tv_nsec);
        }
    }

    return 0;
}
