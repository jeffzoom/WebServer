// 多进程并发服务器
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>

void recyleChild(int arg) { // 回调函数
    while(1) { //需要循环 不然就只有一个未决信号集 只能记录一个子进程结束时发来的信号
        int ret = waitpid(-1, NULL, WNOHANG);
        if(ret == -1) {
            // 所有的子进程都回收了
            break;
        }else if(ret == 0) {
            // 还有子进程活着
            break;
        } else if(ret > 0){
            // 被回收了
            printf("子进程 %d 被回收了\n", ret);
        }
    }
}

int main() {

    // 信号捕捉 SIGCHLD 子进程结束 父进程会收到这个信号 然后回收子进程资源
    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask); //清空信号集中的数据
    act.sa_handler = recyleChild;
    sigaction(SIGCHLD, &act, NULL); //信号捕捉函数
    
    // 1.创建socket
    int lfd = socket(PF_INET, SOCK_STREAM, 0); // lfd 文件标识符
    if(lfd == -1){
        perror("socket");
        exit(-1);
    }

    // 2.绑定
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = INADDR_ANY;
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

    // 不断循环等待客户端连接
    while(1) {

        // 4.接受连接
        struct sockaddr_in cliaddr; // 用来保存接收到客户端的信息
        int len = sizeof(cliaddr);
        int cfd = accept(lfd, (struct sockaddr*)&cliaddr, &len);
        // accept接收客户端连接，默认是一个阻塞的函数，阻塞等待客户端连接，连接失败或断开连接会返回-1

        if(cfd == -1) {
            // 系统调用被有效连接到达之前捕获的信号中断，会出现EINTR错误
            // 应该是捕捉到SIGCHLD信号了吧，服务器子进程结束父进程收到的信号

            // 如果不加if(errno == EINTR)这个判断的话会出现accept: Interrupted system call错误，
            // 然后父进程结束
            // 就是当关闭一个服务器子进程之后，父进程中断，运行子进程终止的回调函数之后，
            // 再到父进程的accept，accept会返回-1
            
            if(errno == EINTR) { 
                continue;        
            }                      
            perror("accept");
            exit(-1);
        }

        // 每一个连接进来，创建一个服务器子进程跟客户端通信
        pid_t pid = fork();
        if(pid == 0) {
            // 进入子进程 获取客户端的信息
            char cliIp[16];
            inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, cliIp, sizeof(cliIp));
            unsigned short cliPort = ntohs(cliaddr.sin_port);
            printf("client ip is : %s, prot is %d\n", cliIp, cliPort);

            // 接收客户端发来的数据
            char recvBuf[1024];
            while(1) {
                int len = read(cfd, &recvBuf, sizeof(recvBuf));

                if(len == -1) {
                    perror("read");
                    exit(-1);
                }else if(len > 0) {
                    printf("recv client : %s\n", recvBuf);
                } else if(len == 0) {
                    printf("client closed....\n");
                    break;
                }
                write(cfd, recvBuf, strlen(recvBuf) + 1); // +1是为了把字符串最后一个\0带上
            }
            close(cfd);
            exit(0);    // 退出当前子进程
        }

    }
    close(lfd);
    return 0;
}