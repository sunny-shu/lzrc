#include "rangecode_encode.h"

void UpdateRange(RangeCodeContex *contex, U32 element, FreqTable *freqTable)
{
    U32 step, lfreq;
    step = contex->range / freqTable->total;

    lfreq = GetAccumulateFreq(element, freqTable->freq);
    contex->low = contex->low + step * lfreq;
    contex->high = contex->low + step * freqTable->freq[element];
    UpdateFreqAndTotalFreq(contex, element, freqTable);
}

S32 OutputConstantNumber(RangeCodeContex *contex, OutputStream *outputStream)
{
    U32 tmp, bitsToOutput;
    U32 equalBitCount, rShiftBitCount;
    // 获取low和high高位相等的bit
    tmp = contex->low ^ contex->high;
    equalBitCount = __builtin_clz(tmp);
    rShiftBitCount = U32_BITCOUNT - equalBitCount;
    bitsToOutput = contex->low >> rShiftBitCount;

    WriteBits(outputStream, bitsToOutput, equalBitCount, &contex->haveDelayBits);
    contex->low = contex->low << equalBitCount;
    contex->high = contex->high << equalBitCount;
    return equalBitCount;
}

S32 OutputDelayNumber(RangeCodeContex *contex, OutputStream *outputStream)
{
    U32 bitsToOutput;
    U32 bitCount, rShiftBitCount;

    U32 delayNum = GetDelayBits(contex->low, contex->high, &bitCount);
    rShiftBitCount = U32_BITCOUNT - bitCount;
    bitsToOutput = delayNum >> rShiftBitCount;

    contex->low = (contex->low - delayNum) << (bitCount - 1);
    contex->high = (contex->high - delayNum) << (bitCount - 1);
    WriteBits(outputStream, bitsToOutput, bitCount, &contex->haveDelayBits);
    contex->haveDelayBits = HAS_DELAY_BITS;
    return bitCount;
}

S32 RCEncodeOneElement(RangeCodeContex *contex, OutputStream *outputStream, U32 element, FreqTable *table)
{
    U32 preOutputPos, preBits;
    preOutputPos = outputStream->outPos;
    preBits = outputStream->bitPos;

    UpdateRange(contex, element, table);
    OutputConstantNumber(contex, outputStream);

    if (contex->high - contex->low < contex->maxTotal) {
        OutputDelayNumber(contex, outputStream);
    }

    contex->range = contex->high - contex->low;
    contex->lastOutputBitCount = (outputStream->outPos - preOutputPos) * UNSIGNED_CHAR_BIT_COUNT + (outputStream->bitPos - preBits);

    return contex->lastOutputBitCount;
}

S32 EndRCEncode(RangeCodeContex *contex, OutputStream *outputStream)
{
    U32 preOutputPos, preBits;
    preOutputPos = outputStream->outPos;
    preBits = outputStream->bitPos;

    U32 bitCountToOutput = U32_BITCOUNT - contex->lastOutputBitCount + (contex->haveDelayBits == HAS_DELAY_BITS ? 1 : 0);
    U32 bitsToOutput = contex->low >> (U32_BITCOUNT - bitCountToOutput);

    WriteBits(outputStream, bitsToOutput, bitCountToOutput, &contex->haveDelayBits);
    FlushOutputStream(outputStream);

    return (outputStream->outPos - preOutputPos) * UNSIGNED_CHAR_BIT_COUNT + (outputStream->bitPos - preBits);
}
