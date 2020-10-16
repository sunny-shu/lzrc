#ifndef RANGECODE_COMMON_H
#define RANGECODE_COMMON_H

#include "lzrc_data_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEIGHT 1
#define UNSIGNED_CHAR_MAX_VALUE 255
#define UNSIGNED_CHAR_BIT_COUNT 8
#define SYMBOL_COUNT 256
#define U32_BITCOUNT 32

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef enum
{
    FINISH_DECODE,
    FINISH_HANDLE_DELAY_BIT
} DecodeState;

typedef enum
{
    HAS_DELAY_BITS,
    NO_DELAY_BITS
} DelayBitsFlag;

typedef enum
{
    READY,
    NOT_READY
} ReadyOrNot;

typedef struct
{
    BYTE byteCache;
    U32 bitPos;
    BYTE *input;
    U32 inputPos;
    U32 maxSize;
} InputStream;

typedef struct
{
    BYTE byteCache;
    U32 bitPos;
    BYTE *output;
    U32 outPos;
    U32 maxSize;
    U32 isOverflow;
} OutputStream;

typedef struct
{
    U32 low;
    U32 high;
    U32 range;
    U32 maxTotal;
    U32 code;
    U32 bitsNeedReadForCode;
    U32 lastOutputBitCount;
    DelayBitsFlag haveDelayBits;
    ReadyOrNot codeState;
    DecodeState rangeCodeState;
} RangeCodeContex;

typedef struct 
{
   U32 freq[SYMBOL_COUNT];
   U32 total;
} FreqTable;

void InitRreqTable(FreqTable *freqTable);

void InitInputStream(InputStream *inputStream, BYTE *src, U32 maxSize);

U32 ReadBits(S32 bitCount, InputStream *inputStream);

void InitOutputStream(OutputStream *outputStream, BYTE *src, U32 size);

U32 GetRemainningBitsOfInputStream(InputStream *inputStream);

void FlushOutputStream(OutputStream *outputStream);

void WriteBits(OutputStream *outputStream, U32 v, U32 count, DelayBitsFlag *flag);

void InitRangeCodeContex(RangeCodeContex *contex);

U32 GetDelayBits(U32 low, U32 high, U32 *bitCount);

U32 GetAccumulateFreq(BYTE v, const U32 *freq);

U32 UpdateFreqAndTotalFreq(RangeCodeContex *contex, U32 v, FreqTable *freqTable);


#ifdef __cplusplus
}
#endif
#endif