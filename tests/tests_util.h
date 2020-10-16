#ifndef TEST_UTIL_H
#define TEST_UTIL_H

// 文件压缩解压
void LzrcFileTest(const char *fileName, int strategy);

// 获取文件大小
U64 GetFileSize(FILE *fp);

// 获取文件crc码
U32 GetFileCrcCode(const char *filepath);

// 查找vector数组中出现频率最高的频率数
int findMaxRepeatFre(std::vector<int> vtCrCode);

#endif
