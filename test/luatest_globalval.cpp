#include <iostream>
#include "lua4tinker.h"

int main() {
    lua_State * L = lua4tinker::new_state();
    lua4tinker::open_libs(L);

    int int_val = 233;
    float float_val = 233.233;
    std::string string_val = "Hello, this is lua4tinker!";

    lua4tinker::set(L, "int_val", int_val);
    lua4tinker::set(L, "float_val", float_val);
    lua4tinker::set(L, "string_val", string_val);
    
    std::string code = R"(
        print(int_val)
        print(float_val)
        print(string_val)
        int_val2 = int_val
        float_val2 = float_val
        string_val2 = string_val
    )";

    int ret = lua4tinker::do_string(L, code);

    if (!ret) {
        int int_val2 = lua4tinker::get<int>(L, "int_val2");
        float float_val2 = lua4tinker::get<float>(L, "float_val2");
        std::string string_val2 = lua4tinker::get<std::string>(L, "string_val2");

        if (
                int_val2 != int_val ||
                float_val2 != float_val ||
                string_val2 != string_val
            ) 
        {
                return 1;
        }

    }
    return ret;
}