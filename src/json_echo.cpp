#include <stdio.h>
#include <stdlib.h>
#include <czmq.h>
#include "json.hpp"

using json = nlohmann::json;


int main(int argc, char **argv) {
    zsock_t *sock = zsock_new_rep("tcp://*:10000");
    
    char buf[10000];
    char *msg = zstr_recv(sock);
    printf("Received message: %s\n", msg);

    json obj = json::parse(msg);
    zstr_free(&msg);
    printf("Request is %s\n", obj["request"].get<std::string>().c_str());

    json response;
    response["message"] = obj["message"];
    std::string reply = response.dump();

    zstr_send(sock, reply.c_str());

    zsock_destroy(&sock);
    return 0;
}
