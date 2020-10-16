#include "lzrc_encode.h"
#include "hash_tree.h"

#define HASH_TOTAL_WIDTH 64
#define MAX_LOOK_LEN (0xFFFFFFFF)
#define MIN_MATCH_THRESHOLD 3
#define RESET_TRIAD(p)   (p).matchLen = 0; (p).unmatchLen = 0; (p).offset = 0; (p).unmatchData = NULL;

S32 LzrcVersionNumber(void)
{
    return LZRC_VERSION_NUMBER;
}

const char* LZRCVersionString(void)
{
    return LZRC_VERSION_STRING;
}

typedef struct Encode_Settings {
    BYTE minMatchLen;                   // 最小匹配长度
    BYTE bestMinMatchLen;               // 最佳匹配长度，用于向前试探
    BYTE hashNum;
    U32 hash_mask;                      // hash计算的掩码
    U32 (*hashFunc)(U32);               // 计算hash的函数指针
    U32 hashTableSize;
    S32 (*compFunc)(const BYTE *, const BYTE *);
    U32 maxOffset;                      // 最大的偏移量
    U32 hashValueMask;                  // hash掩码，不用压缩策略有不同的掩码
    U32 HashKeyMask;                    // 计算hashKey的掩码
    U32 HashMaskBitWidth;               // hash
    S32 (*checkRuncode)(const BYTE*);
} LZ77_Settings;

S32 CheckRuncode(const BYTE *data)
{
    U32 d = *(U32 *)data;
    return (d & 0X00FFFFFF) == (d >> 8);
}

S32 CheckRuncode3B(const BYTE *data)
{
    return data[0] == data[1] && data[1] == data[2];
}

typedef struct {
    U32 matchLen;
    U16 pos; // 偏移量
    U32 offset;
} LZ77_Match_Binary;

// 计算U32位整数的编码
static U32 CalcHash(U32 v)
{
    return v;
}

static U32 CalcHash3B(U32 v)
{
    return v & 0X00FFFFFF;
}

U64 GetHashValue(const BYTE *data, U64 hashTb[], LZ77_Settings *settings)
{
    U32 newHK = settings->hashFunc(*(U32*)data);
    return hashTb[newHK];
}

S32 CompareData(const BYTE *a, const BYTE *b)
{
    return 1;
}

// 3Byte情况下的字符数按比较
S32 CompareData3B(const BYTE *a, const BYTE *b)
{
    return 1;
}

LzrEncodeContex* ConstructLzrcEncodeContex()
{
    LzrEncodeContex* contex = (LzrEncodeContex*)malloc(sizeof(LzrEncodeContex));
    if (contex == NULL) {
        return NULL;
    }
    InitRangeCodeContex(&(contex->encodeContex));
    InitFreqTable(&(contex->literalFreqTable));
    InitFreqTable(&(contex->literalLenFreqTable));
    InitFreqTable(&(contex->matchLenFreqTable));
    InitFreqTable(&(contex->offSetHighBYTEFreqTable));
    InitFreqTable(&(contex->offSetLowFreqTable));
    return contex;
}

// 初始化编码设置
void InitEncodeSetting(LZ77_Settings *settings, ENCODE_STRATEGY strategy)
{
    if (strategy == LZ77_NORMAL) {
        settings->minMatchLen = 4;
        settings->hashFunc = CalcHash;
        settings->hashTableSize = 1 << 16;
        settings->compFunc = CompareData;
        settings->maxOffset = WINDOW_SIZE - 1;
        settings->hashValueMask = 0xFFFFFFFF;
        settings->hashMaskBitWidth = 32;
        settings->hashNum = HASH_TOTAL_WIDTH / settings->hashMaskBitWidth;
        settings->bestMinMatchLen = settings->minMatchLen * 2;
        settings->checkRuncode = CheckRuncode;
    } else if (strategy == LZ77_HIGHT_RATIO) {
        settings->minMatchLen = 3;
        settings->hashFunc = CalcHash3B;
        settings->hashTableSize = 1 << 24;
        settings->compFunc = CompareData3B;
        settings->maxOffset = WINDOW_SIZE - 1;
        settings->hashValueMask = 0xFFFFFFFF;
        settings->HashMaskBitWidth = 32;
        settings->hashNum = HASH_TOTAL_WIDTH / settings->HashMaskBitWidth;
        settings->bestMinMatchLen = settings->minMatchLen * 2;
        settings->checkRuncode = CheckRuncode3B;
    }
}

U32 GetMaxMatchLen(const BYTE *a, const BYTE *b, U32 startPos, U32 endPos)
{
    while ((*(a + startPos) == *(b + startPos)) && (startPos < endPos)) {
        startPos++;
    }
    return startPos;
}

// 压缩函数调用添加接口
LZRC_HashNode *AddNewHashkey(const BYTE *data, U32 value, LZRC_HashNode *root, LZ77_Settings *settings, U32 *step, U32 leftLen)
{
    U32 newHK = settings->hashFunc(*(U32*)data);
    LZRC_HashNode *newRoot = NULL;
    // all same . is run code
    if (settings->checkRuncode(data)) {
        U32 runcodeLen = settings->minMatchLen;
        while (data[runcodeLen] == data[0] && runcodeLen < leftLen) {
            ++runcodeLen;
        }
        newRoot = AddRunCodeNode(newHK, value, runcodeLen, root);
        *step = runcodeLen - settings->minMatchLen + 1;
    } else {
        newRoot = AddNode(newHK, value, root);
        *step = 1;
    }
    if (newRoot == NULL) {
        return root;
    }

    return newRoot;
}

// 基于Tree表，获取匹配的二元组
LZ77_Match_Binary GetMatchBinary(BYTE *lookHeadPos, LZRC_HashNode *root, const BYTE *inBuff, LZ77_Settings *settings, U32 leftLen)
{
    U32 maxMatchLen = leftLen > MAX_LOOK_LEN ? MAX_LOOK_LEN : leftLen;
    LZRC_HashNode *node = GetNode(settings->hashFunc(*(U32)lookHeadPos), root);
    LZ77_Match_Binary matchBinary;
    matchBinary.matchLen = 0; matchBinary.offset = 0; matchBinary.pos = 0;
    // 没有找到直接返回
    if (node == NULL) {
        return matchBinary;
    }
    S32 loop = 0;
    const BYTE *hashPos = NULL;
    if (node->runCodeFlag) {
        U32 currentRuncodeLen = settings->minMatchLen;
        while ((currentRuncodeLen < leftLen) && (lookHeadPos[currentRuncodeLen] == lookHeadPos[0])) {
            ++currentRuncodeLen;
        }
        for (; loop < HASH_VALUE_SIZE; loop += 2) {
            if (node->value[loop] == settings->hashValueMask) {
                break;
            }
            // find >= currentRuncodeLen if not find, help yourselfs
            if (node->value[loop + 1] >= currentRuncodeLen) {
                hashPos = inBuff + node->value[loop] + node->value[loop + 1] - currentRuncodeLen;
                // 防止发呆
                if (hashPos == lookHeadPos) {
                    hashPos = inBuff + node->value[loop];
                }
                if (hashPos >= lookHeadPos) {
                    continue; // 放弃本次处理 hash值有可能超前
                }
                if (settings->compFunc(hashPos, lookHeadPos) && ((lookHeadPos - hashPos) < settings->maxOffset)) {
                    U32 matchLen = GetMaxMatchLen(lookHeadPos, hashPos, settings->minMatchLen, maxMatchLen);
                    if (matchBinary.matchLen < matchLen) {
                        matchBinary.matchLen = matchLen;
                        matchBinary.offset = lookHeadPos - hashPos;
                    }
                }
            }
        }
    } else {
        for (; loop < HASH_VALUE_SIZE; loop++) {
            if (node->value[loop] == settings->hashValueMask) {
                break;
            }
            hashPos = inBuff + node->value[loop];
            if (hashPos >= lookHeadPos) {
                continue; // 同样放弃本次处理
            }
            if (settings->compFunc(hashPos, lookHeadPos) && ((lookHeadPos - hashPos) < settings->maxOffset)) {
                U32 matchLen = GetMaxMatchLen(lookHeadPos, hashPos, settings->minMatchLen, maxMatchLen);
                if (matchBinary.matchLen < matchLen) {
                    matchBinary.matchLen = matchLen;
                    matchBinary.offset = lookHeadPos - hashPos;
                }
            }
        }
    }
    return matchBinary;
}

void CalculateInEncodeTriplet(LZ77_HEAD *head, LZ77_TRIAD triplet)
{
    U32 preCrcCode = head->crcCode;
    const BYTE *startPos = triplet.unmatchData;
    for (S32 i = 0; i < triplet.unmatchLen + triplet.matchLen; i++) {
        preCrcCode = CRC32vC(preCrcCode, *startPos);
        startPos++;
    }
    head->crcCode = preCrcCode;
    return;
}

void EncodeTriplet(LZ77_HEAD *head, LZ77_TRIAD triplet, OutputStream *outputStream, LzrEncodeContex *contex)
{
#if defined(CRC_CHECK_ON)
    CalculateInEncodeTriplet(head, triplet);
#endif
    
    U32 unmatchLen = triplet.unmatchLen;
    // 编码nomatch-len
    while (unmatchLen >= UNSIGNED_CHAR_MAX_VALUE) {
        RCEncodeOneElement(&(contex->encodeContex),
                           outputStream,
                           UNSIGNED_CHAR_MAX_VALUE,
                           &(contex->literalLenFreqTable));
        unmatchLen -= UNSIGNED_CHAR_MAX_VALUE;
    }
    RCEncodeOneElement(&(contex->encodeContex), outputStream, unmatchLen, &(contex->literalLenFreqTable));

    // 编码未匹配的字符
    const BYTE *literal = triplet.unmatchData;
    for (S32 i = 0; i < triplet.unmatchLen; i++) {
        RCEncodeOneElement(&(contex->encodeContex), outputStream, literal[i], &(contex->literalFreqTable));
    }

    // 编码match-len
    while (triplet.matchLen >= UNSIGNED_CHAR_MAX_VALUE) {
        RCEncodeOneElement(&(contex->encodeContex),
                           outputStream,
                           UNSIGNED_CHAR_MAX_VALUE,
                           &(contex->matchLenFreqTable));
        triplet.matchLen -= UNSIGNED_CHAR_MAX_VALUE；
    }
    RCEncodeOneElement(&(contex->encodeContex), outputStream, triplet.matchLen, &(contex->matchLenFreqTable));

    // 编码offset的高地位，offset的高地位拆开分别统计频率进行编码，能稍微提高一点压缩率
    BYTE offsetLowByte = triplet.offset & 0xFF;
    BYTE offsetHighByte = (triplet.offset >> 8) & 0xFF;
    RCEncodeOneElement(&(contex->encodeContex), outputStream, offsetHighByte, &(contex->offSetHighBYTEFreqTable));
    RCEncodeOneElement(&(contex->encodeContex), outputStream, offsetLowByte, &(contex->offSetLowFreqTable));
}

S32 CheckInputParameter(const BYTE *inBuff, BYTE *outBuff, U32 inLen, U32 outLen)
{
    if (inBuff == NULL) {
        return INBUFF_NULL_POINTER;
    }
    if (outBuff == NULL) {
        return OUTPUT_NULL_POINTER;
    }
    if (inLen == 0) {
        return INVALID_INBUFF_LEN;
    }
    if (outLen < sizeof(LZ77_HEAD)) {
        return OUTBUFF_OVERFLOW;
    }
    return 0;
}

// lz77的主函数
S32 LzrcEncodeStream(const BYTE *inBuff, BYTE *outBuff, U32 inLen, U32 outLen, S32 strategy)
{
    S32 checkResult = CheckInputParameter(inBuff, outBuff, inLen, outLen);
    if (checkResult < 0) {
        return checkResult;
    }

    LZ77_Settings settings;
    // 基于小端设计，留出SSLTotalLen的位置，需要留出flag位置
    BYTE *op = outBuff + sizeof(LZ77_HEAD);
    LZ77_HEAD head;
    head.strategy = strategy;
    head.srcLen = inLen;
    head.crcCode = 0;

    InitEncodeSetting(*settings, strategy);
    LzrEncodeContex *lzrcContex = ConstructLzrcEncodeContex();
    if (lzrcContex == NULL) {
        return CONSTRUCT_CONTEX_FAILED;
    }

    OutputStream outputStream;
    InitOutputStream(&outputStream, op, outLen - sizeof(LZ77_HEAD));

    LZRC_HashNode *root = NULL;

    BYTE *lookhead_buff_pos = NULL;
    const BYTE *hashPos = inBuff;
    // -------------init some buffer--------------//
    // 前四个字节不做处理的初始化
    lookhead_buff_pos = (BYTE *)inBuff + SEARCH_BEGIN_AT;

    U32 leftLen = inLen - (lookhead_buff_pos - inBuff);
    // 每次输出的三元组
    LZ77_TRIAD triplet;
    U32 step = 0; // hash go step len
    // 将第一组数据加入到Hash表中
    while (hashPos < lookhead_buff_pos) {
        root = AddNewHashkey(hashPos, hashPos - inBuff, root, &settings, &step, inLen);
        hashPos += step;
    }
    RESET_TRIAD(triplet);
    // 其实的4个字节算到一开始未匹配的串中取
    triplet.unmatchLen = SEARCH_BEGIN_AT;
    triplet.unmatchData = inBuff;
    S32 boundaryValue = settings.bestMinMatchLen + settings.minMatchLen;

    do {
_P_E_NML_:
        if (triplet.unmatchData == NULL) {
            triplet.unmatchData = lookhead_buff_pos;
        }

        leftLen = inLen - (lookhead_buff_pos - inBuff);
        // 退出条件leftLen <= settings.minMatchLen;
        // 当尾部小于等于最小匹配位数settings.minMatchLen，则不处理；
        if (leftLen <= settings.minMatchLen) {
            if (!leftLen) {
                // 无剩余空间，退出
                break;
            }
            triplet.unmatchLen += leftLen;
            // 编码，并退出循环
            EncodeTriplet(&head, triplet, &outputStream, lzrcContex);
            break;
        }

        // 计算最大匹配长度
        LZ77_Match_Binary matchBinary = GetMatchBinary(lookhead_buff_pos, root, inBuff, &settings, leftLen);

        // 如果匹配长度等于自小长度，则向后查找，寻找最佳三元组
        // *** 当前未匹配继续往后找？有区别么？
        if (matchBinary.matchLen <= settings.bestMinMatchLen) {
            LZ77_Match_Binary matchBinaryTmp;
            S32 pos = 1;
            S32 lookMore = boundaryValue > leftLen ? (leftLen - settings.minMatchLen) : settings.bestMinMatchLen;

            for (; pos <= lookMore; pos++) {
                // todo 此处添加hash是否有危险？
                if (hashPos < (lookhead_buff_pos + pos - 1)) {
                    root = AddNewHashkey(lookhead_buff_pos + pos - 1, lookhead_buff_pos + pos - 1 - inBuff, root, &settings, &step, leftLen - pos);
                    hashPos += step;
                }

                matchBinaryTmp = GetMatchBinary(lookhead_buff_pos + pos, root, inBuff, &settings, leftLen - pos);
                if (matchBinaryTmp.matchLen > matchBinary.matchLen + pos + 1) {
                    matchBinary.matchLen = matchBinaryTmp.matchLen;
                    matchBinary.offset = matchBinaryTmp.offset;
                    // 计算获得pos
                    matchBinary.pos = pos;
                }
            }
        }
        // 只匹配了两个字符，没有压缩预算，不压缩
        if (matchBinary.matchLen < settings.minMatchLen) {
            ++triplet.unmatchLen;
            lookhead_buff_pos++;
            while (hashPos < lookhead_buff_pos) {
                root = AddNewHashkey(hashPos, hashPos - inBuff, root, &settings, &step, leftLen);
                hashPos += step;
            }
            goto _P_E_NML_;
        } else {
            // 如果最佳匹配串不是当前位置的数据，则还要再偏移
            if (matchBinary.pos) {
                triplet.unmatchLen += matchBinary.pos;
                lookhead_buff_pos += matchBinary.pos;
            }
            triplet.matchLen = matchBinary.matchLen;
            // offset 区间从1~65536 变成 0~65535
            triplet.offset = matchBinary.offset - 1;
            lookhead_buff_pos += matchBinary.matchLen;
            while (hashPos < lookhead_buff_pos) {
                root = AddNewHashkey(hashPos, hashPos - inBuff, root, &settings, &step, leftLen);
                hashPos += step;
            }
            // rangecode编码
            EncodeTriplet(&head, triplet, &outputStream, lzrcContex);
            RESET_TRIAD(triplet);
        }
    } while (1);

    EndRCEncode(&(lzrcContex->encodeContex), &outputStream);
    free(lzrcContex);
    // 释放tree
    FreeHashTree(root);
    if (outputStream.isOverflow == TRUE) {
        return OUTBUFF_OVERFLOW;
    }

    head.encodeLen = sizeof(LZ77_HEAD) + outputStream.outPos;
    U32 encodeLen = head.encodeLen;
    WriteLZ77Head(outBuff, head);

    return encodeLen;
}
