/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef IP_INFO_H_INCLUDED
#define IP_INFO_H_INCLUDED

#include "km_head.h"

typedef struct tag_ip_info{
    char         *file;
    char         *p;
    unsigned int total;
}IP_INFO,*P_IP_INFO;

unsigned int str2ip(const char *lp);

int initialize_ip_info();
int find_ip_info(unsigned long ip,char *outval);
int de_initialize_ip_info();
#endif // IP_INFO_H_INCLUDED
