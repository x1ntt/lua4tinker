add_executable(luatest lua_test.cpp)    # 添加测试文件
target_link_libraries(luatest lua4lib)  # 链接测试目标与项目源文件

add_executable(luatest_setglobalval luatest_setglobalval.cpp)
target_link_libraries(luatest_setglobalval lua4lib)

add_executable(luatest_function luatest_function.cpp)
target_link_libraries(luatest_function lua4lib)

add_test(NAME luatest COMMAND luatest)
add_test(NAME luatest_setglobalval COMMAND luatest_setglobalval)
add_test(NAME luatest_function COMMAND luatest_function)