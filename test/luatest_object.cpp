#include <iostream>
#include <string>
#include "lua4tinker.h"

using std::cout;
using std::endl;
using std::string;

class A
{
public:
    A(int a) : _a(a) {}
    int _a;
    string _str;
    float _flt;

    double sum(int a, float f) {
        return (a + f + _a + _flt);
    }

    void dump() {
        cout << "a: " << _a << endl;
    }
};

int main() {
    lua_State * L = lua4tinker::new_state();
    lua4tinker::open_libs(L);

    lua4tinker::do_string(L, R"(
        function call_by_cpp(a_object, int_val, str_val, float_val)
            a_object['_a'] = int_val
            a_object['_str'] = str_val
            a_object['_flt'] = float_val
            a_object['_not_exist'] = 233
            print (a_object['_a'])
            print (a_object['_str'])
            print (a_object['_flt'])
            print ("a_object:sum: "..a_object:sum(int_val, float_val))
            -- print (a_object(int_val))
            return a_object['_a']+a_object['_a']
        end
    )");

    A a_object(2);
    lua4tinker::class_object<A>(L, "a_object", &a_object);
    lua4tinker::class_object_mem<A>(L, &a_object, "a_object", &A::_a, "_a");
    lua4tinker::class_object_mem<A>(L, &a_object, "a_object", &A::_str, "_str");
    lua4tinker::class_object_mem<A>(L, &a_object, "a_object", &A::_flt, "_flt");
    lua4tinker::class_object_func<A>(L, &a_object, "a_object", &A::sum, "sum");

    int int_val = 123;
    if (lua4tinker::call<int>(L, "call_by_cpp", lua4tinker::LuaClass("a_object"), int_val, string("Hello, Object!"), 233.233) != int_val*2) {
        return 1;
    }
    a_object.dump();
    return 0;
}

