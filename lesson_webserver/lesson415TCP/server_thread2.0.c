// 多线程并发服务器
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// 最早期的面向对象，将数据封装，然后传入子线程的回调函数
struct sockInfo { 
    int fd; // 通信的文件描述符
    struct sockaddr_in addr; // 客户端信息
    pthread_t tid;  // 线程号
};

struct sockInfo sockinfos[128]; // 创建一个数组 表示最多可容纳128个客户端 全局变量

void * working(void * arg) { // 回调函数
    // 子线程和客户端通信   cfd 客户端的信息 线程号
    // 获取客户端的信息
    struct sockInfo * pinfo = (struct sockInfo *)arg;

    char cliIp[16];
    inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, cliIp, sizeof(cliIp)); // 将网络字节序的整数，转换成点分十进制的IP地址字符串
    unsigned short cliPort = ntohs(pinfo->addr.sin_port); // 主机字节序转换为网络字节序
    printf("client ip is : %s, prot is %d\n", cliIp, cliPort);

    // 接收客户端发来的数据
    char recvBuf[1024];
    while(1) {
        int len = read(pinfo->fd, &recvBuf, sizeof(recvBuf)); // 读数据 存到recvBuf中

        if (len == -1) {
            perror("read");
            exit(-1);
        } else if (len > 0) {
            printf("recv client : %s\n", recvBuf);
        } else if (len == 0) {
            printf("client closed....\n");
            break;
        }
        write(pinfo->fd, recvBuf, strlen(recvBuf) + 1); // +1是为了把字符串最后一个\0带上
    }
    close(pinfo->fd);
    return NULL;
}

int main() {

    // 1.创建socket
    int lfd = socket(PF_INET, SOCK_STREAM, 0);
    printf("lfd:%d\n", lfd);
    if(lfd == -1){
        perror("socket");
        exit(-1);
    }

    // 2.绑定
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999); // 主机字节序到网络字节序的转换函数，9999是主机字节序，端口
    saddr.sin_addr.s_addr = INADDR_ANY; //任意地址
    int ret = bind(lfd,(struct sockaddr *)&saddr, sizeof(saddr));
    if(ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 3.监听
    ret = listen(lfd, 128);
    if(ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 初始化sockInfo数组
    int max = sizeof(sockinfos) / sizeof(sockinfos[0]); // 总元素长度/单个元素长度 就是128
    for(int i = 0; i < max; i++) {
        bzero(&sockinfos[i], sizeof(sockinfos[i])); // 能够将内存块（字符串）的前n个字节清零
        sockinfos[i].fd = -1; // 文件描述符是-1 说明此文件描述符无效
        sockinfos[i].tid = -1; // 线程号-1 也是无效 所以说是初始化
    }

    // 循环等待客户端连接，一旦一个客户端连接进来，就创建一个子线程进行通信
    while(1) {

        struct sockaddr_in cliaddr; // 用来保存接收到客户端的信息
        int len = sizeof(cliaddr);
        // 4.连接客户端
        int cfd = accept(lfd, (struct sockaddr*)&cliaddr, &len);

        struct sockInfo * pinfo; // 回调函数的传入参数
        // 不能struct sockInfo pinfo这样设计，因为在while循环中是局部变量
        // 用malloc到堆中的话，又需要手动释放，会增加开发难度

        // 从这个数组中找到一个可以用的sockInfo元素 如果sockinfos[i].fd == -1就是还没用过的
        for(int i = 0; i < max; i++) {
            if(sockinfos[i].fd == -1) {
                pinfo = &sockinfos[i];
                break;
            }
            if(i == max - 1) { // 如果128个都用完了 就1秒 然后再循环看有没有可以分配的
                sleep(1);
                i--;
            }
        }

        pinfo->fd = cfd; // 文件描述符
        memcpy(&pinfo->addr, &cliaddr, len); // 拷贝数据 &pinfo->addr是拷贝的目标 就是要拷贝到哪去

        // 创建子线程
        pthread_create(&pinfo->tid, NULL, working, pinfo);

        // 不能用pthread_join回收资源 因为会阻塞
        pthread_detach(pinfo->tid); // 线程分离
    }

    close(lfd);
    return 0;
}
