
// 在这里解析http请求报文

#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // 内存映射
#include <arpa/inet.h>
#include <assert.h>

#include "lockerp.h"

using namespace std;

class http_conn {

public:
    // 所有文件的连接是共享一个epollfd，就是epoll文件描述符
    static int m_epollfd;                       // 所有的socket上的事件都被注册到同一个epol

    static int m_user_count;                    // 统计用户的数量
    static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小

public:
    http_conn() {}  // 错误：如果在最后加了“;”，编译会提示错误：main.cpp:(.text+0x1d8)：对‘http_conn::http_conn()’未定义的引用
    ~http_conn() {}
public:
    void process();     // 处理客户端请求
    void init(int sockfd, const sockaddr_in &client_address); // 初始化新接收的连接 
    void close_conn();  // 关闭连接

    bool read();        // 非阻塞的读
    bool write();       // 非阻塞的写

private:
    // 一边写一边考虑，慢慢的往里面追加
    int m_sockfd;                       // 该HTTP连接的socket和对方的socket地址
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];  // 读缓冲区
    int m_read_idx;     // 标识读缓冲区中已经读入的客户端数据的最后一个字节的下一个位置
};

// 错误：加了下面这个编译报错：main.cpp:(.text+0x0): `http_conn::http_conn()'被多次定义
// http_conn::http_conn(/* args */) {
// }
// http_conn::~http_conn() {
// }

#endif
