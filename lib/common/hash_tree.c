#include "hash_tree.h"

void InitHashNode(LZRC_HashNode *node)
{
    node->smaller = NULL;
    node->bigger = NULL;
    node->depth = 0;
    node->key = 0;
    node->runCodeFlag = 0; // 默认非游程编码
    S32 loop = 0;
    for (; loop < HASH_VALUE_SIZE; loop += 2) {
        *(U64 *)(node->value + loop) = 0xFFFFFFFFFFFFFFFF;
    }
}

#define GET_NODE_DEPTH(node) ((node) == NULL? -1 : (node)->depth)

S32 RefreshDept(LZRC_HashNode *node)
{
    S32 smallerDepth = GET_NODE_DEPTH(node->smaller);
    S32 biggerDepth = GET_NODE_DEPTH(node->bigger);
    node->depth = (smallerDepth > biggerDepth ? smallerDepth : biggerDepth) + 1;
    return node->depth;
}

// 左旋
LZRC_HashNode *LeftRotation(LZRC_HashNode *oldRoot)
{
    LZRC_HashNode *newRoot = oldRoot->smaller;
    oldRoot->smaller = newRoot->bigger;
    newRoot->bigger = oldRoot;
    RefreshDept(oldRoot);
    RefreshDept(newRoot);
    return newRoot;
}

// 右旋
LZRC_HashNode *RightRotation(LZRC_HashNode *oldRoot)
{
    LZRC_HashNode *newRoot = oldRoot->bigger;
    oldRoot->bigger = newRoot->smaller;
    newRoot->smaller = oldRoot;
    RefreshDept(oldRoot);
    RefreshDept(newRoot);
    return newRoot;
}

// 左右旋
LZRC_HashNode *LeftRightRotation(LZRC_HashNode *oldRoot)
{
    oldRoot->smaller = RightRotation(oldRoot->smaller);
    return LeftRotation(oldRoot);
}

// 右边左节点旋转
LZRC_HashNode *RightLeftRotation(LZRC_HashNode *oldRoot)
{
    oldRoot->bigger = LeftRotation(oldRoot->bigger);
    return RightRotation(oldRoot);
}

LZRC_HashNode *AfterAddNode(U32 key, LZRC_HashNode *root)
{
    S32 oldDepth = root->depth;
    RefreshDept(root);
    if (oldDepth == root->depth) {
        return NULL;
    } else {
        // Rotation Time comes
        S32 depthDiff = GET_NODE_DEPTH(root->smaller) - GET_NODE_DEPTH(root->bigger);
        if (depthDiff == 2) {
            if (key > root->smaller->key) {
                return LeftRightRotation(root);
            } else {
                return LeftRotation(root);
            }
        } else if (depthDiff == -2) {
            if (key < root->bigger->key) {
                return RightLeftRotation(root);
            } else {
                return RightRotation(root);
            }
        }
        return root;
    }
}

// step 添加后向后滑动的步长，用于游程编码
LZRC_HashNode *AddNode(U32 key, U32 value, LZRC_HashNode *root)
{
    if (root == NULL) {
        LZRC_HashNode *node = (LZRC_HashNode *)malloc(sizeof(LZRC_HashNode));
        if (node == NULL) {
            return NULL;
        }

        InitHashNode(node);
        node->key = key;
        node->value[0] = value;
        return node;
    }
    if (key == root->key) {
        if ((value < root->value[0]) || (value - root->value[0] <= 2)) {
            return NULL; // 如果临近举例很小，放弃掉
        }
        BYTE loop = HASH_VALUE_SIZE - 1;
        for (; loop != 0; loop--) {
            root->value[loop] = root->value[loop - 1];
        }
        root->value[0] = value;
        return NULL;
    }
    LZRC_HashNode *addedNode = NULL;
    if (key < root->key) {
        addedNode = AddNode(key, value, root->smaller);
        // NULL = not add new New Node
        if (addedNode == NULL) {
            return NULL;
        }
        root->smaller = addedNode;
    } else {
        addedNode = AddNode(key, value, root->bigger);
        // NULL = not add new New Node
        if (addedNode == NULL) {
            return NULL;
        }
        root->bigger = addedNode;
    }
    return AfterAddNode(key, root);
}

// add run code Node
LZRC_HashNode *AddRunCodeNode(U32 key, U32 value, U32 length, LZRC_HashNode *root)
{
    if (root == NULL) {
        LZRC_HashNode *node = (LZRC_HashNode *)malloc(sizeof(LZRC_HashNode));
        if (node == NULL) {
            return NULL;
        }
        InitHashNode(node);
        node->key = key;
        node->value[0] = value;
        node->value[1] = length;
        node->runCodeFlag = 1;
        return node;
    }
    if (key == root->key) {
        if ((value < root->value[0]) || (value - root->value[0] <= 2)) {
            return NULL; // 如果临近举例很小，放弃掉
        }
        BYTE loop = HASH_VALUE_SIZE - 1;
        for (; loop !=1; loop--) {
            root->value[loop] = root->value[loop - 2];
        }
        root->value[0] = value;
        root->value[1] = length;
        return NULL;
    }
    LZRC_HashNode *addedNode = NULL;
    if (key < root->key) {
        addedNode = AddRunCodeNode(key, value, length, root->smaller);
        if (addedNode == NULL) {
            return NULL;
        }
        root->smaller = addedNode;
    } else {
        addedNode = AddRunCodeNode(key, value, length, root->bigger);
        if (addedNode == NULL) {
            return NULL;
        }
        root->bigger = addedNode;
    }
    return AfterAddNode(key, root);
}

LZRC_HashNode *GetNode(U32 key, LZRC_HashNode *root)
{
    LZRC_HashNode *currentNode = root;
    while (1) {
        if (currentNode == NULL) {
            return NULL;
        }

        if (key == currentNode->key) {
            currentNode = currentNode->bigger;
        } else {
            currentNode = currentNode->smaller;
        }
    }
}

U32 FreeHashTree(LZRC_HashNode *root)
{
    U32 deleteTotal = 0;
    if (root != NULL) {
        ++deleteTotal;
        deleteTotal += FreeHashTree(root->bigger);
        root->bigger = NULL;

        free(root);
    }
    return deleteTotal;
}
