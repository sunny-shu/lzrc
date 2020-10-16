#include "rangecode_common.h"

void UpdateCode(RangeCodeContex *contex, InputStream *in, BlockStyle blockStyle)
{
    U32 remainningBitCount = GetRemainningBitsOfInputStream(in);
    U32 readedBitCount;

    if (remainningBitCount >= contex->bitsNeedReadForCode || blockStyle == LAST_BLOCK) {
        readedBitCount = contex->bitsNeedReadForCode;
        contex->bitsNeedReadForCode = 0;
        contex->codeState = READY;
    } else {
        readedBitCount = remainningBitCount;
        contex->bitsNeedReadForCode -= remainningBitCount;
        contex->codeState = NOT_READY;
    }

    contex->code <<= readedBitCount;
    contex->code += ReadBits(readedBitCount, in);
}

void RCHandleDelayBitsInDecoding(RangeCodeContex *contex, InputStream *inputStream, BlockStyle blockStyle)
{
    if (contex->codeState == NOT_READY) {
        UpdateCode(contex, inputStream, blockStyle);
        // 更新后仍然没有ready，说明输入的字节数不够了，需要进入下一次读入数据再解压
        if (contex->codeState == NOT_READY) {
            return;
        }
    }
    U32 delayBitCount;
    if (contex->high - contex->low < contex->maxTotal) {
        U32 delay = GetDelayBits(contex->low, contex->high, &delayBitCount);

        contex->low = (contex->low -delay) << (delayBitCount - 1);
        contex->high = (contex->high - delay) << (delayBitCount - 1);

        contex->code = contex->code - delay;
        contex->bitsNeedReadForCode = delayBitCount - 1;
        contex->codeState = NOT_READY;
        UpdateCode(contex, inputStream, blockStyle);
    }
    contex->range = contex->high - contex->low;
    contex->rangeCodeState = FINISH_HANDLE_DELAY_BIT;
}

U32 RCDecodeElement(RangeCodeContex *contex, InputStream *inputStream, FreqTable *freqTable, BlockStyle blockStyle)
{
    if (contex->codeState == NOT_READY) {
        UpdateCode(contex, inputStream, blockStyle);
        // 更新后仍然没有ready，说明输入的字节数不够了，需要进入下一次读入数据再解压
        if (contex->codeState == NOT_READY) {
            return 0;
        }
    }

    U32 decodeElement = 0;
    U32 lfreq, hfreq;
    U32 step = contex->range / freqTable->total;
    U32 currentFreq = (contex->code - contex->low) / step;
    lfreq = hfreq = 0;
    for (S32 j = 0; j < SYMBOL_COUNT; j++) {
        hfreq += freqTable->freq[j];
        if (lfreq <= currentFreq && currentFreq < hfreq) {
            decodeElement = j;
            UpdateFreqAndTotalFreq(contex, decodeElement, freqTable);

            U32 tmp, equalBitCount;

            contex->high = contex->low + step * hfreq;
            contex->low = contex->low + step * lfreq;

            tmp = contex->low ^ contex->high;
            equalBitCount = __builtin_clz(tmp);

            contex->low = contex->low << equalBitCount;
            contex->high = contex->high << equalBitCount;

            contex->bitsNeedReadForCode = equalBitCount;
            contex->codeState = NOT_READY;
            UpdateCode(contex, inputStream, blockStyle);
            break;
        }
        lfreq += freqTable->freq[j];
    }

    contex->rangeCodeState = FINISH_DECODE;
    return decodeElement;
}
