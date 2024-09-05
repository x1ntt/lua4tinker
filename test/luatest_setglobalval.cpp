#include <iostream>
#include "lua4tinker.h"

int main() {
    lua_State * L = lua4tinker::new_state();
    lua4tinker::open_libs(L);

    lua4tinker::set(L, "test_str", 1);
    
    std::string code = R"(
        print(test_str)
    )";

    return lua4tinker::do_string(L, code);
}