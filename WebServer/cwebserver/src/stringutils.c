#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "stringutils.h"

#define STRING_SIZE_INC 64

//�ַ�����չ����,���·����ڴ�ռ�
void string_extend(string *s, size_t new_len) {
    assert(s != NULL);

    if (new_len >= s->size) {//���ڵ�ǰ���ַ�������
        s->size += new_len - s->size;
		//��չ��64λ
        s->size += STRING_SIZE_INC - (s->size % STRING_SIZE_INC);
        s->ptr = realloc(s->ptr, s->size);//���·���ռ�
    }
}

//��ʼ���ַ���
string* string_init() {
    string *s;
    s = malloc(sizeof(*s));//����ռ�
    s->ptr = NULL;//ָ����Ϊ��
    s->size = s->len = 0;//��������Ϊ0
    
    return s;
}

string* string_init_str(const char *str) {
    string *s = string_init();
    string_copy(s, str);

    return s;
}

void string_free(string *s) {
    if (!s) return;

    free(s->ptr);//����ָ��
    free(s);
}

void string_reset(string *s) {
    assert(s != NULL);

    if (s->size > 0) {
        s->ptr[0] = '\0';
    }
    s->len = 0;
}

//�����ַ���
int string_copy_len(string *s, const char *str, size_t str_len) {
	//���ȶ���
    assert(s != NULL);
    assert(str != NULL);

    if (str_len <= 0) return 0;

	//��1��ԭ����'\0'
    string_extend(s, str_len + 1);
	//�����ַ���
    strncpy(s->ptr, str, str_len);
	//���¸�ֵ�ַ����ĳ���
    s->len = str_len;
	//���'\0'
    s->ptr[s->len] = '\0';

    return str_len;//�����ַ����ĳ���
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
        buf[len++] = digits[i % 10];//���ֵ�����������0-9
        i = i / 10;
    }

    if (minus)
        buf[len++] = '-';

    for (int i = len - 1; i >= 0; i--) {
        string_append_ch(s, buf[i]);
    }

    return len;
    
}

//�ַ�����չ���ȣ����س���
int string_append_len(string *s, const char *str, size_t str_len) {
	//�жϲ���
    assert(s != NULL);
    assert(str != NULL);
    if (str_len <= 0) return 0;

	//����չ����
    string_extend(s, s->len + str_len + 1);

	//memcpy()�ַ���������
    memcpy(s->ptr + s->len, str, str_len);
    s->len += str_len;//���ӳ���
    s->ptr[s->len] = '\0';//���'\0'

    return str_len;//���س���
}

int string_append(string *s, const char *str) {
    return string_append_len(s, str, strlen(str));
}

//����µ��ַ�
int string_append_ch(string *s, char ch) {
    assert(s != NULL);

	//��չ�ĳ���+2
    string_extend(s, s->len + 2);

    s->ptr[s->len++] = ch;//���ַ��Ž��ַ�����
    s->ptr[s->len] = '\0';//���'\0'

    return 1;
}

