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
#include <string>
#include <iostream>

namespace lua4tinker {

    // 用于获取lua_pushcclosure附带的upvalue索引, index为想要获取的upvalue索引，n为upvalues的数量
    #define lua_upvalueindex(index, n) (index-(n+1))

    // 补充 error codes for lua_do*
    #define LUA_OK  0

    lua_State *new_state(size_t size = 8192) {
        return lua_open (size);
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
            }else {
                std::cerr << "堆栈中不是字符串, 请检查参数类型是否正确" << std::endl;
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
                std::cerr << "堆栈中不是number, 请检查参数类型是否正确" << std::endl;
            } 
            return (T)0; 
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

    // lua 调用 cpp 函数
    template<int32_t nIdxParams, typename RVal, typename Func, typename... Args, std::size_t... index>
    RVal direct_invoke_invoke_helper(Func&& func, lua_State* L, std::index_sequence<index...>)
    {
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
                direct_invoke_func<1, RVal, Func, Args...>(std::forward<Func>(func), L);
            }
            return 0;
        }

        // template <typename T>
        static int gc(lua_State *L) {
            auto pThis = (FuncWarpType*)lua_touserdata(L, lua_upvalueindex(1, 1));
            pThis->~FuncWarpType();
            std::cout << "gc 调用析构函数" << std::endl;
            return 0;
        }
    };

    template <typename R, typename... Args>
    void push_functor(lua_State *L, const char *name, R(func)(Args...)) {
        // 创建用户数据，将函数包裹器置入lua内存（以便利用lua的垃圾回收析构Functor对象）
        using Functor_wrap = functor<R, Args...>;
        new (lua_newuserdata(L, sizeof(Functor_wrap))) Functor_wrap(name, func);    // 使用lua的内存存放函数包裹器

        // 当lua4垃圾回收时回调
        int tag = lua_newtag(L);
        lua_pushcfunction(L, &Functor_wrap::gc);
        lua_settagmethod(L, tag, "gc");

        lua_pushcclosure(L, &Functor_wrap::invoke, 1);
    }

    // cpp 调用 lua 函数

    void push_args(lua_State* L) {};
    template<typename T, typename... Args>
    void push_args(lua_State* L, T&& parm)
    {
        push(L, std::forward<T>(parm));
    }
    template<typename T, typename... Args>
    void push_args(lua_State* L, T&& parm, Args&&... args)
    {
        push(L, std::forward<T>(parm));
        push_args<Args...>(L, std::forward<Args>(args)...);
    }

    // pop a value from lua stack
    template<typename T>
    struct pop
    {
        static constexpr const int32_t nresult = 1;

        static T apply(lua_State* L)
        {
            stack_delay_pop _dealy(L, nresult);
            return read<T>(L, -1);
        }
    };

    template <typename RVal, typename... Args>
    RVal call_stackfunc(lua_State *L, Args&&... args) {
        push_args(L, std::forward<Args>(args)...);
        int ret = LUA_OK;
        if ((ret = lua_call(L, sizeof...(args), pop<RVal>::nresult)) != LUA_OK) {
            std::cerr << "调用lua4函数失败: " << ret << std::endl;
            lua_pushnil(L);
        }
        return pop<RVal>::apply(L);
    }

    
    // 使用接口
    template <typename Func>
    void def(lua_State *L, const char *name, Func &&func) {
        push_functor(L, name, std::forward<Func>(func));
        lua_setglobal(L, name);
    }

    template <typename RVal, typename... Args>
    RVal call(lua_State *L, const char *name, Args &&... args) {
        lua_getglobal(L, name);
        if (lua_isfunction(L, -1) == false) {
            lua_pop(L, 1);
            std::cerr << "调用lua4函数失败: " << name << "不存在 (或者不是一个函数)" << std::endl;
            return RVal();  // 暂时用这个代替 等需要多返回值的时候再修改
        } else {
            return call_stackfunc<RVal>(L, std::forward<Args>(args)...);
        }
    }

    template <typename T>
    void set(lua_State* L, const char* name, T && val) {
        push<std::decay_t<T>>(L, std::forward<std::decay_t<T>>(val));
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