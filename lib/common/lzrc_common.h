#include <stdint.h>

#ifndef RC_ENCODE_H
#define RC_ENCODE_H

#ifdef __cplusplus
extern "C"
{
#endif
uint32_t CRC32vC(uint32_t preCrcCode, uint8_t data);
void WriteLE32(void *dst, uint32_t value);
uint32_t ReadLE32(const void *ptr);

#ifdef __cplusplus
}
#endif
#endif
