add_executable(luatest lua_test.cpp)    # 添加测试文件
target_link_libraries(luatest lua4lib)  # 链接测试目标与项目源文件

add_executable(luatest_globalval luatest_globalval.cpp)
target_link_libraries(luatest_globalval lua4lib)

add_executable(luatest_function luatest_function.cpp)
target_link_libraries(luatest_function lua4lib)

add_executable(luatest_table luatest_table.cpp)
target_link_libraries(luatest_table lua4lib)

add_executable(luatest_object luatest_object.cpp)
target_link_libraries(luatest_object lua4lib)

add_test(NAME luatest COMMAND luatest)
add_test(NAME luatest_globalval COMMAND luatest_globalval)
add_test(NAME luatest_function COMMAND luatest_function)
add_test(NAME luatest_table COMMAND luatest_table)
add_test(NAME luatest_object COMMAND luatest_object)