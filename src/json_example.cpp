#include <iostream>
#include "json.hpp"

using json = nlohmann::json;


int main(void) {
    json obj = json::parse("{\"x\": 4, \"y\": 5}");
    std::cout << "obj=" << obj.dump() << std::endl;
    std::cout << "obj[x]=" << obj["x"] << std::endl;
    std::cout << "obj[y]=" << obj["y"] << std::endl;

    json obj2 = {
        {"a", 3.14},
        {"b", "hello"},
        {"c", 123}
    };
    std::cout << "obj2=" << obj2.dump() << std::endl;
    std::cout << "obj2[a]=" << obj2["a"] << std::endl;
    std::cout << "obj2[b]=" << obj2["b"] << std::endl;
    std::cout << "obj2[c]=" << obj2["c"] << std::endl;

    return 0;
}
