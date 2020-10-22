#include "rangecode_common.h"

void InitFreq(U32 *freq, S32 total)
{
    for (S32 i = 0; i < total; i++) {
        freq[i] = 1;
    }
}

void InitFreqTable(FreqTable *freqTable)
{
    freqTable->total = SYMBOL_COUNT;
    InitFreq(freqTable->freq, freqTable->total);
}

void InitRangeCodeContex(RangeCodeContex *contex)
{
    contex->low = 0;
    contex->high = 0xFFFFFFFF;
    contex->range = 0xFFFFFFFF;
    contex->maxTotal = SYMBOL_COUNT;
    contex->haveDelayBits = NO_DELAY_BITS;
    contex->code = 0;
    contex->bitsNeedReadForCode = U32_BITCOUNT;
    contex->codeState = NOT_READY;
    contex->rangeCodeState = FINISH_HANDLE_DELAY_BIT;
}

U32 GetRemainingBitsOfInputStream(InputStream *inputStream)
{
    U32 byteCount = inputStream->maxSize - inputStream->inputPos;
    U32 bitCount = inputStream->bitPos;
    return byteCount * UNSIGNED_CHAR_BIT_COUNT + bitCount;
}

void InitInputStream(InputStream *inputStream, BYTE *src, U32 maxSize)
{
    inputStream->byteCache = 0;
    inputStream->bitPos = 0;
    inputStream->inputPos = 0;
    inputStream->input = src;
    inputStream->maxSize = maxSize;
}

U32 ReadBits(S32 bitCount, InputStream *inputStream)
{
    S32 count;
    U32 tmp = 0;
    count = bitCount;
    while (count > 0) {
        if (inputStream->bitPos == 0) {
            if (inputStream->inputPos == inputStream->maxSize) {
                inputStream->byteCache = 0;
                inputStream->bitPos = UNSIGNED_CHAR_BIT_COUNT;
            } else {
                inputStream->byteCache = inputStream->input[inputStream->inputPos];
                inputStream->inputPos++;
                inputStream->bitPos = UNSIGNED_CHAR_BIT_COUNT;
            }
        }
        tmp = tmp << 1;
        BYTE bit = inputStream->byteCache >> (UNSIGNED_CHAR_BIT_COUNT - 1);
        tmp += bit;
        inputStream->byteCache = inputStream->byteCache << 1;
        inputStream->bitPos--;
        count--;
    }

    return tmp;
}

void InitOutputStream(OutputStream *outputStream, BYTE *src, U32 size)
{
    outputStream->byteCache = 0;
    outputStream->bitPos = 0;
    outputStream->outPos = 0;
    outputStream->output = src;
    outputStream->maxSize = size;
    outputStream->isOverflow = FALSE;
}

void FlushOutputStream(OutputStream *outputStream)
{
    if (outputStream->isOverflow == TRUE) {
        return;
    }
    if (outputStream->bitPos > 0) {
        outputStream->byteCache = outputStream->byteCache << (UNSIGNED_CHAR_BIT_COUNT - outputStream->bitPos);
        outputStream->output[outputStream->outPos] = outputStream->byteCache;
        outputStream->outPos++;
    }
}

void WriteBits(OutputStream *outputStream, U32 v, U32 count, DelayBitsFlag *flag)
{
    U32 countTmp;
    countTmp = count;
    int newAddedByteCount = (outputStream->bitPos + countTmp + 7) / UNSIGNED_CHAR_BIT_COUNT;
    if (outputStream->isOverflow == TRUE || outputStream->outPos + newAddedByteCount > outputStream->maxSize) {
        outputStream->isOverflow = TRUE;
        return;
    }
    if ((countTmp > 0) && (*flag == HAS_DELAY_BITS)) {
        *flag = NO_DELAY_BITS;
        U32 bit = (v >> (countTmp - 1)) & 1;
        countTmp--;
        U32 tmp = (outputStream->byteCache + bit) << (UNSIGNED_CHAR_BIT_COUNT - outputStream->bitPos);
        outputStream->byteCache = (tmp & 0xFF) >> (UNSIGNED_CHAR_BIT_COUNT - outputStream->bitPos);
        U32 index = outputStream->outPos - 1;
        while (tmp & 0xFFFFFF00) {
            tmp = (U32)(outputStream->output[index]) + 1;
            outputStream->output[index] = tmp & 0xFF;
            index--;
        }
    }
    while (countTmp > 0) {
        BYTE bit = (v >> (countTmp - 1)) & 1;
        outputStream->byteCache = (outputStream->byteCache << 1) + bit;
        outputStream->bitPos++;
        countTmp--;
        if (outputStream->bitPos == UNSIGNED_CHAR_BIT_COUNT) {
            outputStream->output[outputStream->outPos] = outputStream->byteCache;
            outputStream->outPos++;
            outputStream->byteCache = 0;
            outputStream->bitPos = 0;
        }
    }
}

U32 GetDelayBits(U32 low, U32 high, U32 *bitCount)
{
    U32 tmp, equalBitCount;
    *bitCount = 0;
    for (U16 i = 0; i < U32_BITCOUNT; i++) {
        tmp = (low + (1 << i)) ^ high;
        equalBitCount = __builtin_clz(tmp);
        if (equalBitCount >= U32_BITCOUNT - i) {
            *bitCount = U32_BITCOUNT - i;
            return (low >> (U32_BITCOUNT - *bitCount)) << (U32_BITCOUNT - *bitCount);
        }
    }
    return 0;
}

U32 GetAccumulateFreq(BYTE v, const U32 *freq)
{
    U32 accumulateFreq = 0;
    for (S32 i = 0; i < v; i++) {
        accumulateFreq += freq[i];
    }
    return accumulateFreq;
}

U32 UpdateFreqAndTotalFreq(RangeCodeContex *contex, U32 v, FreqTable *freqTable)
{
    freqTable->total += WEIGHT;
    freqTable->freq[v] += WEIGHT;
    if (freqTable->total > contex->maxTotal) {
        contex->maxTotal = freqTable->total;
    }
    return 0;
}
