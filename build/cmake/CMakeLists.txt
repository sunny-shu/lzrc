cmake_minimum_required(VERSION 2.6)

PROJECT(main)

# value range: {Debug\Release\"" }
set(CMAKE_BUILD_TYPE Debug)

#--------------------------------------------------
# set global variable
#--------------------------------------------------
set(LZRC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(LZRC_TEST_DIR "${LZRC_SRC_DIR}/tests")

#--------------------------------------------------
# options
#--------------------------------------------------
option(BUILD_TESTS "BUILD TESTS" ON)             #启动测试开关，默认打开
option(CRC_CHECK "CRC CHECK" ON)                 #启动CRC校验开关，默认关闭

if(CRC_CHECK)
    message(STATUS "CRC check")
    add_definitions(-DCRC_CHECK_ON)
else()
    message(STATUS "no CRC check")
endif()

add_definitions(-Wall)

#--------------------------------------------------
# set lib name
#--------------------------------------------------
set(ENCODE_LZRC_LIB_NAME encode_lzrc)
set(DECODE_LZRC_STREAM_LIB_NAME decode_lzrc_stream)
set(COMMON_LZRC_LIB_NAME common_lzrc)
set(HASH_TREE_LZRC_LIB_NAME hash_tree_lzrc)

#--------------------------------------------------
# add sub dir
#--------------------------------------------------
add_subdirectory(lib/common)
add_subdirectory(lib/lzrcs)
