// Implement a simple 0MQ server that echos messages.

#include <stdio.h>
#include <stdlib.h>
#include <czmq.h>


/**
 * Show program usage.
 */
void show_usage() {
    printf("usage: echo_server <port>\n");
}


/**
 * Run a request/response server that echoes each request as the response.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        show_usage();
        return 1;
    }

    char url[] = "tcp://*:99999";
    sprintf(url, "tcp://*:%d", atoi(argv[1]));
    zsock_t *sock = zsock_new_rep(url);
    printf("Listening on %s\n", url);
    
    for (;;) {
        char buf[10000];
        char *msg = zstr_recv(sock);
        printf("Received message: %s\n", msg);

        zstr_send(sock, msg);
        zstr_free(&msg);
    }

    zsock_destroy(&sock);
    return 0;
}
