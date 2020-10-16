#ifndef LZRC_COMMON_H
#define LZRC_COMMON_H
#include "lzrc_data_type.h"

#define expect(expr, value)   (__builtin_expect ((expr), (value)) )

#ifndef likely
#define likely(expr)   expect((expr) != 0, 1)
#endif
#ifndef unlikely
#define unlikely(expr)   expect((expr) != 0, 0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

U32 CRC32vC(U32 crcCode, BYTE data);
void UtilWriteLE32(void *dst, U32 value32);
U32 UtilReadLE32(const void *s);

#ifdef __cplusplus
}
#endif

#endif
