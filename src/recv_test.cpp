// Implement a simple 0MQ server that echos messages.

#include <iostream>
#include <string>
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
    std::cerr << "usage: recv_test [-h host] [-p port]" << std::endl;
}


/**
 * Run a request/response server that echoes each request as the response.
 */
int main(int argc, char **argv) {
    int opt;
    std::string host = "localhost";
    int port = 10000;

    while ((opt = getopt(argc, argv, "h:p:")) != -1) {
        switch (opt) {
        case 'h':
            host = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            show_usage();
            return 1;
        }
    }

    std::string url = std::string("tcp://") + host + ":" + std::to_string(port);
    zsock_t *sock = zsock_new_sub(url.c_str(), "");
    
    std::cout << "Listening on " << url << std::endl;
    
    byte *buf;
    size_t size;
    int count = 0;
    double start;
    for (;;) {
        if (zsock_recv(sock, "b", &buf, &size) == -1) {
            std::cerr << "Error while receiving a message" << std::endl;
            break;
        }
        if (count == 0) {
            start = get_time();
        }
        ++count;
        if (buf[0] != 0) {
            break;
        }
        free(buf);
    }
    double stop = get_time();
    double elapsed = stop - start;

    // Don't count the first message.
    --count;
    std::cout << "Received " << count << " msgs in " << elapsed << " sec"
              << " == " << (count/elapsed) << " msgs/sec" << std::endl;

    zsock_destroy(&sock);
    return 0;
}
