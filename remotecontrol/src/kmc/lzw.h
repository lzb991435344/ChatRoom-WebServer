/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef LZW_H_INCLUDED
#define LZW_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <memory.h>

#define LZW_BASE    0x102   //  The code base
#define CODE_LEN    12      //  Max code length
#define TABLE_LEN   4099    //  It must be prime number and bigger than 2^CODE_LEN=4096.
                            //  Such as 5051 is also ok.
#define DIV         TABLE_LEN
#define HASHSTEP    13         // It should bigger than 0.

typedef struct {
    HANDLE      h_suffix; // Suffix table handle.
    HANDLE      h_prefix; // Prefix table handle.
    HANDLE      h_code;  // Code table handle.

    LPWORD      lp_prefix; // Prefix table head pointer.
    LPBYTE      lp_suffix; // Suffix table head pointer.
    LPWORD      lp_code; // Code table head pointer.

    WORD        code;
    WORD        prefix;
    BYTE        suffix;

    BYTE        cur_code_len; // Current code length.[ used in Dynamic-Code-Length mode ]
}LZW_DATA,*PLZW_DATA;

typedef struct {
    DWORD        top;
    DWORD        index;

    LPBYTE      lp_buffer;
    HANDLE      h_buffer;

    BYTE        by_left;
    DWORD       dw_buffer;

    BOOL        end_flag;
}BUFFER_DATA,*PBUFFER_DATA;

typedef struct {
	WORD        index;
	HANDLE      h_stack;
	LPBYTE      lp_stack;
}STACK_DATA,*PSTACK_DATA;

void lzw_enchode(unsigned char *src,int srclen,unsigned char *dest,int *destlen);
void lzw_dechode(unsigned char *src,int srclen,unsigned char *dest,int *destlen);

#endif // LZW_H_INCLUDED
