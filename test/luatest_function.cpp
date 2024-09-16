#include <iostream>
#include "lua4tinker.h"

int sum(int a, int b, int c) {
    return (a+b+c);
}

int strcheck(int a, std::string ss) {
    std::cout<<ss.size()<<std::endl;
    return ss.size() == a;
}

int main() {
    lua_State * L = lua4tinker::new_state();
    lua4tinker::open_libs(L);

    lua4tinker::def(L, "cpp_sum", sum);
    lua4tinker::def(L, "strcheck", strcheck);

    return lua4tinker::do_string(L, R"(
        assert(cpp_sum(1, 2, 3) == 6)
        assert(strcheck(6, "hello~") == 1)
    )");
}