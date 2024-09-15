#include <iostream>
#include "lua4tinker.h"

int sum(int a, int b, int c) {
    return (a+b+c);
}

int main() {
    lua_State * L = lua4tinker::new_state();
    lua4tinker::open_libs(L);

    lua4tinker::def(L, "cpp_sum", sum);

    lua4tinker::do_string(L, R"(
        print (cpp_sum(1, 2, 3))
    )");

    return 0;
}