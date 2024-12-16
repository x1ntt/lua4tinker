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
#include <cxxabi.h>
#include <functional>
#include <string>
#include <iostream>

namespace lua4tinker {

    using std::cout;
    using std::endl;
    using std::cerr;

    // 简单日志实现
    #define LOG_I(frm, args...) do {printf("[info_%s_%s][%s:%d] ", __DATE__ ,__TIME__, __FILE__, __LINE__); printf(frm, ##args); printf("\n");}while(0);   // 普通消息
    #define LOG_W(frm, args...) do {printf("[warn_%s_%s][%s:%d] ", __DATE__ ,__TIME__, __FILE__, __LINE__); printf(frm, ##args); printf("\n");}while(0);   // 有错误但不影响程序运行
    #define LOG_A(frm, args...) do {printf("[abort_%s_%s][%s:%d] ", __DATE__ ,__TIME__ ,__FILE__, __LINE__); printf(frm, ##args); printf("\n");assert(false);}while(0);  // 程序无法再运行

    // 对象定义宏函数
    #define OBJECT_DEF(L, TYPE, OBJECT_NAME) lua4tinker::object_def<TYPE>(L, #OBJECT_NAME, &OBJECT_NAME);
    #define OBJECT_MEM_DEF(L, TYPE, OBJECT_NAME, MEM_NAME) lua4tinker::object_mem_def<TYPE>(L, &OBJECT_NAME, #OBJECT_NAME, &TYPE::MEM_NAME, #MEM_NAME);
    #define OBJECT_FUNC_DEF(L, TYPE, OBJECT_NAME, FUNC_NAME) lua4tinker::object_func_def<TYPE>(L, &OBJECT_NAME, #OBJECT_NAME, &TYPE::FUNC_NAME, #FUNC_NAME);

    // 用于获取lua_pushcclosure附带的upvalue索引, index为想要获取的upvalue索引，n为upvalues的数量
    #define lua_upvalueindex(index, n) (index-(n+1))

    // 补充 error codes for lua_do*
    #define LUA_OK  0

    int luaclass_tag = -1;

    lua_State *new_state(size_t size = 8192) {
        lua_State *L = lua_open (size);
        luaclass_tag = lua_newtag(L);
        return L;
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

    void dump_gc(lua_State *L) {
        LOG_I("gc status (count/threshold): %d/%d", lua_getgccount(L), lua_getgcthreshold(L));
    }

    struct stack_delay_pop {
        lua_State *L;
        int pop_num;
        stack_delay_pop(lua_State *_L, int _pop_num) : L(_L), pop_num(_pop_num) {}
        ~stack_delay_pop() { lua_pop(L, pop_num); }
    };

    struct stack_scope_exit {
        lua_State* L;
        size_t    nTop;
        explicit stack_scope_exit(lua_State* _L) : L(_L) {
            nTop = lua_gettop(L);
        }
        ~stack_scope_exit() { 
            lua_settop(L, nTop); 
        }
    };

    void enum_stack(lua_State *L) {
        int stack_count = lua_gettop(L);
        LOG_I("------------ enum_stack start, lua_gettop=%d ------------", stack_count);
        for (; stack_count>0; stack_count--) {
            int lua_type_result = lua_type(L, stack_count);
            if (lua_type_result == LUA_TNUMBER) {
                LOG_I("index(%d, LUA_TNUMBER) = %f", stack_count, lua_tonumber(L, stack_count));
            } else if (lua_type_result == LUA_TSTRING) {
                LOG_I("index(%d, LUA_TSTRING) = %s", stack_count, lua_tostring(L, stack_count));
            } else if (lua_type_result == LUA_TTABLE) {
                LOG_I("index(%d, LUA_TTABLE)", stack_count);
            } else if (lua_type_result == LUA_TNIL) {
                LOG_I("index(%d, LUA_TNIL)", stack_count);
            } else if (lua_type_result == LUA_TUSERDATA) {
                LOG_I("index(%d, LUA_TUSERDATA) = %p", stack_count, (void*)lua_touserdata(L, stack_count));
            } else if (lua_type_result == LUA_TFUNCTION) {
                LOG_I("index(%d, LUA_TFUNCTION)", stack_count);
            } else {
                LOG_I("index(%d) = lua_type: %d", stack_count, lua_type_result);
            }
        }
        LOG_I("------------ enum_stack end ------------");
    }

    // Helper Function End

    template <typename T>
    struct stack_helper {
        static void push(lua_State *L, T && val) {
            std::cout << typeid(val).name() << std::endl;
            LOG_A("尚未处理的push");
        }
        static T read(lua_State *L, size_t index) {
            return std::make_optional<T>;
        }
    };

    class LuaClass {
    public:
        LuaClass(const char *object_name) : _object_name(object_name) {};
        const char *_object_name;
    };

    template <>
    struct stack_helper<LuaClass> {
        static void push(lua_State *L, LuaClass && val) {
            lua_getglobal(L, val._object_name);
            // cout << "push luaclass: " << val._object_name << endl;
            if (!lua_istable(L, -1)) {
                LOG_A("对象 %s 不存在, 需要预先定义对象", val._object_name);
            }
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
                std::cerr << "" << std::endl;
                LOG_A("堆栈中不是字符串, 请检查参数类型是否正确");
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
                LOG_A("堆栈中不是number, 请检查参数类型是否正确");
            } 
            return (T)0; 
        }
    };

    template <typename T>
    void push(lua_State *L, T &&val) {
        stack_helper<std::decay_t<T>>::push(L, std::forward<std::decay_t<T>>(val));
    }

    template <typename T>
    T read(lua_State *L, int index) {
        return stack_helper<std::decay_t<T>>::read(L, index);
    }

    // Function

    // lua 调用 cpp 函数
    template<int32_t nIdxParams, typename RET_T, typename Func, typename... Args, std::size_t... index>
    RET_T direct_invoke_invoke_helper(Func&& func, lua_State* L, std::index_sequence<index...>)
    {
        return func(read<Args>(L, index + nIdxParams)...);
    }
    
    template<int32_t nIdxParams, typename RET_T, typename Func, typename... Args>
    RET_T direct_invoke_func(Func&& func, lua_State* L)
    {
        return direct_invoke_invoke_helper<nIdxParams, RET_T, Func, Args...>(std::forward<Func>(func),
            L,
            std::make_index_sequence<sizeof...(Args)>{});
    }

    template<int32_t nIdxParams, typename RET_T, typename Func, typename CLASS_T, typename... Args, std::size_t... index>
    RET_T direct_invoke_invoke_helper(Func&& func, lua_State* L, CLASS_T* object_ptr, std::index_sequence<index...>)
    {
        return func(object_ptr, read<Args>(L, index + nIdxParams)...);
    }

    template<int32_t nIdxParams, typename RET_T, typename Func, typename CLASS_T, typename... Args>
    RET_T direct_invoke_member_func(Func&& func, lua_State* L, CLASS_T* object_ptr)
    {
        return direct_invoke_invoke_helper<nIdxParams, RET_T, Func, CLASS_T, Args...>(std::forward<Func>(func),
            L,
            object_ptr,
            std::make_index_sequence<sizeof...(Args)>{});
    }

    struct functor_base {
        virtual ~functor_base() {}
        virtual int apply(lua_State *L) = 0;
    };

    template <typename RET_T, typename... Args>
    struct functor : functor_base {
        typedef std::function<RET_T(Args...)> FunctionType;
        using FuncWarpType = functor<RET_T, Args...>;
        FunctionType       func;
        std::string        name;

        functor(const char *_name, FunctionType &&_func) : name(_name), func(_func) {
            // cout << "functor构造: " << (void*)this << endl;
        };
        ~functor() {
            // cout << "functor析构: " << (void*)this << endl;
        };

        int apply(lua_State *L) {
            return invoke_function<FunctionType>(L, std::forward<FunctionType>(func));
        }

        static int invoke(lua_State *L) {
            // 这里会被注册给lua，从这里取出当前的函数包裹器，并获取参数并调用
            auto pThis = (FuncWarpType*)lua_touserdata(L, lua_upvalueindex(1, 1));
            if (pThis == nullptr) {
                LOG_A("拿取userdata失败");
                return 0;
            }
            return pThis->apply(L);
        }

        template <typename Func>
        static int invoke_function(lua_State *L, Func &&func) {
            if constexpr (!std::is_void_v<RET_T>) { // 函数有返回值，需要将返回值压栈
                push<RET_T>(L, direct_invoke_func<1, RET_T, Func, Args...>(std::forward<Func>(func), L));
                return 1;
            }else {
                direct_invoke_func<1, RET_T, Func, Args...>(std::forward<Func>(func), L);
            }
            return 0;
        }

        static int gc(lua_State *L) {
            auto pThis = (FuncWarpType*)lua_touserdata(L, lua_upvalueindex(1, 1));
            pThis->~FuncWarpType();
            return 0;
        }

        static void setgc(lua_State *L) {
            static int gc_tag = lua_newtag(L);
            lua_pushcfunction(L, &FuncWarpType::gc);
            lua_settagmethod(L, gc_tag, "gc");
        }
    };

    template <typename CLASS_T, typename RET_T, typename... Args>
    struct mem_functor {
        using FuncWarpType = mem_functor<CLASS_T, RET_T, Args...>;
        typedef std::function<RET_T(CLASS_T*, Args...)> FunctionType;

        FunctionType _func;
        std::string _mem_func_name;
        CLASS_T *_object_ptr;
        mem_functor (FunctionType && func, const char *mem_func_name, CLASS_T *object_ptr) : _func(std::move(func)), _mem_func_name(mem_func_name), _object_ptr(object_ptr) {
            // cout << "mem_functor构造: " << (void*)this << endl;
        }
        ~mem_functor () {
            // cout << "mem_functor析构: " << (void*)this << endl;
        }
        virtual int32_t apply(lua_State* L) {
            if constexpr (std::is_void_v<RET_T>) {  // 无返回值
                direct_invoke_member_func<2, RET_T, FunctionType, CLASS_T, Args...>(std::forward<FunctionType>(_func), L, _object_ptr);
                return 0;
            } else {    // 有返回值
                push<RET_T>(L, direct_invoke_member_func<2, RET_T, FunctionType, CLASS_T, Args...>(std::forward<FunctionType>(_func), L, _object_ptr));
                return 1;
            }
        };

        static int invoke(lua_State *L) {
            auto pThis = (FuncWarpType*)lua_touserdata(L, lua_upvalueindex(1, 1));
            if (pThis == nullptr) {
                LOG_A("拿取userdata失败");
                return 0;
            }
            return pThis->apply(L);
        }

        static int gc(lua_State *L) {
            auto pThis = (FuncWarpType*)lua_touserdata(L, lua_upvalueindex(1, 1));
            pThis->~FuncWarpType();
            return 0;
        }

        static void setgc(lua_State *L) {
            static int gc_tag = lua_newtag(L);
            lua_pushcfunction(L, &FuncWarpType::gc);
            lua_settagmethod(L, gc_tag, "gc");
        }
    };

    template <typename R, typename... Args>
    void push_functor(lua_State *L, const char *name, R(func)(Args...)) {
        // 创建用户数据，将函数包裹器置入lua内存（以便利用lua的垃圾回收析构Functor对象）
        using Functor_wrap = functor<R, Args...>;
        new (lua_newuserdata(L, sizeof(Functor_wrap))) Functor_wrap(name, func);    // 使用lua的内存存放函数包裹器

        // 当lua4垃圾回收时回调
        Functor_wrap::setgc(L);

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

    template <typename RET_T, typename... Args>
    RET_T call_stackfunc(lua_State *L, Args&&... args) {
        push_args(L, std::forward<Args>(args)...);
        int ret = LUA_OK;
        if ((ret = lua_call(L, sizeof...(args), pop<RET_T>::nresult)) != LUA_OK) {
            LOG_W("调用lua4函数失败: %d", ret);
            lua_pushnil(L);
        }
        return pop<RET_T>::apply(L);
    }

    struct base_var {
        virtual void get(lua_State *L) = 0;
        virtual void set(lua_State *L) = 0;
    };

    template <typename CLASS_T, typename MEM_T>
    struct mem_var : base_var {
        mem_var(MEM_T CLASS_T::* val_ptr, CLASS_T *this_ptr) : _val_ptr(val_ptr), _this_ptr(this_ptr) {
            // cout << "[mem_var] 构造mem_var: " << (void*)this << ", _this_ptr: " << (void*)_this_ptr << endl;
        }
        CLASS_T *_this_ptr;             // 对象地址
        MEM_T CLASS_T::* _val_ptr;      // 成员变量指针

        void get(lua_State *L) {    // lua从cpp侧取数据
            push(L, _this_ptr->*(_val_ptr));
        };
        void set(lua_State *L) {
            _this_ptr->*(_val_ptr) = read<MEM_T>(L, 3);
        };
    };

    int mem_set_tag_func(lua_State *L) {
        // cout << "mem_set_tag_func" << endl;
        lua_pushvalue(L, 2);
        lua_rawget(L, 1);

        auto *mem_var_ptr = (base_var*)lua_touserdata(L, -1);
        if (mem_var_ptr) {
            mem_var_ptr->set(L);
        }else {
            LOG_W("未定义成员变量 %s 到lua", lua_tostring(L, 2));
        }
        return 0;
    }

    int mem_get_tag_func(lua_State *L) {
        // cout << "mem_get_tag_func" << endl;
        lua_pushvalue(L, 2);
        lua_rawget(L, 1);
        auto *mem_var_ptr = (base_var*)lua_touserdata(L, -1);
        if (mem_var_ptr) {
            mem_var_ptr->get(L);    // push 结果到栈上去
            return 1;
        }

        // 当表设置了gettable tag方法的时候，如果返回0，会导致表值中的方法调用失败（这里会给lua返回nil）
        // 所以这里返回1，以便lua可以拿到表值中的方法
        return 1;
    }
    
    // 使用接口
    template <typename Func>
    void def(lua_State *L, const char *name, Func &&func) {
        push_functor(L, name, std::forward<Func>(func));
        lua_setglobal(L, name);
    }

    template <typename RET_T, typename... Args>
    RET_T call(lua_State *L, const char *name, Args &&... args) {
        lua_getglobal(L, name);
        if (lua_isfunction(L, -1) == false) {
            lua_pop(L, 1);
            LOG_W("调用lua4函数失败, %s 不存在, 或者不是一个函数", name);
            return RET_T();  // 暂时用这个代替 等需要多返回值的时候再修改
        } else {
            return call_stackfunc<RET_T>(L, std::forward<Args>(args)...);
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

    template <typename T>
    void object_def(lua_State *L, const char *object_name, T *object_ptr) {
        // 创建一个表 需要调用者自行管理对象生命周期
        lua_newtable(L);

        lua_pushcfunction(L, mem_set_tag_func);
        lua_settagmethod(L, luaclass_tag, "settable");
        lua_settag(L, luaclass_tag);

        lua_pushcfunction(L, mem_get_tag_func);
        lua_settagmethod(L, luaclass_tag, "gettable");
        lua_settag(L, luaclass_tag);

        lua_setglobal(L, object_name);
    }

    template <typename CLASS_T, typename MEM_T>
    void object_mem_def(lua_State *L, CLASS_T *object_ptr, const char *object_name, MEM_T CLASS_T::*mem_var_ptr, const char *mem_var_name) {
        stack_scope_exit scope_exit(L);
        lua_getglobal(L, object_name);
        if (lua_type(L, 1) == LUA_TTABLE) {
            lua_pushstring(L, mem_var_name);
            new (lua_newuserdata(L, sizeof(mem_var<CLASS_T, MEM_T>))) mem_var<CLASS_T, MEM_T>(mem_var_ptr, object_ptr);
            enum_stack(L);
            lua_rawset(L, -3);
        }else {
            LOG_A("需要事先通过定义对象");
        }
    }

    template <typename CLASS_T, typename RET_T, typename... ARGS>
    void object_func_def(lua_State *L, CLASS_T *object_ptr, const char *object_name, RET_T (CLASS_T::*func)(ARGS...), const char *mem_func_name) {
        stack_scope_exit scope_exit(L);
        lua_getglobal(L, object_name);
        if (lua_type(L, 1) == LUA_TTABLE) {
            lua_pushstring(L, mem_func_name);
            using FuncWarpType = mem_functor<CLASS_T, RET_T, ARGS...>;
            new (lua_newuserdata(L, sizeof(FuncWarpType))) FuncWarpType(func, mem_func_name, object_ptr);
            FuncWarpType::setgc(L);
            lua_pushcclosure(L, &FuncWarpType::invoke, 1);
            lua_rawset(L, -3);
        }else {
            LOG_A("需要事先通过定义对象");
        }
    }
}

#endif