#include <arpa/inet.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include "stringutils.h"
#include "log.h"

// 以append模式打开日志文件
void log_open(server *serv, const char *logfile) {
    if (serv->use_logfile) {
        serv->logfp = fopen(logfile, "a");

		//无日志文件
        if (!serv->logfp) 
        {
            perror(logfile);
            exit(1);
        }
        return;
    }

    openlog("webserver", LOG_NDELAY | LOG_PID, LOG_DAEMON);
}

// 关闭日志文件
void log_close(server *serv) 
{
    if (serv->logfp)
        fclose(serv->logfp);
    closelog();//打开的日志也是资源，需要回收
}

// 生成时间戳字符串
static void date_str(string *s)
{
	/**
	struct tm
  { 　
    int tm_sec;	//秒–取值区间为[0,59] 　　
	int tm_min; //分 - 取值区间为[0,59] 　　
	int tm_hour; //时 - 取值区间为[0,23] 　　
	int tm_mday;//一个月中的日期 - 取值区间为[1,31] 
	int tm_mon;	//月份（从一月开始，0代表一月） - 取值区间为[0,11] 
	int tm_year; //年份，其值从1900开始 
	int tm_wday; // 星期–取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推  　
	int tm_yday; //从每年的1月1日开始的天数–取值区间为[0,365]，其中0代表1月1日，1代表1月2日，以此类推 　
	int tm_isdst; //夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的进候，tm_isdst为0；不了解情况时，tm_isdst()为负。 　
	long int tm_gmtoff;	 //指定了日期变更线东面时区中UTC东部时区正秒数或UTC西部时区的负秒数 　　
	const char *tm_zone; //当前时区的名字(与环境变量TZ有关) 　
   };
	*/
    struct tm *ti;//结构体指针
    time_t rawtime;
    char local_date[100];
    char zone_str[20];
    int zone;
    char zone_sign;
	/**
	time_t time(time_t *t)
	参数：seconds -- 这是指向类型为 time_t 的对象的指针，用来存储 seconds 的值。
	返回值：以time_t 对象返回当前日历时间。
	*/
    time(&rawtime);
	/**
	
	说明：此函数获得的tm结构体的时间是日历时间。
    用 法: struct tm *localtime(const time_t *clock);
    返回值：返回指向tm 结构体的指针.tm结构体是time.h中定义的用于分别存储时间的各个量(年月日等)的结构体.*/
    ti = localtime(&rawtime);
    zone = ti->tm_gmtoff / 60;

    if (ti->tm_zone < 0) 
    {
        zone_sign = '-';
        zone = -zone;
    } else
        zone_sign = '+';
    
    zone = (zone / 60) * 100 + zone % 60;

	/**
	#include <time.h>
   size_t strftime(char *str, size_t count, const char *format, const struct tm *tm);
   strftime()函数可以把YYYY-MM-DD HH:MM:SS格式的日期字符串转换成其它形式的字符串。
   strftime()的语法是strftime(格式, 日期/时间, 修正符, 修正符, ...)
     str, 表示返回的时间字符串
     count, 要写入的字节的最大数量
     format, 格式字符串由零个或多个转换符和普通字符(除%)
     tm, 输入时间
	*/
    strftime(local_date, sizeof(local_date), "%d/%b/%Y:%X", ti);
	/**
	int snprintf(char *str, size_t size, const char *format, ...)。
	将可变个参数(...)按照format格式化成字符串，然后将其复制到str中
    (1) 如果格式化后的字符串长度 < size，则将此字符串全部复制到str中，并给其后添加一个字符串
     结束符('\0')；
    (2) 如果格式化后的字符串长度 >= size，则只将其中的(size-1)个字符复制到str中，并给其后添加一个
     字符串结束符('\0')，返回值为欲写入的字符串长度。
	*/
    snprintf(zone_str, sizeof(zone_str), " %c%.4d", zone_sign, zone);

    string_append(s, local_date);
    string_append(s, zone_str);
}

// 记录HTTP请求
void log_request(server *serv, connection *con) {
    http_request *req = con->request;
    http_response *resp = con->response;
    char host_ip[INET_ADDRSTRLEN];
    char content_len[20];
    string *date = string_init();

    if (!serv || !con)
        return;

    if (resp->content_length > -1 && req->method != HTTP_METHOD_HEAD) {
        snprintf(content_len, sizeof(content_len), "%d", resp->content_length); 
    } else {
        strcpy(content_len, "-");
    }

    inet_ntop(con->addr.sin_family, &con->addr.sin_addr, host_ip, INET_ADDRSTRLEN);
    date_str(date);

    // 日志中需要记录的项目：IP，时间，访问方法，URI，版本，状态，内容长度
    if (serv->use_logfile) 
    {
        fprintf(serv->logfp, "%s - - [%s] \"%s %s %s\" %d %s\n",
                host_ip, date->ptr, req->method_raw, req->uri,
                req->version_raw, con->status_code, content_len);
        fflush(serv->logfp);
    } 
    else
    {
        syslog(LOG_ERR, "%s - - [%s] \"%s %s %s\" %d %s",
                host_ip, date->ptr, req->method_raw, req->uri,
                req->version_raw, con->status_code, content_len);
    }
    string_free(date);
}

// 写入日志函数
//参数1:指向服务器结构体的指针,参数2:类型,参数3:格式,参数4:变参的变量
static void log_write(server *serv, const char *type, const char *format, va_list ap) 
{
    string *output = string_init();//把时间和消息类型写到字符串指针out

    // 写入时间，消息类型
    if (serv->use_logfile) 
    {
        string_append_ch(output, '[');
        date_str(output);//时间戳
        string_append(output, "] ");
    }

    string_append(output, "[");
    string_append(output, type);//类型
    string_append(output, "] ");
    
    string_append(output, format);

    if (serv->use_logfile) 
    {//使用日志文件
        string_append_ch(output, '\n');
        vfprintf(serv->logfp, output->ptr, ap);
        fflush(serv->logfp);//刷新流
    } 
    else 
    {
		/**
		void syslog(int priority, const char * message, ...);
		LOG_ERR：错误发生
		*/
        vsyslog(LOG_ERR, output->ptr, ap);
    }
    string_free(output);
}

// 记录出错信息
void log_error(server *serv, const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    log_write(serv, "error", format, ap);
    va_end(ap);
}

// 记录日志信息
void log_info(server *serv, const char *format, ...) 
{
	/**valist作用:用于解决变参数的一组宏
	#include <stdarg.h>，用于获取不确定个数的参数。
   （1）首先在函数里定义一具VA_LIST型的变量，这个变量是指向参数的指针；
   （2）然后用VA_START宏初始化刚定义的VA_LIST变量；
   （3）然后用VA_ARG返回可变的参数，VA_ARG的第二个参数是你要返回的参数的类型（如果函数有多个可变参数的，依次调用VA_ARG获取各个参数）；
   （4）最后用VA_END宏结束可变参数的获取
	*/
    va_list ap;
    va_start(ap, format);
    log_write(serv, "info", format, ap);
    va_end(ap);
}
