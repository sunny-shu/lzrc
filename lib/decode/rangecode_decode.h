#ifndef RANGECODE_DECODE_H
#define RANGECODE_DECODE_H

#include "rangecode_common.h"
#ifdef __cplusplus
extern "C" {
#endif

void RCHandleDelayBitsInDecoding(RangeCodeContex *contex, InputStream *inputStream, BlockStyle blockStyle);

U32 RCDecodeElement(RangeCodeContex *contex, InputStream *inputStream, FreqTable *freaTable,
                    BlockStyle blockStyle);


#ifdef __cplusplus
}
#endif
#endif
