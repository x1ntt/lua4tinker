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
    
    // lua4tinker::def(L, "lamfunc", [](int a, std::string str, float flt) -> int {
    //     std::cout<< a << ", " << str << ", " << flt <<std::endl;
    //     return a + str.size();
    // });

    int ret =  lua4tinker::do_string(L, R"(
        assert(cpp_sum(1, 2, 3) == 6)
        assert(strcheck(6, "hello~") == 1)

        function max(n1, n2)
            if (n1 > n2) then
                res = n1;
            else
                res = n2;
            end
            return res;
        end 
    )");

    if (lua4tinker::call<int>(L, "max", 1, std::string("2d"), 3) != 2) {
        return 1;
    }

    return ret;
}