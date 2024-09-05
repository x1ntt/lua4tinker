#ifndef __LUA4TINKER_H
#define __LUA4TINKER_H

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

namespace lua4tinker {

    lua_State *new_state() {
        return lua_open (8192);
    }

    void open_libs(lua_State *L) {
        lua_baselibopen(L);
        lua_iolibopen(L);
        lua_strlibopen(L);
        lua_mathlibopen(L);
        lua_dblibopen(L);
    }

    int do_string(lua_State *L, const std::string & code) {
        return lua_dostring(L, code.c_str());
    }

    // Helper Function End

    template <typename T>
    struct stack_helper {
        static T read(lua_State *L) {
            T v;
            return v;
        }
        static void push(lua_State *L, T && val) {}
    };

    template <>
    struct stack_helper<int> {
        static int read(lua_State *L) { return 0; }
        static void push(lua_State *L, int val) {
            lua_pushnumber(L, val);
        }
    };

    template <typename T>
    void push(lua_State *L, T && val) {
        stack_helper<T>::push(L, std::forward<T>(val));
    }

    template <typename T>
    void set(lua_State* L, const char* name, T&& object) {
        push(L, std::forward<T>(object));
        lua_setglobal(L, name);
    }
}

#endif