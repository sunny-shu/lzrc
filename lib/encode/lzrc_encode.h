#ifndef LZRC_ENCODE_H
#define LZRC_ENCODE_H

#include <stdint.h>
#include "lzrc_data_type.h"
#include <stdlib.h>
#include <string.h>
#include "lz77_common.h"
#include "rangecode_common.h"
#include "rangecode_encode.h"
#include "lzrc_common.h"

#define ERROR_OUTBUFF_OVERFLOW (-1)
#ifdef __cpluspulus
extern "C" {
#endif

#define LZRC_VERSION_MAJOR   1
#define LZRC_VERSION_MINOR   0
#define LZRC_VERSION_RELEASE 0

#define LZRC_VERSION_NUMBER (LZRC_VERSION_MAJOR * 100 * 100 + LZRC_VERSION_MINOR * 100 + LZRC_VERSION_RELEASE)

#define LZRC_LIB_VERSION LZRC_VERSION_MAJOR.LZRC_VERSION_MINOR.LZRC_VERSION_RELEASE
#define QUOTE(string) #string
#define EXPAND_AND_QUOTE(string) QUOTE(string)
#define LZRC_VERSION_STRING EXPAND_AND_QUOTE(LZRC_LIB_VERSION)

typedef enum {
    CONSTRUCT_CONTEX_FAILED = -1,  // 创建压缩上下文失败（malloc失败）
    OUTBUFF_OVERFLOW = -2,         // outbuff的空间不足
    INVALID_INBUFF_LEN = -3,       // 输入数据的长度为0
    INBUFF_NULL_POINTER = -4,      // inbufff为空指针
    OUTPUT_NULL_POINTER = -5       // outbuff为空指针
} LzrcEncodeErrorCode;

typedef struct {
    FreqTable literalFreqTable;
    FreqTable matchLenFreqTable;
    FreqTable offSetHighBYTEFreqTable;
    FreqTable offSetLowFreqTable;
    FreqTable literalLenFreqTable;
    RangeCodeContex encodeContex;
    LZRC_Dics dics;
} LzrEncodeContex;

S32 LzrcEncodeStream(const BYTE *inBuff, BYTE *outBuff, U32 inLen, U32 outLen, S32 strategy);

S32 LzrcVersionNumber(void);

const char* LZRCVersionString(void);

#ifdef __cpluspulus
}
#endif

#endif
