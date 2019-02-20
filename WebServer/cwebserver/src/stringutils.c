#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "stringutils.h"

#define STRING_SIZE_INC 64

//
//字符串扩展长度,重新分配内存空间
void string_extend(string *s, size_t new_len) {
    assert(s != NULL);

    if (new_len >= s->size) {//大于当前的字符串长度
        s->size += new_len - s->size;
		//扩展到64位
        s->size += STRING_SIZE_INC - (s->size % STRING_SIZE_INC);
        s->ptr = realloc(s->ptr, s->size);//重新分配空间
    }
}

//初始化字符串
string* string_init() {
    string *s;
    s = malloc(sizeof(*s));//分配空间
    s->ptr = NULL;//指针置为空
    s->size = s->len = 0;//长度设置为0
    
    return s;
}

string* string_init_str(const char *str) {
    string *s = string_init();
    string_copy(s, str);

    return s;
}

void string_free(string *s) {
    if (!s) return;

    free(s->ptr);//销毁指针
    free(s);
}

void string_reset(string *s) {
    assert(s != NULL);

    if (s->size > 0) {
        s->ptr[0] = '\0';
    }
    s->len = 0;
}

//复制字符串
int string_copy_len(string *s, const char *str, size_t str_len) {
	//首先断言
    assert(s != NULL);
    assert(str != NULL);

    if (str_len <= 0) return 0;

	//加1的原因是'\0'
    string_extend(s, str_len + 1);
	//复制字符串
    strncpy(s->ptr, str, str_len);
	//重新赋值字符串的长度
    s->len = str_len;
	//添加'\0'
    s->ptr[s->len] = '\0';

    return str_len;//返回字符串的长度
}

int string_copy(string *s, const char *str) {
    return string_copy_len(s, str, strlen(str));
}

int string_append_string(string *s, string *s2) {
    assert(s != NULL);
    assert(s2 != NULL);

    return string_append_len(s, s2->ptr, s2->len);
}

int string_append_int(string *s, int i) {
    assert(s != NULL);
    char buf[30];
    char digits[] = "0123456789";
    int len = 0;
    int minus = 0;

    if (i < 0) {
        minus = 1;
        i *= -1;
    } else if (i == 0) {
        string_append_ch(s, '0');
        return 1;
    }
    
    while (i) {
        buf[len++] = digits[i % 10];//数字的索引定义是0-9
        i = i / 10;
    }

    if (minus)
        buf[len++] = '-';

    for (int i = len - 1; i >= 0; i--) {
        string_append_ch(s, buf[i]);
    }

    return len;
    
}

//字符串扩展长度，返回长度
int string_append_len(string *s, const char *str, size_t str_len) {
	//判断参数
    assert(s != NULL);
    assert(str != NULL);
    if (str_len <= 0) return 0;

	//先扩展长度
    string_extend(s, s->len + str_len + 1);

	//memcpy()字符串的内容
    memcpy(s->ptr + s->len, str, str_len);
    s->len += str_len;//增加长度
    s->ptr[s->len] = '\0';//添加'\0'

    return str_len;//返回长度
}

int string_append(string *s, const char *str) {
    return string_append_len(s, str, strlen(str));
}

//添加新的字符
int string_append_ch(string *s, char ch) {
    assert(s != NULL);

	//扩展的长度+2
    string_extend(s, s->len + 2);

    s->ptr[s->len++] = ch;//把字符放进字符数组
    s->ptr[s->len] = '\0';//添加'\0'

    return 1;
}

