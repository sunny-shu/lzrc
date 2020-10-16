#--------------------------------------------------
# link include file and lib file directory
#--------------------------------------------------
include_directories(${LZRC_BASE_DIR}/lib/common ${LZRC_BASE_DIR}/lib/datatypedef)

#--------------------------------------------------
# set the source file of library
#--------------------------------------------------
set(COMMON_LZRC_LIB_SRC ${LZRC_BASE_DIR}/lib/common/rangecode_common.c ${LZRC_BASE_DIR}/lib/common/lzrc_common.c)
set(HASH_TREE_LZRC_LIB_SRC ${LZRC_BASE_DIR}/lib/common/hash_tree.c)

#--------------------------------------------------
# generate the library
#--------------------------------------------------
add_library(${COMMON_LZRC_LIB_NAME} STATIC ${COMMON_LZRC_SRC})
add_library(${HASH_TREE_LZRC_LIB_NAME} STATIC ${HASH_TREE_LZRC_LIB_SRC})
