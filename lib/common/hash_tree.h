#ifndef HASH_TREE_H
#define HASH_TREE_H
#include "lzrc_data_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HASH_VALUE_SIZE 32 // must be even

typedef struct HashTreeNode {
    U32 key;
    U32 value[HASH_VALUE_SIZE];
    S32 depth;       // 树的深度
    U32 runCodeFlag; // 如果是1，则该节点是游程编码，只记录HASH_VALUE_SIZE * 2的大小
    struct HashTreeNode *smaller;
    struct HashTreeNode *bigger;
} LZRC_HashNode;

void InitHashNode(LZRC_HashNode *node);

LZRC_HashNode *AddNode(U32 key, U32 value, LZRC_HashNode *root);

LZRC_HashNode *AddRunCodeNode(U32 key, U32 value, U32 length, LZRC_HashNode *root);

LZRC_HashNode *GetNode(U32 key, LZRC_HashNode *root);

U32 FreeHashTree(LZRC_HashNode *root);

#ifdef __cplusplus
}
#endif
#endif
