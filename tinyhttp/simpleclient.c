#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
 int sockfd;
 int len;
 struct sockaddr_in address;
 int result;
 char ch = 'A';

 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 address.sin_family = AF_INET;//协议族
 address.sin_addr.s_addr = inet_addr("127.0.0.1");//ip地址
 address.sin_port = htons(9734);//端口号
 len = sizeof(address);

 //connect连接
 result = connect(sockfd, (struct sockaddr *)&address, len);

//connect出错
 if (result == -1)
 {
  perror("oops: client1");
  exit(1);
 }
 //通过socketfd读写
 write(sockfd, &ch, 1);//将A字符写入sockfd
 read(sockfd, &ch, 1);//读取字符A
 printf("char from server = %c\n", ch);
 close(sockfd);//关闭socket
 exit(0);
}
