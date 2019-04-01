cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 14)

find_package(nlohmann_json REQUIRED)
link_libraries(nlohmann_json::nlohmann_json)

add_library(graph STATIC common.cpp cluster.cpp dfg.cpp graph.cpp mvs.cpp vs.cpp)
target_compile_options(graph PRIVATE -Wall -Wextra -Wno-sign-compare -Wno-unused-function)
add_executable(config_info config_info.cpp)
target_link_libraries(config_info graph)
add_executable(mis mis-main.cpp)
target_link_libraries(mis graph)
add_executable(mvs mvs-main.cpp)
target_link_libraries(mvs graph)
add_executable(vs vs-main.cpp)
target_link_libraries(vs graph)
add_executable(test_intset test_intset.cpp)
add_executable(test_dfs test_dfs.cpp)
target_link_libraries(test_dfs graph)
add_executable(test_mis test_mis.cpp)
target_link_libraries(test_mis graph)
add_executable(test_mvs test_mvs.cpp)
target_link_libraries(test_mvs graph)
enable_testing()
add_test(NAME intset COMMAND test_intset)
add_test(NAME dfs COMMAND test_dfs)
add_test(NAME mis COMMAND test_mis)
add_test(NAME mvs COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_crypt_Transform_entry.45.txt
  ${CMAKE_SOURCE_DIR}/data/DFG_hadamard_HadamardSAD8x8_for.body.1.txt)