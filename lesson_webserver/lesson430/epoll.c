#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

int main() {

    // 创建socket
    int lfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in saddr;
    saddr.sin_port = htons(9999);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;

    // 绑定
    bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr));

    // 监听
    listen(lfd, 8);

    // 调用epoll_create()创建一个epoll实例
    int epfd = epoll_create(100);

    // 将监听的文件描述符lfd相关的检测信息添加到epoll实例中，加到rbr红黑树中
    struct epoll_event epev;
    epev.events = EPOLLIN; // 检测读事件
    epev.data.fd = lfd; // 要监听的文件描述符
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &epev);

    struct epoll_event epevs[1024]; // 创建数组 用来保存内核检测完返回的数据

    while(1) {

        // 告诉内核，让内核去检测rbr中有哪个文件描述符的信息已经EPOLLIN（数据到达）了
        int ret = epoll_wait(epfd, epevs, 1024, -1); 
        if(ret == -1) {
            perror("epoll_wait");
            exit(-1);
        }

        printf("ret = %d\n", ret);

        for(int i = 0; i < ret; i++) {

            int curfd = epevs[i].data.fd;
            
            // if和else，其实就是先if有客户端连接，然后再else连接客户端之后有数据到达，读数据
            // if 连接客户端
            // else 客户端有数据发送到服务器
            if(curfd == lfd) { // 监听的文件描述符有数据达到
                // 监听的文件描述符有数据达到，有客户端连接
                struct sockaddr_in cliaddr;
                int len = sizeof(cliaddr);
                int cfd = accept(lfd, (struct sockaddr *)&cliaddr, &len);

                // 添加文件描述符cfd，检测这个新的文件描述符是否有读事件
                epev.events = EPOLLIN; // epev.events可能是EPOLLIN，也有可能是EPOLLOUT
                epev.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &epev);

                printf("if: cfd:%d, lfd:%d, curfd:%d\n", cfd, lfd, curfd); // 测试来的文件描述符是什么
            } else {
                if(epevs[i].events & EPOLLOUT) {
                    continue;
                }
                // 有数据到达，需要通信    EPOLLIN读事件发生改变    EPOLLOUT写事件发生改变
                printf("else: lfd:%d, curfd:%d\n", lfd, curfd); // 测试来的文件描述符是什么

                char buf[1024] = {0};
                int len = read(curfd, buf, sizeof(buf));
                if(len == -1) {
                    perror("read");
                    exit(-1);
                } else if(len == 0) {
                    printf("client closed...\n");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
                    close(curfd);
                } else if(len > 0) {
                    printf("read buf = %s\n", buf);
                    write(curfd, buf, strlen(buf) + 1);
                }
            }
        }
    }

    close(lfd);
    close(epfd); // 要关闭 不然占用一个文件描述符 浪费资源
    return 0;
}