/*
��������:����
��������:2010.8
Զ�̿��Ʊ������Ⱥ:30660169,6467438
*/

#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include "..\\share\\head.h"

char * file_read(char *path,unsigned long *length);
int    file_write(char *path,char *src,unsigned long length);
#endif // FILE_H_INCLUDED
