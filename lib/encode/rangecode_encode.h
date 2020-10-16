#include <stdint.h>

#ifndef RANGECODE_ENCODE_H
#define RANGECODE_ENCODE_H
#include "rangecode_common.h"

#ifdef __cplusplus
extern "C" {
#endif

S32 RCEncodeOneElement(RangeCodeContex *contex, OutputStream *outputStream, U32 element, FreqTable *table);

S32 EndRCEncode(RangeCodeContex *contex, OutputStream *outputStream);

#ifdef __cplusplus
}
#endif
#endif
