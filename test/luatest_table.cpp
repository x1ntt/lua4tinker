#include <iostream>
#include "lua4tinker.h"


int main() {
    lua_State * L = lua4tinker::new_state();
    lua4tinker::open_libs(L);

    lua_newtable(L);
    
    lua_pushstring(L, "age");
    lua_pushnumber(L, 23);
    lua_settable(L, 1);

    lua_pushstring(L, "name");
    lua_pushstring(L, "Colen");
    lua_settable(L, 1);

    lua_setglobal(L, "student");

    int ret =  lua4tinker::do_string(L, R"(
        print ("Hi~")
        print (student['age'])
        print (student['name'])
    )");

    return ret;
}