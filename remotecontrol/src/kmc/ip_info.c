/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "ip_info.h"
#include "file.h"

P_IP_INFO p_ip_info = NULL;

int initialize_ip_info()
{
    char g_qqwry_path[MAX_PATH];
    sprintf(g_qqwry_path, "%sipaddr", g_share_main->km_cur_path);

    unsigned long length;

    p_ip_info = malloc(sizeof(IP_INFO));
    if (p_ip_info == NULL)
        return -1;
    memset(p_ip_info, 0x0, sizeof(IP_INFO));

    p_ip_info->file = file_read(g_qqwry_path, &length);
    if (p_ip_info->file == NULL)
        return -1;

    p_ip_info->total = (*((unsigned int*)p_ip_info->file+1) - *(unsigned int*)p_ip_info->file);
    if (p_ip_info->total % 7 != 0)
		return -1;
    p_ip_info->total /= 7;
    p_ip_info->total++;
    p_ip_info->p = p_ip_info->file + *(unsigned int*)p_ip_info->file;
    return 1;
}

inline unsigned int get_3b(const char *mem)
{
	return 0x00ffffff & *(unsigned int*)(mem);
}

unsigned int str2ip(const char *lp)
{
	unsigned int ret = 0;
	unsigned int now = 0;

	while (*lp){
		if ('.' == *lp){
			ret = 256 * ret + now;
			now = 0;
		}
		else
			now = 10 * now + *lp - '0';
		++lp;
	}
	ret = 256 * ret + now;
	return ret;
}

int find_ip_info(unsigned long ip,char *outval)
{
    if (NULL == p_ip_info || NULL == p_ip_info->file){
		strcpy(outval,"IP文件错误");
		return 1;
	}

    char *ptr = p_ip_info->file;
    char *p = p_ip_info->p ;
	char *now_p;
    char *ret[2];

	unsigned int begin = 0, end = p_ip_info->total;
	while (1){
		if ( begin >= end - 1 )
			break;
		if ( ip < *(unsigned int*)(p + (begin + end)/2 * 7) )
			end = (begin + end)/2;
		else
			begin = (begin + end)/2;
 	}

	unsigned int temp = get_3b(p + 7 * begin + 4);
	if (ip <= *(unsigned int*)(ptr + temp)){
		now_p = ptr + temp + 4;
		if ( 0x01 == *now_p )
			now_p = ptr + get_3b(now_p + 1);
		//country
		if ( 0x02 == *now_p ){
			ret[0] = ptr + get_3b(now_p + 1);
			now_p += 4;
		}else{
			ret[0] = now_p;
			for (; *now_p; ++now_p)
				;
			++now_p;
		}
 		//local
 		if ( 0x02 == *now_p ) //jump
			ret[1] = ptr + get_3b(now_p + 1);
		else
			ret[1] = now_p;

        sprintf(outval, "%s(%s)", ret[0], ret[1]);
	}else{
		strcpy(outval,"未知");
	}
    return 0;
}

int de_initialize_ip_info()
{
    if (p_ip_info != NULL){
        if (p_ip_info->file != NULL)
            free(p_ip_info->file);
        free(p_ip_info);
    }
    return 0;
}
