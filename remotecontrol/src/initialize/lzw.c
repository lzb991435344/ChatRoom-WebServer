/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "lzw.h"

void stack_create( PSTACK_DATA stack )
{
	stack->h_stack  = GlobalAlloc( GHND , TABLE_LEN*sizeof(BYTE) );
	stack->lp_stack = (unsigned char*)GlobalLock( stack->h_stack );
	stack->index = 0;
}

void stack_destory( PSTACK_DATA stack )
{
	GlobalUnlock( stack->h_stack );
    GlobalFree  ( stack->h_stack );
}

void re_init_lzw( PLZW_DATA lzw )    //When code table reached its top it should
{                                    //be reinitialized.
    memset( lzw->lp_code, 0xFFFF, TABLE_LEN*sizeof(WORD) );
    lzw->code          = LZW_BASE;
    lzw->cur_code_len  = 9;
}

void lzw_create(PLZW_DATA    lzw)
{
    lzw->h_code        = GlobalAlloc( GHND, TABLE_LEN*sizeof(WORD) );
    lzw->h_prefix      = GlobalAlloc( GHND, TABLE_LEN*sizeof(WORD) );
    lzw->h_suffix      = GlobalAlloc( GHND, TABLE_LEN*sizeof(BYTE) );
    lzw->lp_code       = (unsigned short*)GlobalLock( lzw->h_code   );
    lzw->lp_prefix     = (unsigned short*)GlobalLock( lzw->h_prefix );
    lzw->lp_suffix     = (unsigned char*)GlobalLock( lzw->h_suffix );
    lzw->code          = LZW_BASE;
    lzw->cur_code_len  = 9;
    memset( lzw->lp_code, 0xFFFF, TABLE_LEN*sizeof(WORD) );
}

void lzw_destory(PLZW_DATA    lzw)
{
    GlobalUnlock( lzw->h_code   );
    GlobalUnlock( lzw->h_prefix );
    GlobalUnlock( lzw->h_suffix );

	GlobalFree( lzw->h_code  );
    GlobalFree( lzw->h_prefix );
    GlobalFree( lzw->h_suffix );
}

WORD get_hash_index( PLZW_DATA lzw )
{
    DWORD tmp;
    WORD result;
    DWORD prefix;
    DWORD suffix;
    prefix = lzw->prefix;
    suffix = lzw->suffix;
    tmp = prefix<<8 | suffix;
    result = (unsigned short)(tmp % DIV);
    return result;
}

WORD re_hash_index( WORD hash ) // If hash conflict occured we must recalculate
{                               // hash index .
    WORD result;
    result = hash + HASHSTEP;
    result = result % DIV;
    return result;
}

BOOL in_table( PLZW_DATA lzw ) // To find whether current code is already in table.
{
    BOOL result;
    WORD hash;

    hash = get_hash_index( lzw );
    if ( lzw->lp_code[ hash ] == 0xFFFF ) {
        result = FALSE;
    } else {
        if( lzw->lp_prefix[ hash ] == lzw->prefix &&
            lzw->lp_suffix[ hash ] == lzw->suffix )
        {
            result = TRUE;
        }
		else
		{
            result = FALSE;
            while ( lzw->lp_code[ hash ] != 0xFFFF )
			{
                if( lzw->lp_prefix[ hash ] == lzw->prefix &&
                    lzw->lp_suffix[ hash ] == lzw->suffix )
                {
                        result = TRUE;
                        break;
                }
                hash = re_hash_index( hash );
            }
        }
    }
    return result;
}

WORD get_code( PLZW_DATA lzw )
{
    WORD hash;
    WORD code;
    hash = get_hash_index( lzw );
    if( lzw->lp_prefix[ hash ] == lzw->prefix &&
        lzw->lp_suffix[ hash ] == lzw->suffix )
    {
        code = lzw->lp_code[ hash ];
    } else {
        while( lzw->lp_prefix[ hash ] != lzw->prefix ||
               lzw->lp_suffix[ hash ] != lzw->suffix )
		{
                hash = re_hash_index( hash );
        }
        code = lzw->lp_code[ hash ];
    }
    return code;
}

void insert_table( PLZW_DATA lzw )
{
    WORD hash;
    hash = get_hash_index( lzw );
    if ( lzw->lp_code[ hash ] == 0xFFFF ) {
        lzw->lp_prefix[ hash ] = lzw->prefix;
        lzw->lp_suffix[ hash ] = lzw->suffix;
        lzw->lp_code[ hash ]   = lzw->code;
    } else {
        while( lzw->lp_code[ hash ] != 0xFFFF ) {
                hash = re_hash_index( hash );
        }
        lzw->lp_prefix[ hash ] = lzw->prefix;
        lzw->lp_suffix[ hash ] = lzw->suffix;
        lzw->lp_code[ hash ]   = lzw->code;
    }

}

void output_code( DWORD code ,PBUFFER_DATA out, PLZW_DATA lzw)
{
    out->dw_buffer |= code << ( 32 - out->by_left - lzw->cur_code_len );
    out->by_left += lzw->cur_code_len;

    while ( out->by_left >= 8 ) {
        out->lp_buffer[ out->index++ ] = (BYTE)( out->dw_buffer >> 24 );
        out->dw_buffer <<= 8;
        out->by_left -= 8;
    }
}

void do_encode( PBUFFER_DATA in, PBUFFER_DATA out, PLZW_DATA lzw)
{
    WORD prefix;
    while ( in->index != in->top ) {
        if ( !in_table(lzw) ) {
			// current code not in code table
			// then add it to table and output prefix
			insert_table(lzw);
			prefix = lzw->suffix;
			output_code( lzw->prefix ,out ,lzw );
			lzw->code++;

			if ( lzw->code == (WORD)1<< lzw->cur_code_len ) {
				// code reached current code top(1<<cur_code_len)
				// then current code length add one
				lzw->cur_code_len++;
				if ( lzw->cur_code_len == CODE_LEN + 1 ) {
					re_init_lzw( lzw );
				}
			}
        } else {
			// current code already in code table
			// then output nothing
			prefix = get_code(lzw);
        }
        lzw->prefix = prefix;
        lzw->suffix = in->lp_buffer[ in->index++ ];
    }
}

void out_code( WORD code ,PBUFFER_DATA buffer,PLZW_DATA lzw,PSTACK_DATA stack)
{
	WORD tmp;
	if ( code < 0x100 ) {
		stack->lp_stack[ stack->index++ ] = (unsigned char)code;
	} else {
		stack->lp_stack[ stack->index++ ] = lzw->lp_suffix[ code ];
		tmp = lzw->lp_prefix[ code ];
		while ( tmp > 0x100 ) {
			stack->lp_stack[ stack->index++ ] = lzw->lp_suffix[ tmp ];
			tmp = lzw->lp_prefix[ tmp ];
		}
		stack->lp_stack[ stack->index++ ] = (BYTE)tmp;
	}

	while ( stack->index ) {
		buffer->lp_buffer[ buffer->index++ ] = stack->lp_stack[ --stack->index ] ;
	}
}

void insert_2_table(PLZW_DATA lzw )
{
	lzw->lp_code[ lzw->code ]   = lzw->code;
	lzw->lp_prefix[ lzw->code ] = lzw->prefix;
	lzw->lp_suffix[ lzw->code ] = lzw->suffix;
	lzw->code++;

	if ( lzw->code == ((WORD)1<<lzw->cur_code_len)-1 ) {
		lzw->cur_code_len++;
		if( lzw->cur_code_len == CODE_LEN+1 )
		  lzw->cur_code_len = 9;
	}
	if (lzw->code >= 1<<CODE_LEN ) {
		re_init_lzw(lzw);
	}

}

WORD get_next_code( PBUFFER_DATA buffer , PLZW_DATA lzw )
{
	BYTE next;
	WORD code;
	while ( buffer->by_left < lzw->cur_code_len ) {
		next = buffer->lp_buffer[ buffer->index++ ];
		buffer->dw_buffer |= (DWORD)next << (24-buffer->by_left);
		buffer->by_left   += 8;
	}
	code = (unsigned short)(buffer->dw_buffer >> ( 32 - lzw->cur_code_len ));
	buffer->dw_buffer <<= lzw->cur_code_len;
	buffer->by_left    -= lzw->cur_code_len;

	return code;
}

void do_decode( PBUFFER_DATA in, PBUFFER_DATA out, PLZW_DATA lzw, PSTACK_DATA stack)
{
	WORD code;
	WORD tmp;
	while ( in->index != in->top  ) {
		//printf("%d:%d\n",in->index,in->top);
		//Sleep(1);
		code = get_next_code( in ,lzw );

		if ( code < 0x100 ) {
			// code already in table
			// then simply output the code
			lzw->suffix = (BYTE)code;
		} else {
			if ( code < lzw->code  ) {
				// code also in table
				// then output code chain
				tmp = lzw->lp_prefix[ code ];
				while ( tmp > 0x100 ) {
					tmp = lzw->lp_prefix[ tmp ];
				}
				lzw->suffix = (BYTE)tmp;
			} else {
				// code == lzw->code
				// code not in table
				// add code into table
				// and out put code
				tmp = lzw->prefix;
				while ( tmp > 0x100 ) {
					tmp = lzw->lp_prefix[ tmp ];
				}
				lzw->suffix = (BYTE)tmp;
			}
		}
		insert_2_table( lzw );
		out_code(code,out,lzw,stack);
		lzw->prefix = code;
	}
}

void lzw_enchode(unsigned char *src,int srclen,unsigned char *dest,int *destlen)
{
	LZW_DATA        lzw;
    BUFFER_DATA     in ;
    BUFFER_DATA     out;

    lzw_create( &lzw);
    in.lp_buffer  = src;
    in.top        = srclen;
    in.index      = 0;
    in.by_left    = 0;
    in.dw_buffer  = 0;
    in.end_flag   = FALSE;

    out.lp_buffer  = dest;
    out.top        = *destlen;
    out.index      = 0;
    out.by_left    = 0;
    out.dw_buffer  = 0;
    out.end_flag   = FALSE;

	lzw.prefix = in.lp_buffer[ in.index++ ];
	lzw.suffix = in.lp_buffer[ in.index++ ];

    do_encode(&in , &out, &lzw);

	output_code(lzw.prefix, &out , &lzw);
	output_code(lzw.suffix, &out , &lzw);

	if ( out.by_left ) {
		out.lp_buffer[ out.index++ ] = (BYTE)( out.dw_buffer >> (32-out.by_left) ) << (8-out.by_left);
	}

	*destlen = out.index;
    lzw_destory( &lzw );
}

void lzw_dechode(unsigned char *src,int srclen,unsigned char *dest,int *destlen)
{
	LZW_DATA        lzw;
	BUFFER_DATA     in ;
	BUFFER_DATA     out;
	STACK_DATA      stack;
	BOOL   first_run;

	first_run = TRUE;

    lzw_create( &lzw);
	stack_create(&stack );

	in.lp_buffer  = src;
    in.top        = srclen;
    in.index      = 0;
    in.by_left    = 0;
    in.dw_buffer  = 0;
    in.end_flag   = FALSE;

    out.lp_buffer  = dest;
    out.top        = *destlen;
    out.index      = 0;
    out.by_left    = 0;
    out.dw_buffer  = 0;
    out.end_flag   = FALSE;

	lzw.prefix = get_next_code( &in, &lzw );
	lzw.suffix = (unsigned char)lzw.prefix;
	out_code(lzw.prefix, &out, &lzw , &stack);

    do_decode(&in , &out, &lzw, &stack);

	if ( out.by_left ) {
		out.lp_buffer[ out.index++ ] = (BYTE)( out.dw_buffer >> (32-out.by_left) ) << (8-out.by_left);
	}

	*destlen = out.index;

    stack_destory( &stack);
    lzw_destory( &lzw );
}
/*
//------------------------------------------------------------------------------
//1311812
char * file_read(char *path,int *outlen)
{
	char *fdata = NULL;
	FILE *fi = fopen(path,"rb");
    if(fi != NULL){
        fseek(fi,0,2);
        *outlen = ftell(fi);
        fseek(fi,0,0);

        fdata = (char*)malloc(*outlen+1);
        memset(fdata,0x0,*outlen+1);
        fread(fdata,1,*outlen,fi);
        fclose(fi);
    }
	return fdata;
}

void file_write(char *path,char *data, int len)
{
	FILE *fi = fopen(path,"wb");
    if(fi != NULL){
        fwrite(data,1,len,fi);
        fclose(fi);
    }
}

CHAR*  file_name_in = "c:\\aa.bmp";
CHAR*  file_name_out= "c:\\testaatestaaa.bmp";
CHAR*  file_name    = "c:\\decode_k.bmp";

int main(int argc, char *argv[])
{
	int srclen =0;
	char *src;
	src = file_read(file_name_in, &srclen);
	int destlen = 5024000;
	char *dest = (char*)malloc(destlen);;
	test_enchode((unsigned char*)src, srclen, (unsigned char*)dest, &destlen);
	file_write(file_name_out, dest, destlen);
	//encode(h_file_sour, h_file_dest);

//	h_file     = file_handle(file_name);
//	decode(h_file_dest,h_file);

	int srclen =0;
	char *src;
	src = file_read(file_name_out, &srclen);
	int destlen = 5024000;
	char *dest = (char*)malloc(destlen);;
	test_dechode((unsigned char*)src, srclen, (unsigned char*)dest, &destlen);
	file_write(file_name, dest, destlen);

    free(src);
    free(dest);
    return 0;
}
*/
