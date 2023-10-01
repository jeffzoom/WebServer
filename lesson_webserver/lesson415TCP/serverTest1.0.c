// TCP 通信的服务器端

#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main() {
    // 1.创建scoket（用于监听的套接字
    // int socket(int domain, int type, int protocol);
    int my_socket = socket (AF_INET, SOCK_STREAM, 0);

    if(my_socket == -1) {
        perror("socket");
        exit(-1);
    }

    // 2.绑定
    // struct sockaddr_in {
    // 1   sa_family_t sin_family; /* __SOCKADDR_COMMON(sin_) */
    // 3   in_port_t sin_port; /* Port number. */
    // 2   struct in_addr sin_addr; /* Internet address. */
    //     /* Pad to size of `struct sockaddr'. */
    //     unsigned char sin_zero[sizeof (struct sockaddr) - __SOCKADDR_COMMON_SIZE -
    //     sizeof (in_port_t) - sizeof (struct in_addr)];
    // }
    struct sockaddr_in my_sockaddr;
    my_sockaddr.sin_family = AF_INET; // 用PF_INET也行，
    // 把主机字节序装换成网络字节序，p是点分十进制，n是网络字节序
    // inet_pton(AF_INET, "192.168.18.167", my_sockaddr.sin_addr.s_addr);
    // 0表示任意地址，0.0.0.0，0指定0是无线网卡或以太网卡，指定0表示两个都绑定，
    // 等客户端进来两个都可以访问
    my_sockaddr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    // 要从主机字节序转换成网络字节序，9999是主机字节序
    my_sockaddr.sin_port = htons(9999);
    
    int ret = bind (my_socket, (struct sockaddr *)&my_sockaddr, sizeof(my_sockaddr));

    if (ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 3.监听
    ret = listen(my_socket, 8);
    if (ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 4.接收客户端连接，accrpt是阻塞函数，有客户端连接进来才会接着往下执行
    struct sockaddr_in clientaddr; // 客户端的地址
    int len = sizeof(clientaddr);
    int cfd = accept(my_socket, (struct sockaddr *)&clientaddr, &len);
    if (cfd == -1) {
        perror("accept");
        exit(-1);
    }

    // 输出客户端的信息，客户端的ip是网络字节序，要转换成主机字节序
    char clientIP[16]; // 储存点分十进制，三个点四个数，每个数最多三位
    inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIP, sizeof(clientIP)); // sin_addr是地址  
    unsigned short clientPort = ntohs(clientaddr.sin_port); // sin_port是端口
    printf("client IP is %s, port is %d\n", clientIP, clientPort);

    // 5.通信
    // 获取客户端数据
    char recvBuf[1024] = {0};

    while(1) {
        int num = read(cfd, recvBuf, sizeof(recvBuf));
        if (num == -1) {
            perror("read");
            exit(-1);
        } else if(num > 0) {
            printf("recv client data : %s\n", recvBuf);
        } else if (num == 0) {
            printf("client close\n");
        }

        // 获取客户端数据
        char * data = "hello, i am server";
        write(cfd, data, strlen(data));
    }
    

    // 关闭文件描述符
    close(cfd);
    close(my_socket); // lfd

    return 0;
}
