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
#include <functional>

namespace lua4tinker {

    // 用于获取lua_pushcclosure附带的upvalue索引, index为想要获取的upvalue索引，n为upvalues的数量
    #define lua_upvalueindex(index, n) (index-(n+1))

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
        static void push(lua_State *L, T && val) {
            std::cout << typeid(val).name() << std::endl;
            assert(!"尚未处理的push");
        }
        static T read(lua_State *L, size_t index) {
            return std::make_optional<T>;
        }
    };

    template <>
    struct stack_helper<std::string> {
        static void push(lua_State *L, const std::string & val) {
            lua_pushlstring(L, val.c_str(), val.size());
        }

        static std::string read(lua_State *L, size_t index) {
            if (lua_type(L, index) == LUA_TSTRING) {
                const char *str = lua_tostring(L, index);
                size_t str_size = lua_strlen(L, index);
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
        static T read(lua_State *L, size_t index) {
            if (lua_type(L, index) == LUA_TNUMBER) {    // lua4中number包含数值和字符串型的数值
                return (T)lua_tonumber(L, index);
            } else {
                assert(!"堆栈中不是number");
            } 
            return 0; 
        }
    };

    template <typename T>
    void push(lua_State *L, T &&val) {
        stack_helper<T>::push(L, std::forward<T>(val));
    }

    template <typename T>
    T read(lua_State *L, size_t index) {
        return stack_helper<T>::read(L, index);
    }

    // Function

    template<int32_t nIdxParams, typename RVal, typename Func, typename... Args, std::size_t... index>
    RVal direct_invoke_invoke_helper(Func&& func, lua_State* L, std::index_sequence<index...>)
    {
        // return stdext::invoke(std::forward<Func>(func), read<Args>(L, index + nIdxParams)...);
        return func(read<Args>(L, index + nIdxParams)...);
    }
    
    template<int32_t nIdxParams, typename RVal, typename Func, typename... Args>
    RVal direct_invoke_func(Func&& func, lua_State* L)
    {
        return direct_invoke_invoke_helper<nIdxParams, RVal, Func, Args...>(std::forward<Func>(func),
            L,
            std::make_index_sequence<sizeof...(Args)>{});
    }

    struct functor_base {
        virtual ~functor_base() {}
        virtual int apply(lua_State *L) = 0;
    };

    template <typename RVal, typename... Args>
    struct functor : functor_base {
        typedef std::function<RVal(Args...)> FunctionType;
        using FuncWarpType = functor<RVal, Args...>;
        FunctionType       func;
        std::string        name;

        functor(const char *_name, FunctionType &&_func) : name(_name), func(_func) { };

        int apply(lua_State *L) {
            return invoke_function<RVal, FunctionType>(L, std::forward<FunctionType>(func));
        }

        static int invoke(lua_State *L) {
            // 这里会被注册给lua，从这里取出当前的函数包裹器，并获取参数并调用
            auto pThis = (FuncWarpType*)lua_touserdata(L, lua_upvalueindex(1, 1));
            if (pThis == nullptr) {
                assert(!"拿取用户数据失败");
                return 0;
            }
            return pThis->apply(L);
        }

        template <typename R, typename Func>
        static int invoke_function(lua_State *L, Func &&func) {
            if constexpr (!std::is_void_v<R>) { // 函数有返回值，需要将返回值压栈
                push<RVal>(L, direct_invoke_func<1, RVal, Func, Args...>(std::forward<Func>(func), L));
                return 1;
            }else {
                
            }
            return 0;
        }
    };

    template <typename R, typename... Args>
    void push_functor(lua_State *L, const char *name, R(func)(Args...)) {
        // 创建用户数据，将函数包裹器置入lua内存（以便利用lua的垃圾回收析构Functor对象）
        using Functor_wrap = functor<R, Args...>;
        new (lua_newuserdata(L, sizeof(Functor_wrap))) Functor_wrap(name, func);    // 使用lua的内存存放函数包裹器
        
        lua_pushcclosure(L, &Functor_wrap::invoke, 1);
    }
    
    template <typename Func>
    void def(lua_State *L, const char *name, Func &&func) {
        push_functor(L, name, std::forward<Func>(func));
        lua_setglobal(L, name);
    }

    template <typename T>
    void set(lua_State* L, const char* name, T&& val) {
        // stack_helper<std::decay_t<T>>::push(L, std::forward<T>(object));
        push<std::decay_t<T>>(L, std::forward<T>(val));
        lua_setglobal(L, name);
    }

    template <typename T>
    T get(lua_State *L, const char* name) {
        lua_getglobal(L, name);
        stack_delay_pop tmp(L, 1);
        return read<T>(L, 1);
    }
}

#endif