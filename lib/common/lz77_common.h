#ifndef LZ77_COMMON_H
#define LZ77_COMMON_H
#include "lzrc_data_type.h"
#include "lzrc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL 0
#endif

#define NM_BIT_LEN                8
#define OFFSET_BIT_LEN            16
#define DEFAUIT_SEARCH_BUFF_LEN   ((1 << OFFSET_BIT_LEN) - 1) // 1 << 10
#define MAX_NM_LEN                ((1 << NM_BIT_LEN) - 1)     // 256 - 1
#define BUFF_SIZE                 (1 << 18)
#define SEARCH_BEGIN_AT           1
#define FLAG_BIGGER_SSL           128
#define SSL_MASK                  (FLAG_BIGGER_SSL - 1)
#define FLAG_MATCHS               128
#define ENCODE_NM                 0 // not match
#define ENCODE_M                  1 // match
#define HASH_TABLE_SIZE           (1 << 16)
#define OFFSET_WIDTH              4
#define NOTMATCH_WIDTH            1
#define MATCH_WIDTH               1
#define MAX_OFFSET
#define WINDOW_SIZE               ((1 << 10) * 64)

// lz77字典
#define DICS_NUM 4
#define DIC_SIZE 127
typedef struct {
    BYTE dics[DICS_NUM][DIC_SIZE];  // 最长定为255，最后一个直接用来放长度
    BYTE current;                   // 当前要刷新的字典
} LZRC_Dics;

typedef enum {
    LZ77_NORMAL = 0,
    LZ77_HIGHT_RATIO = 1
} ENCODE_STRATEGY;

// lz77三元组
typedef struct {
    U32 unmatchLen;
    const BYTE *unmatchData;
    U32 matchLen;
    U32 offset;
} LZ77_TRIAD;

// lz77头文件
typedef struct {
    S32 strategy;
    U32 srcLen;
    U32 encodeLen;
    U32 crcCode;
} LZ77_HEAD;

/*
 * Compiler secifics
 */
#ifndef UTIL_STATIC
#define UTIL_STATIC static __attribute__((unused))
#endif

UTIL_STATIC void WriteLZ77Head(BYTE *out, const LZ77_HEAD head)
{
    UtilWriteLE32(out, head.strategy);
    UtilWriteLE32(out + 4, head.srcLen);
    UtilWriteLE32(out + 8, head.encodeLen);
    UtilWriteLE32(out + 12, head.crcCode);
}

UTIL_STATIC void ReadLZ77Head(const BYTE* in, LZ77_HEAD *head)
{
    head->strategy = UtilReadLE32(in);
    head->srcLen = UtilReadLE32(in + 4);
    head->encodeLen = UtilReadLE32(in + 8);
    head->crcCode = UtilReadLE32(in + 12);
}

#ifdef __cplusplus
}
#endif
#endif
