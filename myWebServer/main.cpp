/*
 * @version: 
 * @Author: zsq 1363759476@qq.com
 * @Date: 2023-04-06 11:06:48
 * @LastEditors: zsq 1363759476@qq.com
 * @LastEditTime: 2023-06-08 10:55:04
 * @FilePath: /Linux_nc/WebServer_202304/myWebServer/main.cpp
 * @Descripttion: 
 */

// addsig 处理信号
// main里面提示输入格式
// 程序模拟proactor模式
/*
在主线程监听有没有数据到达
在主线程中把数据读出来，然后封装成一个任务类，交给子线程，就是工作线程，子线程再取任务做任务
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "http_connt.h" 
#include "lockerp.h"
#include "threadpools.h"

// 先编译: g++ *.cpp -o myWebServer20230417 -pthread
// 运行:  ./myWebServer20230417 10000
// 网站的根目录，浏览器用这个请求: http://192.168.18.187:10000/index.html，如果修改文件路径记得改doc_root的http_connt.cpp的路径

// #define _XOPEN_SOURCE // 想让struct straction sa;不报错来着，原来是sigaction写成了straction
#define MAX_FD 65536 // 最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000 // 监听的最大的事件数量

// using namespace std;

// 添加文件描述符到epoll中
extern void addfd(int epollfd, int fd, bool one_shot); // extern也行啊，这样就不用在http_conn.h中声明了，但是我觉得不好，因为要是多个文件用addfd，每用一次就要extern，在.h中声明反而方便
// 从epoll中删除文件描述符
extern void removedfd(int epollfd, int fd);

// 处理SIGPIPE信号，若不处理就会终止进程
// SIGPIPE 编号13 Broke，pipe向没有读端的管道写数据，默认动作：终止进程
// 一端断开连接，另外一端还写数据的话，就会产生SIGPIPE信号
// sig是要处理的函数，void(handler)(int)是信号处理函数，一个函数指针
void addsig(int sig, void(handler)(int)) {                   
    // struct straction sa; // 错误 sigaction写成了straction
    struct sigaction sa;            // 信号捕捉函数
    memset(&sa, '\0', sizeof(sa));  // memset(变量名, 起始字节, 结束字节)清空数据
    sa.sa_handler = handler; // 回调函数：handler(任意名)，被函数指针指向（管理）
    sigfillset(&sa.sa_mask); // 将信号集中的所有标志位置为1，设置临时阻塞信号集，临时都是阻塞的
                             // sa.sa_mask临时阻塞信号集，在信号捕捉函数执行过程中，临时阻塞某些信号
    // sigaction(sig, &sa, nullptr); // 注册信号捕捉 
    assert(sigaction(sig, &sa, NULL) != -1); // assert中，若值为假，那么它先向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行
}

int main(int argc, char *argv[]) {  // 程序模拟的是proacter模式
    if (argc <= 1) {                // 端口号没有输入的错误提示
        // cout << "usage:" << basename(argv[0]) << "port_number" << endl;  
        printf( "usage: %s port_number\n", basename(argv[0]));
        exit(-1);
    }

    // int port = *argv[1]; 
    int port = atoi(argv[1]); // 指定端口号 把参数str所指向的字符串转换为一个整数（类型为int型）

    // 对SIGPIPE信号处理，SIG_IGN 忽略此信号，SIG_DFL是使用信号的默认行为
    addsig(SIGPIPE, SIG_IGN); // 一端断开连接，另外一端还写数据的话，就会产生SIGPIPE信号

    // 创建线程池
    threadpool<http_conn> *pool =nullptr;
    try {
        pool = new threadpool<http_conn>;
    } catch (...) {
        return 1;
    }

    http_conn * users = new http_conn[MAX_FD]; // 不懂，应该跟http建立连接有关吧

    // 1.创建一个套接字        错误  SOCK_DGRAM 之前写成了这个，然后就perror("listen")了
    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // listenfd就是监听的文件描述符
    
    // 2.绑定，将 fd 和本地的IP + 端口进行绑定
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port); // 端口

    int reuse = 1; // 要在绑定之前设置端口复用
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); // 端口复用
    // SO_REUSEADDR，打开或关闭地址复用功能

    int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    if(ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 3.监听
    ret = listen(listenfd, 5);
    if(ret == -1) {
        perror("listen"); // 之前socket(AF_INET, SOCK_STREAM, 0)写成了SOCK_DGRAM
        exit(-1);
    }
    
    // 将监听的文件描述符相关的检测信息添加到epoll实例中
    int epollfd = epoll_create(5);
    epoll_event events[MAX_EVENT_NUMBER]; // 可以写成struct epoll_event吗

    addfd(epollfd, listenfd, false); // 监听的文件描述符不用EPOLLONESHOT，因为本来就只触发一次，接收数据的文件描述符需要EPOLLONESHOT
    // 一般监听文件描述符listenfd不设置为边沿触发EPOLLET

    // 静态数量，默认是0
    http_conn::m_epollfd = epollfd;

    while (true) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        // epoll_wait默认是阻塞的，产生信号就会中断
        // 当epoll_wait中断结束返回，就会返回-1，以及出现出现EINTR
        if ((number < 0) && errno != EINTR) { // 在中断情况下会出现EINTR
            printf("epoll failure\n");
            break;
        }

        // 遍历循环事件数组
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;

            if (sockfd == listenfd) { // 客户端请求连接
                // 4.连接
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                // client_address是传出参数，记录了连接成功后客户端的地址信息（ip，port）
                
                if ( connfd < 0 ) {
                    printf( "errno is: %d\n", errno );
                    continue;
                } 

                if( http_conn::m_user_count >= MAX_FD ) { // MAX_FD最大的文件描述符个数
                    // 目前连接数满了
                    // 给客户端写一个信息：服务器内部正忙  优化
                    close(connfd);
                    continue;
                }

                // 将新的客户的数据初始化，放到数组中
                // connfd是连接的文件描述符
                users[connfd].init(connfd, client_address);

            } else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) { // 出现下列事件，
                // 对方异常断开或者错误等事件 EPOLLRDHUP代表对端断开连接
                // EPOLLERR：表示对应的文件描述符发生错误；EPOLLHUP：表示对应的文件描述符被挂断；
                users[sockfd].close_conn(); // 为什么是users[sockfd]，不懂

            } else if(events[i].events & EPOLLIN) { // epoll检测到读事件，有读事件发生
                if (users[sockfd].read()) { // 数据过来了，一次性把数据处理完
                    pool->append(users + sockfd); // 添加到线程池队列当中
                } else { 
                    users[sockfd].close_conn(); // 读失败了，就断开连接吧
                }
                
            } else if(events[i].events & EPOLLOUT) { // 有写事件发生  写缓冲区没有满，就可以写
                if (!users[sockfd].write()) { // 一次性把数据写完，把数据写出去，发送数据
                    // 写失败了，就断开连接吧
                    users[sockfd].close_conn();
                }
            }
        }
    }

    close(epollfd);
    close(listenfd); // #include <unistd.h>
    delete[] users;
    delete pool;

    return 0;
}
