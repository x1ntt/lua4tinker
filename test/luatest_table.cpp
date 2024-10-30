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
    lua_settagmethod(L, index_tag, "index");

    lua_settag(L, index_tag);
    
    lua_setglobal(L, "student");

    int ret =  lua4tinker::do_string(L, R"(
        print ("Hi~")
        print (student['age'])
        print (student['name'])
        student:func()
        cc=student['cc']
    )");

    return ret;
}

/*
    定义成员变量：class_var_def(L, &object, object.var1);
    定义函数：class_func_def(L, &object, T::func);
*/