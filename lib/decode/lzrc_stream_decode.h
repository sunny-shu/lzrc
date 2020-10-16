#ifndef LZRC_STREAM_DECODE_H
#define LZRC_STREAM_DECODE_H

#include "lz77_common.h"
#include "rangecode_common.h"
#include "rangecode_decode.h"
#include "lzrc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DECODING_LITERAL_LEN,
    DECODING_LITERAL,
    DECODING_MATCH_LEN,
    DECODING_OFFSET_HIGH_BYTE,
    DECODING_OFFSET_LOW_BYTE
} DecodingState;

typedef enum {
    NO_ERROR,
    DATA_FORMAT_ERROR,           // 格式错误，输入的数据与正确的压缩数据不匹配
    OUTBUFF_NOT_ENOUGH_SPACE,    // 解压时，输出地址的空间不足
    HISTORY_MALLOC_FAILED,       // 创建解压上下文时，历史查询窗口的内存分配失败
    CRC_CHECK_FAILED,            // 解压完成时校验CRC码失败
    HISTORY_SIZE_CHECK_FAILED,   // 修改codedex，添加对窗口的大小的校验，如果等于0，校验失败
    OFFSET_CHECK_FAILED,         // offset超过了最大值
    LZ77_HEAD_INIT_FAILED,       // 压缩头读取失败
    NO_INPUT_DATA                // 解压时insize为0
} LzrcDecodingErrorCode;

typedef struct {
    BYTE *data;
    U32 arraySize;
    U32 currentSize;
    BYTE *start;
    BYTE *end;
} CircularArray;

// 流式解压上下文，保存流式解压的相关数据及状态
typedef struct {
    DecodingState lzrcState;            // 当前的解压状态，详情请参考枚举值
    FreqTable literalFreqTable;         // 未匹配串的概率表
    FreqTable matchLenFreqTable;        // match-len的概率表
    FreqTable offSetHighBYTEFreqTable;
    FreqTable offSetLowFreqTable;
    FreqTable literalLenFreqTable;
    RangeCodeContex decodeContex;       // 区间的状态
    BlockStyle blockStyle;              // 解压块的类型，是否为最后一块
    ReadyOrNot tripletIsReady;          // 三元组是否解压完毕，单次三元组解压完毕后，可以进入LZ77的解压
    CircularArray history;              // LZ77解压用到的窗口，使用循环数组的结构
    LZ77_TRIAD triplet;                 // LZ77的三元组
    U32 literalNotDecodeCount;          // 流式解压在单次解压中会由于输入不够中断解压，该字段记录未匹配串还有多少未解压
    LZ77_HEAD lz77Head;                 // LZ77的压缩头
    U32 hasDecodeSize;                  // 记录已经解压了多少字节，用于作为解压结束及异常的判断条件
    U32 handledSize;                    // 记录已经处理的压缩数据，用于判断该块是否为最后一块
    ReadyOrNot lz77HeadInitOrNot;       // 记录LZ77head是否初始化过
    LzrcDecodingErrorCode errorCode;
    U32 crcCode;                        // crc校验码，边解压边计算，并与头部的校验码做对比，来保证解压的正确性
    U32 isDecodingFinished;             // 区间编码结束时会多输出几个字节，为了防止多余的解码，设置此标志
} LzrcStreamDecodeContex;

typedef struct {
    BYTE *outBuff;
    U32 maxSize;
    U32 curPos;
} ByteOutputStream;

S32 LzrcStreamDecode(LzrcStreamDecodeContex *lzrcContex, BYTE *in, U32 inSize, BYTE *out, U32 outSize);

LzrcStreamDecodeContex *ConstructLzrcStreamDecodeContex(void);

void DestroyLzrcStreamDecodeContex(LzrcStreamDecodeContex *contex);

#ifdef __cplusplus
}
#endif

#endif
