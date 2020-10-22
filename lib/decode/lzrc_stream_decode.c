#include "lzrc_stream_decode.h"

void IniByteOutputStream(ByteOutputStream *out, BYTE *buff, U32 size)
{
    out->maxSize = size;
    out->outBuff = buff;
    out->curPos = 0;
}

void WriteChar(ByteOutputStream *out, BYTE v, LzrcStreamDecodeContex *lzrcContex)
{
    lzrcContex->crcCode = CRC32vC(lzrcContex->crcCode, v);
    if (out->maxSize - out->curPos >= 1) {
        out->outBuff[out->curPos] = v;
        out->curPos++;
    } else {
        lzrcContex->errorCode = OUTBUFF_NOT_ENOUGH_SPACE;
    }
}

void InitCircularArray(LzrcStreamDecodeContex *contex, U32 size)
{
    // 先将指针置为NULL，malloc不会初始化内存；防止申请失败，后面调用free时，指针不为NULL报异常
    contex->history.data = NULL;
    contex->history.arraySize = size;
    contex->history.currentSize = 0;
    contex->history.start = contex->history.end = NULL;
    if (size == 0) {
        contex->errorCode = HISTORY_SIZE_CHECK_FAILED;
        return;
    }
    BYTE *data = (BYTE *)malloc(size);
    if (data == NULL) {
        contex->errorCode = HISTORY_MALLOC_FAILED;
        return;
    }
    contex->history.data = data;
}

void DestroyCircularArray(CircularArray *array)
{
    if (array->data) {
        free(array->data);
        array->data = NULL;
    }
}

void PutCharsToCircularArray(BYTE *in, U32 inSize, CircularArray *array)
{
    if (inSize == 0) {
        return;
    }
    S32 i = 0;
    // 循环数组为空时的处理
    if (array->currentSize == 0) {
        array->data[0] = in[0];
        array->start = array->end = array->data;
        array->currentSize++;
        i++;
    }

    for (; i < inSize; i++) {
        // 循环数组满了后，新的加入的字节得覆盖掉start位置的数据
        if (array->currentSize == array->arraySize) {
            *(array->start) = in[i];
            array->end = array->start;
            array->start++;
            if (array->start == (array->data + array->arraySize)) {
                array->start = array->data;
            }
        } else { // 循环数组未满时，字节存入currentSize的位置
            array->currentSize++;
            array->end++;
            *(array->end) = in[i];
        }
    }
}

U32 ExistErrorInTriplet(LzrcStreamDecodeContex *lzrcContex)
{
    U32 unmatchLen = lzrcContex->triplet.unmatchLen;
    U32 matchLen = lzrcContex->triplet.matchLen;
    U32 offset = lzrcContex->triplet.offset;
    U32 hasError = FALSE;
    U32 maxCurrentOffset = lzrcContex->history.currentSize;

    // 校验offset，防止压缩后的报文被篡改后，解压出来的offset超大，引起内存越界访问
    if (offset >= maxCurrentOffset) {
        lzrcContex->errorCode = OFFSET_CHECK_FAILED;
        hasError = TRUE;
    } else if (matchLen == 0 && unmatchLen == 0) { // 正常的三元组不可能出现matchLen与unmatchLen全为0。该异常会导致死循环
        lzrcContex->errorCode = DATA_FORMAT_ERROR;
        hasError = TRUE;
    }
    return hasError;
}

void  HandleMatch(ByteOutputStream *out, LzrcStreamDecodeContex *lzrcContex)
{
    CircularArray *array = &lzrcContex->history;
    U32 matchLen = lzrcContex->triplet.matchLen;
    U32 offset = lzrcContex->triplet.offset;
    U32 delta = array->end - array->data;

    if (unlikely(ExistErrorInTriplet(lzrcContex))) {
        return;
    }

    // 确定匹配串在循环数组中的起始位置
    BYTE *matchStartPos = NULL;
    if (delta >= offset) {
        matchStartPos = array->end - offset;
    } else {
        matchStartPos = array->data + (array->arraySize - (offset - delta));
    }

    // 拷贝字符串到解压输出的buff，同时将解压出的支付放入循环数组中，更新查询窗口
    while (matchLen > 0) {
        WriteChar(out, *matchStartPos, lzrcContex);
        PutCharsToCircularArray(matchStartPos, 1, array);
        matchStartPos++;
        if (matchStartPos == (array->data + array->arraySize)) {
            matchStartPos = array->data;
        }
        matchLen--;
    }
    lzrcContex->hasDecodeSize += lzrcContex->triplet.matchLen;

    // 文件解压结束时，解压出的数据大于原始字节，说明压缩数据的格式不正确
    if (lzrcContex->hasDecodeSize > lzrcContex->lz77Head.srcLen) {
        lzrcContex->errorCode = DATA_FORMAT_ERROR;
    }
#if defined(CRC_CHECK_ON)
    // 解压出的字节数与原始字节数相同后，使用头部的校验码进行校验
    if (lzrcContex->hasDecodeSize == lzrcContex->lz77Head.srcLen && lzrcContex->crcCode != lzrcContex->lz77Head.crcCode) {
        lzrcContex->errorCode = CRC_CHECK_FAILED;
    }
#endif
}

LzrcStreamDecodeContex *ConstructLzrcStreamDecodeContex(void)
{
    LzrcStreamDecodeContex *lzrcContex = (LzrcStreamDecodeContex *)malloc(sizeof(LzrcStreamDecodeContex));
    if (lzrcContex == NULL) {
        return NULL;
    }

    lzrcContex->lzrcState = DECODING_LITERAL_LEN;
    lzrcContex->blockStyle = FIRST_BLOCK;
    lzrcContex->lz77HeadInitOrNot = NOT_READY;
    lzrcContex->errorCode = NO_ERROR;
    lzrcContex->hasDecodeSize = 0;
    lzrcContex->handledSize = 0;

    InitCircularArray(lzrcContex, WINDOW_SIZE);
    if (lzrcContex->errorCode != NO_ERROR) {
        DestroyLzrcStreamDecodeContex(lzrcContex);
        return NULL;
    }

    InitFreqTable(&(lzrcContex->literalFreqTable));
    InitFreqTable(&(lzrcContex->literalLenFreqTable));
    InitFreqTable(&(lzrcContex->matchLenFreqTable));
    InitFreqTable(&(lzrcContex->offSetHighBYTEFreqTable));
    InitFreqTable(&(lzrcContex->offSetLowFreqTable));

    lzrcContex->tripletIsReady = NOT_READY;
    lzrcContex->triplet.matchLen = 0;
    lzrcContex->triplet.offset = 0;
    lzrcContex->triplet.unmatchLen = 0;
    lzrcContex->crcCode = 0;
    lzrcContex->isDecodingFinished = FALSE;

    InitRangeCodeContex(&(lzrcContex->decodeContex));

    return lzrcContex;
}

void DestroyLzrcStreamDecodeContex(LzrcStreamDecodeContex *contex)
{
    free(contex->history.data);
    free(contex);
}

void DecodeLiteralLen(LzrcStreamDecodeContex *lzrcContex, InputStream *inputStream, BlockStyle blockStyle)
{
    U32 decodeNum;
    LZ77_TRIAD *triplet = &lzrcContex->triplet;
    RangeCodeContex *rangeCodeContex = &lzrcContex->decodeContex;
    if (rangeCodeContex->rangeCodeState == FINISH_DECODE) {
        RCHandleDelayBitsInDecoding(rangeCodeContex, inputStream, blockStyle);
    }
    if (rangeCodeContex->rangeCodeState == FINISH_HANDLE_DELAY_BIT) {
        decodeNum = RCDecodeElement(rangeCodeContex, inputStream, &lzrcContex->literalLenFreqTable, blockStyle);
        if (rangeCodeContex->rangeCodeState == FINISH_DECODE) {
            triplet->unmatchLen += decodeNum;
            if (decodeNum < UNSIGNED_CHAR_MAX_VALUE) {
                lzrcContex->literalNotDecodeCount = triplet->unmatchLen;
                lzrcContex->hasDecodeSize += triplet->unmatchLen;
                lzrcContex->lzrcState = triplet->unmatchLen == 0 ? DECODING_MATCH_LEN : DECODING_LITERAL;
            }
        }
    }
}

void DecodeLiteral(LzrcStreamDecodeContex *lzrcContex, InputStream *inputStream, ByteOutputStream *out,
                   BlockStyle blockStyle)
{
    BYTE decodeNum;

    RangeCodeContex *rangecodeContex = &lzrcContex->decodeContex;
    if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
        RCHandleDelayBitsInDecoding(rangecodeContex, inputStream, blockStyle);
    }

    if (rangecodeContex->rangeCodeState == FINISH_HANDLE_DELAY_BIT) {
        decodeNum = RCDecodeElement(rangecodeContex, inputStream, &lzrcContex->literalFreqTable, blockStyle);
        if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
            lzrcContex->literalNotDecodeCount--;
            if (lzrcContex->literalNotDecodeCount == 0) {
                lzrcContex->lzrcState = DECODING_MATCH_LEN;
            }
            WriteChar(out, decodeNum, lzrcContex);
            PutCharsToCircularArray(&decodeNum, 1, &lzrcContex->history);
            return;
        }
    }
}

void DecodeMatchLen(LzrcStreamDecodeContex *lzrcContex, InputStream *inputStream, BlockStyle blockStyle)
{
    U32 decodeNum;
    LZ77_TRIAD *triplet = &lzrcContex->triplet;
    RangeCodeContex *rangecodeContex = &lzrcContex->decodeContex;
    if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
        RCHandleDelayBitsInDecoding(rangecodeContex, inputStream, blockStyle);
    }
    if (rangecodeContex->rangeCodeState == FINISH_HANDLE_DELAY_BIT) {
        decodeNum = RCDecodeElement(rangecodeContex, inputStream, &lzrcContex->matchLenFreqTable, blockStyle);
        if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
            triplet->matchLen += decodeNum;
            if (decodeNum < UNSIGNED_CHAR_MAX_VALUE) {
                lzrcContex->lzrcState = DECODING_OFFSET_HIGH_BYTE;
            }
        }
    }
}

void DecodeOffsetHighByte(LzrcStreamDecodeContex *lzrcContex, InputStream *inputStream, BlockStyle blockStyle)
{
    U32 decodeNum;
    LZ77_TRIAD *triplet = &lzrcContex->triplet;
    RangeCodeContex *rangecodeContex = &lzrcContex->decodeContex;
    if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
        RCHandleDelayBitsInDecoding(rangecodeContex, inputStream, blockStyle);
    }
    if (rangecodeContex->rangeCodeState == FINISH_HANDLE_DELAY_BIT) {
        decodeNum = RCDecodeElement(rangecodeContex, inputStream, &lzrcContex->offSetHighBYTEFreqTable, blockStyle);
        if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
            triplet->offset += decodeNum << UNSIGNED_CHAR_BIT_COUNT;
            lzrcContex->lzrcState = DECODING_OFFSET_LOW_BYTE;
        }
    }
}

void DecodeOffsetLowByte(LzrcStreamDecodeContex *lzrcContex, InputStream *inputStream, BlockStyle blockStyle)
{
    U32 decodeNum;
    LZ77_TRIAD *triplet = &lzrcContex->triplet;
    RangeCodeContex *rangecodeContex = &lzrcContex->decodeContex;
    if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
        RCHandleDelayBitsInDecoding(rangecodeContex, inputStream, blockStyle);
    }
    if (rangecodeContex->rangeCodeState == FINISH_HANDLE_DELAY_BIT) {
        decodeNum = RCDecodeElement(rangecodeContex, inputStream, &lzrcContex->offSetLowFreqTable, blockStyle);
        if (rangecodeContex->rangeCodeState == FINISH_DECODE) {
            triplet->offset += decodeNum;
            lzrcContex->lzrcState = DECODING_LITERAL_LEN;
            lzrcContex->tripletIsReady = READY;
        }
    }
}

void ResetTriplet(LzrcStreamDecodeContex *lzrcContex)
{
    LZ77_TRIAD *triplet = &lzrcContex->triplet;
    triplet->matchLen = 0;
    triplet->unmatchLen = 0;
    triplet->offset = 0;
    lzrcContex->tripletIsReady = NOT_READY;
}

U32 IfExistError(LzrcStreamDecodeContex *lzrcContex, U32 inSize)
{
    lzrcContex->handledSize += inSize;
    // 解压最后一块时，如果输入字节不够了需要补零。所以根据已处理的字节数判断是否最后一块，并存储给后面使用
    if (unlikely(lzrcContex->handledSize == lzrcContex->lz77Head.encodeLen)) {
        lzrcContex->blockStyle = LAST_BLOCK;
    }
    // 已处理的压缩数据的长度大于压缩头记录的压缩长度，说明数据出现异常了，无法继续解压
    if (lzrcContex->handledSize > lzrcContex->lz77Head.encodeLen) {
        lzrcContex->errorCode = DATA_FORMAT_ERROR;
    }
    U32 hasError = lzrcContex->errorCode == NO_ERROR ? FALSE : TRUE;
    switch (lzrcContex->errorCode) {
        case NO_ERROR:
            if (inSize == 0) {
                // 不存在错误的情况下， 如果输入为零，则错误码设置为NO_INPUT_DATA，方便调用者记录告警
                lzrcContex->errorCode = NO_INPUT_DATA;
                hasError = TRUE;
            }
            break;
        case NO_INPUT_DATA:
            if (inSize > 0) {
                // 如果之前的错误码为NO_INPUT_DATA.此次输入长度不为零，则清除该错误码，因为输入为零并不会造成解压失败
                lzrcContex->errorCode = NO_ERROR;
                hasError = FALSE;
            }
            break;
        default:
            // 除NO_ERROR、NO_INPUT_DATA以外的错误，都会导致解压失败，所以错误码被设置后不能被清除，而且检测到后直接停止解压
            break;
    }
    return hasError;
}

void InitInput(LzrcStreamDecodeContex *lzrcContex, BYTE *in, U32 inSize, InputStream *inputStream)
{
    BYTE *start = in;
    U32 payloadLen = inSize;
    U32 headSize = sizeof(LZ77_HEAD);
    // 第一次解压时，需要使用压缩头部数据，初始化一些字段
    if (unlikely(lzrcContex->lz77HeadInitOrNot == NOT_READY)) {
        // 第一次解压时，如果字节数不够，会导致头部数据初始化失败
        if (inSize < headSize) {
            lzrcContex->errorCode = LZ77_HEAD_INIT_FAILED;
            return;
        }
        start = in + headSize;
        payloadLen -= headSize;
        ReadLZ77Head(in, &(lzrcContex->lz77Head));
        lzrcContex->lz77HeadInitOrNot = READY;
    }
    InitInputStream(inputStream, start, payloadLen);
}

S32 lzrcStreamDecode(LzrcStreamDecodeContex *lzrcContex, BYTE *in, U32 inSize, BYTE *out, U32 outSize)
{
    ByteOutputStream output;
    IniByteOutputStream(&output, out, outSize);
    InputStream inputStream;
    InitInput(lzrcContex, in, inSize, &inputStream);
    // 如果存在错误或者解压已结束，直接返回不继续解压。output的curPos字段，记录了往outbuff里面写了多少个字节
    if (unlikely(IfExistError(lzrcContex, inSize) || lzrcContex->isDecodingFinished)) {
        return output.curPos;
    }

    while (1) {
        switch (lzrcContex->lzrcState) {
            case DECODING_LITERAL_LEN:
                DecodeLiteralLen(lzrcContex, &inputStream, lzrcContex->blockStyle);
                break;
            case DECODING_LITERAL:
                DecodeLiteral(lzrcContex, &inputStream, &output, lzrcContex->blockStyle);
                break;
            case DECODING_MATCH_LEN:
                DecodeMatchLen(lzrcContex, &inputStream, lzrcContex->blockStyle);
                break;
            case DECODING_OFFSET_HIGH_BYTE:
                DecodeOffsetHighByte(lzrcContex, &inputStream, lzrcContex->blockStyle);
                break;
            case DECODING_OFFSET_LOW_BYTE:
                DecodeOffsetLowByte(lzrcContex, &inputStream, lzrcContex->blockStyle);
                break;
        }
        if (lzrcContex->tripletIsReady == READY) {
            HandleMatch(&output, lzrcContex);
            ResetTriplet(lzrcContex);
            // 文件解压结束条件
            if (lzrcContex->hasDecodeSize >= lzrcContex->lz77Head.srcLen || lzrcContex->errorCode != NO_ERROR) {
                lzrcContex->isDecodingFinished = TRUE;
                return output.curPos;
            }
        } else {
            if (unlikely(lzrcContex->decodeContex.codeState == NOT_READY)) {
                return output.curPos;
            }
        }
    }
}
