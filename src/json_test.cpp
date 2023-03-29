#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <czmq.h>
#include "json.hpp"

using json = nlohmann::json;


int main(int argc, char **argv) {
    json obj = json::parse("{\"x\": 4, \"y\": \"hello\", \"servo1\": 123}");

    printf("x=%d\n", (int) obj["x"]);
    printf("y=%s\n", ((std::string) obj["y"]).c_str());
    printf("x is null: %d, unknown is null: %d\n",
       obj["x"].is_null(), obj["unknown"].is_null());
    std::string name = "servo";
    name += 1;
    std::cout << name << " is null: " << obj[name].is_null() << std::endl;

    return 0;
}
