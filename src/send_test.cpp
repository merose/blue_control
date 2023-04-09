// Implement a simple 0MQ server that echos messages.

#include <iostream>
#include <string>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <czmq.h>


double get_time() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec/1E9;
}


/**
 * Show program usage.
 */
void show_usage() {
    std::cerr << "usage: send_test -c count -s size [-p port] [-w wait_us]"
              << std::endl;
}


/**
 * Run a request/response server that echoes each request as the response.
 */
int main(int argc, char **argv) {
    int opt;
    int size = -1;
    int count = -1;
    int port = 10000;
    int wait_us = 0;

    while ((opt = getopt(argc, argv, "s:c:p:w:")) != -1) {
        switch (opt) {
        case 's':
            size = atoi(optarg);
            break;
        case 'c':
            count = atoi(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'w':
            wait_us = atoi(optarg);
            break;
        default:
            show_usage();
            return 1;
        }
    }

    if (count < 0 || size < 0) {
        std::cerr << "Size and count must be specified." << std::endl;
        show_usage();
        return 1;
    }
    if (count < 2) {
        std::cerr << "Count must be at least 2." << std::endl;
        show_usage();
        return 1;
    }

    std::string url = std::string("tcp://*:") + std::to_string(port);
    zsock_t *sock = zsock_new_pub(url.c_str());
    std::cout << "Press ENTER to start..." << std::endl;
    while (getchar() != '\n') {
        // do nothing
    }
    std::cout << "Sending on " << url << std::endl;
    
    byte buf[size] = {0};
    double start = get_time();
    for (int i=0; i < count; ++i) {
        // Set the initial byte to zero in the last message.
        if (i < count-1) {
            buf[0] = 0;
        } else {
            std::cout << "Sending last message" << std::endl;
            buf[0] = 1;
        }
        zsock_send(sock, "b", buf, (size_t) size);
        if (wait_us > 0) {
            usleep(wait_us);
        }
    }
    double stop = get_time();
    double elapsed = stop - start;

    std::cout << "Sent " << count << " msgs in " << elapsed << " sec"
              << " == " << (count/elapsed) << " msgs/sec" << std::endl;
    std::cout << "Press ENTER to exit..." << std::endl;
    while (getchar() != '\n') {
        // do nothing
    }

    zsock_destroy(&sock);
    return 0;
}
