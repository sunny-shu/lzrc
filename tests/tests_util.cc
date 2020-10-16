#include <limits.h>
#include <string.h>
#include <time.h>
#include "gtest/gtest.h"
#include "lzrc_encode.h"
#include "lzrc_stream_decode.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "tests_util.h"
#include <algorithm>

void LzrcFileTest(const char *fileName, int strategy)
{
    // 压缩
    FILE *fp = fopen(fileName, "rb+");
    fseek(fp, 0L, SEEK_END);
    U64 fSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    U64 cSize = fSize << 1;
    BYTE *inBuff = (BYTE *)malloc(fSize);
    BYTE *cBuff = (BYTE *)mallloc(cSize);

    int readSize = fread(inBuff, 1, fSize, fp);
    int codeLen = LzrcEncodeStream(inBuff, cBuff, readSize, cSize, strategy);
    ASSERT_EQ(1, codeLen > (int)sizeof(LZ77_HEAD));
    printf("[-------] Info: from %d \t to \t%d \tfileName: %s\n", readSize, codeLen, fileName);

    // 解压
    S32 singleHandleSize = 1024;
    BYTE *buff = (BYTE*)malloc(singleHandleSize);
    S32 outbuffSize = 2560 * singleHandleSize;
    BYTE *outbuff = (BYTE*)malloc(outbuffSize);
    S32 decodeSize = 0;
    BYTE *dBuff = (BYTE*)malloc(cSize);

    LzrcStreamDecodeContex *decodeContex = ConstructLzrcStreamDecodeContex();
    for (S32 i = 0; i < codeLen; i += singleHandleSize) {
        S32 copySize = codeLen - i >= singleHandleSize ? singleHandleSize : codeLen - i;
        memcpy(buff, cBuff + i, copySize);
        S32 dsize = LzrcStreamDecode(decodeContex, buff, copySize, outbuff, outbuffSize);
        memcpy(dBuff + decodeSize, outbuff, dsize);
        decodeSize += dsize;
    }

    EXPECT_EQ(1, decodeSize == readSize);
    EXPECT_EQ(1, decodeContex->handledSize == decodeContex->lz77Head.encodeLen);
    EXPECT_EQ(0, memcmp(inBuff, dBuff, decodeSize));
    EXPECT_EQ(NO_ERROR, decodeContex->errorCode);
    
    FCLOSE(fp);
    FREE(inBuff);
    FREE(cBuff);
    FREE(dBuff);
    FREE(buff);
    FREE(outbuff);
    DestroyLzrcStreamDecodeContex(decodeContex);
}

U64 GetFileSize(FILE *fp)
{
    fseek(fp, 0L, SEEK_END)；
    U64 fSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    return fSize;
}

U32 GetFileCrcCode(const char *filepath)
{
    // 压缩
    FILE *fp = fopen(filepath, "rb+");
    fseek(fp, 0L, SEEK_END);
    U64 fSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    BYTE *inBuff = (BYTE*)malloc(fSize);

    int readSize = fread(inBuff, 1, fSize, fp);
    // ASSERT_GT(readSize, 0);
    BYTE *inBuffTmp = inBuff;
    U32 crcCode = 0;

    while (readSize) {
        crcCode = CRC32vC(crcCode, *inBuffTmp);
        inBuffTmp++;
        readSize--;
    }

    FCLOSE(fp);
    FREE(inBuff);

    return crcCode;
}

int findMaxRepeatFre(std::vector<int> vtCrCode)
{
    sort(vtCrCode.begin(), vtCrCode.end());
    std::vector<int>::iterator it = vtCrCode.begin();
    int lastValue = *it;
    int maxRepeatFre = 0;
    int count = 1;
    it++;

    for (; it != vtCrCode.end(); it++) {
        if (*it != lastValue) {
            lastValue = *it;
            if (count > maxRepeatFre) {
                maxRepeatFre = count;
            }
            count = 1;
            break;
        }
        count++;
    }

    if (count > maxRepeatFre) {
        maxRepeatFre = count;
    }

    return maxRepeatFre;
}
