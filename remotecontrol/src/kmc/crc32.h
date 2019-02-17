/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef CRC32_H_INCLUDED
#define CRC32_H_INCLUDED

unsigned long crc32(char *buf, size_t size);
unsigned long crc32_file(char *path);

#endif // CRC32_H_INCLUDED
