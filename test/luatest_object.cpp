#include <iostream>
#include "lua4tinker.h"

using std::cout;
using std::endl;

class A
{
public:
    A(int _a) : a(_a) {}
    int a;
};

int sum(int a) {
    // lua4tinker::enum_stack();
    cout << "summmm: " << a << endl;
    return (a);
}

int main() {
    lua_State * L = lua4tinker::new_state();
    lua4tinker::open_libs(L);

    lua4tinker::def(L, "sum", sum);

    lua4tinker::do_string(L, R"(
        function call_by_cpp(a_object)
            a_object['a'] = 123
            print (a_object['a'])
            return 0
        end
    )");

    A a(2);
    lua4tinker::class_object<A>(L, "a", &a);
    lua4tinker::class_object_mem<A>(L, "a", "a", &A::a);

    if (lua4tinker::call<int>(L, "call_by_cpp", lua4tinker::LuaClass("a")) != 0) {
        return 1;
    }
}

/*
# 传递对象
    + class_object(L, "class_obj", &class_obj);  // 创建table
    + class_object_mem(L, &class_obj, &class_obj.a1); // 填充table的key，

    首先 class_object_def() 创建一个table以对象为名，设定好index和settable，然后class_object_mem_def() 填充table的key
        class_object_mem_def() 将成员指针和对象指针放入一个mem_warp中在newuserdata中初始化，并且将userdata放入table中
    当Lua侧访问这个成员时，会触发到index方法，此时该方法中可以通过栈获取到table和成员名，根据这个获取到mem_warp，
    将对象指针和成员指针取出来，然后调用对应mem_warp的get方法，获取到成员值

    lua4tinker::call() 的时候，参数应该是上面定义的table，push中的实际逻辑应该是根据对象名把对应table push到栈顶。
    具体实现，通过类LuaClass实现模板特化，stack_helper中实现lua_getglobal()
*/