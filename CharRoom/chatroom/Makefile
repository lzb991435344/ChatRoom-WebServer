CC = g++ //使用的编译工具
CFLAGS = -std=c++11  //C++的版本

all: ClientMain.cpp ServerMain.cpp Server.o Client.o  //这里是所有涉及到的文件
	$(CC) $(CFLAGS) ServerMain.cpp  Server.o -o chatroom_server //-o是生成的文件名字
	$(CC) $(CFLAGS) ClientMain.cpp Client.o -o chatroom_client

Server.o: Server.cpp Server.h Common.h
	$(CC) $(CFLAGS) -c Server.cpp //gcc -c + 文件名，生成.o文件

Client.o: Client.cpp Client.h Common.h
	$(CC) $(CFLAGS) -c Client.cpp

clean:  
	rm -f *.o chatroom_server chatroom_client  //清除所有.o文件，*是通配符
