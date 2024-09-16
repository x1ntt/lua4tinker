# 简介

目前使用vscode在linux上开发，使用gcc编译器，目前clang也可，最终希望运行在vs2022上(cl编译器)

克隆项目后 尝试cmake即可，其中包含一些测试代码

因为还在开发，所以有时候可能编译不通过

## 目前实现的功能

+ 定义全局变量到lua4 和 获取全局变量

```cpp
lua4tinker::set(L, "int_val", int_val);
int int_val2 = lua4tinker::get<int>(L, "int_val2");
```

+ 定义全局函数

```cpp
lua4tinker::def(L, "cpp_sum", sum);
```
