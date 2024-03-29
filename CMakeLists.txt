cmake_minimum_required(VERSION 3.11)
project(mvs)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
include(FetchContent)

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
)
FetchContent_MakeAvailable(json)
link_libraries(nlohmann_json::nlohmann_json)

add_library(graph STATIC
  common.cpp
  cluster.cpp
  dfg.cpp
  graph.cpp
  io.cpp
  mvs.cpp
  vs.cpp
)
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
add_test(NAME mvs_crypt_1 COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_crypt_Transform_entry.45.txt
  1 1 64)
add_test(NAME mvs_crypt_2 COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_crypt_Transform_entry.45.txt
  2 2 14)
add_test(NAME mvs_hadamard_18 COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_hadamard_HadamardSAD8x8_for.body.1.txt
  18 18 1)
add_test(NAME mvs_hadamard_17 COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_hadamard_HadamardSAD8x8_for.body.1.txt
  17 17 1)
add_test(NAME mvs_hadamard_16 COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_hadamard_HadamardSAD8x8_for.body.1.txt
  16 16 1)
add_test(NAME mvs_hadamard_15 COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_hadamard_HadamardSAD8x8_for.body.1.txt
  15 15 16)
add_test(NAME mvs_hadamard_14 COMMAND test_mvs
  ${CMAKE_SOURCE_DIR}/data/DFG_hadamard_HadamardSAD8x8_for.body.1.txt
  14 14 8)
