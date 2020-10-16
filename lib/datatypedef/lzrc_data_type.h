#ifndef LZRC_DATA_TYPE_H
#define LZRC_DATA_TYPE_H

#include <string.h>
#include <stdlib.h>
#if (define(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))) /* C99 */
#include <stdint.h>
typedef uint8_t BYTE;
typedef uint16_t U16;
typedef int16_t S16;
typedef uint32_t U32;
typedef int32_t S32;
typedef uint64_t U64;
typedef uint64_t S64;
#else
typedef unsigned char BYTE;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned int U32;
typedef signed int S32;
typedef unsigned long long U64;
typedef signed long long S64;
#endif

// 流式解压时，该次解压块的序号
typedef enum {
    FIRST_BLOCK,
    LAST_BLOCK,
    OTHER_BLOCK
} BlockStyle;

#endif
