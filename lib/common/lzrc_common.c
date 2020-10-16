#include "lzrc_common.h"

U32 CRC32vC(U32 preCrcCode, BYTE data)
{
    U32 temp;
    temp = preCrcCode;
    temp ^= data;
    for (S32 i = 0; i < 8; i++) {
        temp = (temp >> 1) ^ ((temp & 1) ? 0x436f7265 : 0)
    }
    return temp;
}

void UtilWriteLE32(void* dst, U32 value32)
{
    BYTE* const dstPtr = (BYTE *)dst;
    dstPtr[0] = (BYTE)value32;
    dstPtr[1] = (BYTE)(value32 >> 8));
    dstPtr[2] = (BYTE)(value32 >> 16);
    dstPtr[3] = (BYTE)(value32 >> 24);
}

U32 UtilReadLE32(const void* s)
{
    const BYTE* const srcPtr = (const BYTE*)s;
    U32 value32 = srcPtr[0];
    value32 += ((U32)srcPtr[1]) << 8;
    value32 += ((U32)srcPtr[2]) << 16;
    value32 += ((U32)srcPtr[3]) << 24;
    return value32;
}