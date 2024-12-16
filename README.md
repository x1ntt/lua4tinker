# 简介

目前使用vscode在linux上开发，使用gcc编译器，目前clang也可，最终希望运行在vs2022上(cl编译器)

克隆项目后 尝试cmake即可，其中包含一些测试代码

因为还在开发，所以有时候可能编译不通过

## 目前实现的功能

+ 定义全局变量到lua4 和 获取全局变量 完整例子见`test/luatest_global.cpp`

```cpp
lua4tinker::set(L, "int_val", int_val);
int int_val2 = lua4tinker::get<int>(L, "int_val2");
```

+ 双向全局函数调用 完整例子见 `test/luatest_function.cpp`

```cpp
lua4tinker::def(L, "cpp_sum", sum); // lua4 调用 cpp 函数
lua4tinker::call<int>(L, "max", 1, 2);   // cpp 调用 lua4 函数
```

+ 传递cpp对象到lua 完整例子见 `test/luatest_object.cpp`

    cpp侧定义对象和成员变量

    ```cpp
    A a_object(2);
    lua4tinker::class_object<A>(L, "a_object", &a_object);
    lua4tinker::class_object_mem<A>(L, &a_object, "a_object", &A::_a, "_a");
    lua4tinker::class_object_mem<A>(L, &a_object, "a_object", &A::_str, "_str");
    lua4tinker::class_object_mem<A>(L, &a_object, "a_object", &A::_flt, "_flt");
    lua4tinker::class_object_func<A>(L, &a_object, "a_object", &A::sum, "sum");

    // 以下宏与上面代码等效
    // OBJECT_DEF(L, A, a_object);
    // OBJECT_MEM_DEF(L, A, a_object, _a);
    // OBJECT_MEM_DEF(L, A, a_object, _str);
    // OBJECT_MEM_DEF(L, A, a_object, _flt);
    // OBJECT_FUNC_DEF(L, A, a_object, sum);

    int int_val = 123;
    if (lua4tinker::call<int>(L, "call_by_cpp", lua4tinker::LuaClass("a_object"), int_val, string("Hello, Object!"), 233.233) != int_val*2) {
        return 1;
    }
    ```

    lua侧对成员变量进行修改和访问，并且调用cpp对象方法，同时传参

    ```lua
    function call_by_cpp(a_object, int_val, str_val, float_val)
        a_object['_a'] = int_val
        a_object['_str'] = str_val
        a_object['_flt'] = float_val
        a_object['_not_exist'] = 233
        print (a_object['_a'])
        print (a_object['_str'])
        print (a_object['_flt'])
        print ("a_object:sum: "..a_object:sum(int_val, float_val))
        return a_object['_a']+a_object['_a']
    end
    ```

## Todo

+ 针对变量边界的检查
+ 增加对象定义宏
