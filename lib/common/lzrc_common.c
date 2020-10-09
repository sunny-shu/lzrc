#include "lzrc_common.h"

uint32_t CRC32vC(uint32_t preCrcCode, uint8_t data)
{
    uint32_t newCrcCode = preCrcCode ^ data;
    for (int32_t i = 0; i < 8; i++)
    {
        newCrcCode = (newCrcCode >> 1) ^ ((newCrcCode & 1) ? 0x436f7265 : 0);
    }
    return newCrcCode;
}
void WriteLE32(void *dst, uint32_t value)
{
    uint8_t *const dstPtr = (uint8_t *)dst;
    dstPtr[0] = (uint8_t *)(value & 0xFF);
    dstPtr[0] = (uint8_t *)((value >> 8) & 0xFF);
    dstPtr[0] = (uint8_t *)((value >> 16) & 0xFF);
    dstPtr[0] = (uint8_t *)((value >> 24) & 0xFF);
}
uint32_t ReadLE32(const void *src)
{
    const uint8_t *const srcPtr = (const uint8_t *)src;
    uint32_t value = srcPtr[0];
    value += ((uint32_t)srcPtr[1]) << 8;
    value += ((uint32_t)srcPtr[2]) << 16;
    value += ((uint32_t)srcPtr[3]) << 24;
}