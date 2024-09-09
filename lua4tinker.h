#ifndef __LUA4TINKER_H
#define __LUA4TINKER_H

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#include <optional>
#include <cassert>
#include <concepts>
#include <typeinfo>

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

    struct stack_delay_pop {
        lua_State *L;
        int pop_num;
        stack_delay_pop(lua_State *_L, int _pop_num) : L(_L), pop_num(_pop_num) {}
        ~stack_delay_pop() { lua_pop(L, pop_num); }
    };

    // Helper Function End

    template <typename T>
    struct stack_helper {
        static T read(lua_State *L) {
            return std::make_optional<T>;
        }
        static void push(lua_State *L, T && val) {
            std::cout << typeid(val).name() << std::endl;
            assert(!"尚未处理的push");
        }
    };

    template <>
    struct stack_helper<std::string> {
        static void push(lua_State *L, const std::string & val) {
            lua_pushlstring(L, val.c_str(), val.size());
        }

        static std::string read(lua_State *L) {
            if (lua_type(L, 1) == LUA_TSTRING) {
                const char *str = lua_tostring(L, 1);
                size_t str_size = lua_strlen(L, 1);
                return std::string(str, str_size);
            }
            return "";
        }
    };

    template <typename T>
    concept LuaNumber = std::is_integral_v<T> || std::is_floating_point_v<T>;
    // 试试使用requires语法
    // requires std::integral<T> || std::floating_point<T>
    template <LuaNumber T>
    struct stack_helper<T> {
        static void push(lua_State *L, T val) {
            lua_pushnumber(L, val);
        }
        static T read(lua_State *L) {
            if (lua_type(L, 1) == LUA_TNUMBER) {    // lua4中number包含数值和字符串型的数值
                return (T)lua_tonumber(L, 1);
            } else {
                assert(!"堆栈中不是number");
            } 
            return 0; 
        }
    };

    // template <typename T>
    // void push(lua_State *L, T && val) {
    //     stack_helper<T>::push(L, std::forward<T>(val));
    // }

    // template <typename T>
    // T read(lua_State *L) {
    //     return stack_helper<T>::read(L);
    // }

    template <typename T>
    void set(lua_State* L, const char* name, T&& object) {
        stack_helper<std::decay_t<T>>::push(L, std::forward<T>(object));
        lua_setglobal(L, name);
    }

    template <typename T>
    T get(lua_State *L, const char* name) {
        lua_getglobal(L, name);
        stack_delay_pop tmp(L, 1);
        return stack_helper<T>::read(L);
    }
}

#endif