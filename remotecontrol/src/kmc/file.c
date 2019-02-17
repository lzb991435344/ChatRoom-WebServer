/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "file.h"

char * file_read(char *path,unsigned long *length)
{
    char * data;
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL;
    fseek(fp, 0, 2);
    *length = ftell(fp);
    fseek(fp, 0, 0);
    data = malloc(*length);
    if (data == NULL){
        fclose(fp);
        return NULL;
    }
    fread(data, 1, *length, fp);
    fclose(fp);
    return data;
}

int file_write(char *path, char *src, unsigned long length)
{
    FILE *fp = fopen(path, "wb+");
    if (fp == NULL)
        return -1;
    fwrite(src, 1, length, fp);
    fclose(fp);
    return 1;
}
