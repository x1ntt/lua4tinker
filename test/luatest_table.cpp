#include <iostream>
#include "lua4tinker.h"

class A {
private:
    int _a;
public:
    A(int a) : _a(a) {}
    
    int p() {
        std::cout << "_a: " << _a << std::endl;
        return 0;
    };
};

int table_func(lua_State *L) {
    std::cout << "table_func" << std::endl;
    return 0;
}

int index_func(lua_State *L) {
    std::cout << "index_func: lua_gettop=" << lua_gettop(L) << std::endl;
    lua4tinker::enum_stack(L);
    return 0;
}
/*
    index 方法 栈枚举结果
        index_func: lua_gettop=2
        index(2) = cc
        index(1) = lua_type: 4
    
    settable 方法 栈枚举结果
        index_func: lua_gettop=3
        index(3, LUA_TSTRING) = 123
        index(2, LUA_TSTRING) = cc
        index(1, LUA_TTABLE)
*/

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

    lua_pushstring(L, "func");
    lua_pushcfunction(L, table_func);
    lua_settable(L, 1);

    // index tag method
    int index_tag = lua_newtag(L);
    lua_pushcfunction(L, index_func);
    lua_settagmethod(L, index_tag, "settable");

    lua_settag(L, index_tag);
    
    lua_setglobal(L, "student");

    int ret =  lua4tinker::do_string(L, R"(
        print ("Hi~")
        print (student['age'])
        print (student['name'])
        student:func()
        print ("settable: ")
        student['cc'] = "123"
        print ("index: ")
        cc = student['cc']

        local tm = gettagmethod(tag(student), "settable")
        print (tm)
    )");

    return ret;
}
