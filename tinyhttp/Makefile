all: httpd

httpd: httpd.c
	gcc -W -Wall -lsocket -lpthread -o httpd httpd.c

/**参数的含义
 -w 不生成任何警告信息
 -Wall 生成所有警告信息
 -lsocket/-lpthread 
 -o File 生成指定的输出文件
 -share 生成共享库
*/
clean:
	rm httpd
