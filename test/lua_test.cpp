#include <iostream>
#include "lua4tinker.h"

int CFunction(lua_State *L) {
    int n = lua_gettop(L);
    int arg1 = lua_tonumber(L, n);
    std::cout << "here is CPP~, arg1: " << arg1 << "\tn: " << n << std::endl;
    lua_pushnumber(L, arg1);
    return 1;
}

int main() {
    lua_State * L = lua_open (8192);
    lua_baselibopen(L);
    lua_iolibopen(L);
    lua_strlibopen(L);
    lua_mathlibopen(L);
    lua_dblibopen(L);

    lua_pushstring(L, "Hello, String~");
    lua_setglobal(L, "strr");   // 设置栈上的内容为变量

    lua_register(L, "CFunction", CFunction);

    std::string code = R"(
        print (strr);
        print (CFunction(123))

        function Sum(a, b, c, d)
            local sum = a+b+c+d;
            local ave = sum/4;
            return sum,ave;
        end
    )";
    
    std::cout << "lua_stackspace: " << lua_stackspace(L) << std::endl;

    lua_dostring(L, code.c_str());

    lua_getglobal(L, "Sum");
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushnumber(L, 3);
    lua_pushnumber(L, 4);

    int ret = lua_call(L, 4, 2);
    double sum=0,ave=0;

    if(lua_isnumber(L,1))
    {
        sum=lua_tonumber(L,1);
    }
    if(lua_isnumber(L,2))
    {
        ave=lua_tonumber(L,2);
    }
    lua_pop(L,2);
    std::cout << "ret(" << ret << ")" << "sum: " << sum << ", ave: " << ave << std::endl;

    lua_close(L);
    return 0;
}