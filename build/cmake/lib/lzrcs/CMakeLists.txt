#--------------------------------------------------
# link include file and lib file directory
#--------------------------------------------------
include_directories(${LZRC_SRC_DIR}/lib/encode ${LZRC_SRC_DIR}/lib/datatypedef ${LZRC_SRC_DIR}/lib/decode ${LZRC_SRC_DIR}/lib/common)

#--------------------------------------------------
# set the source file of library
#--------------------------------------------------
set(ENCODE_SRC ${LZRC_SRC_DIR}/lib/encode/rangecode_encode.c ${LZRC_SRC_DIR}/lib/encode/lzrc_encode.c)

set(DECODE_SRC ${LZRC_SRC_DIR}/lib/decode/lzrc_stream_decode.c ${LZRC_SRC_DIR}/lib/decode/rangecode_decode.c)

#--------------------------------------------------
# generate the library
#--------------------------------------------------
add_library(${ENCODE_LZRC_LIB_NAME} STATIC ${ENCODE_SRC})
target_link_libraries(${ENCODE_LZRC_LIB_NAME} ${COMMON_LZRC_LIB_NAME} ${HASH_TREE_LZRC_LIB_NAME})
add_library(${DECODE_LZRC_STREAM_LIB_NAME} STATIC ${DECODE_SRC})
target_link_libraries(${DECODE_LZRC_STREAM_LIB_NAME} ${COMMON_LZRC_LIB_NAME})
