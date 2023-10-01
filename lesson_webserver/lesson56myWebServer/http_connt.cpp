#include "http_connt.h"

using namespace std;

// 错误：静态成员记得初始化，不然会报错
int http_conn::m_epollfd = -1; // 所有的socket上的事件都被注册到同一个epol
int http_conn::m_user_count = 0; // 统计用户的数量

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL); // 获取文件描述符状态flag
    int new_option = old_option | O_NONBLOCK; // 设置为非阻塞
    fcntl(fd, F_SETFL, new_option); // 这样就把fd设置为非阻塞了？是的

    // fcntl(fd, F_SETFL, fcntl(fd,F_GETFL) | O_NONBLOCK); // 一步到位

    return old_option; // 其实没有用到这个返回值
}

// 向epoll中添加需要监听的文件描述符
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP; // 优化，好的服务器既可以水平触发又可以边沿触发，设置边沿触发要加入EPOLLET
                                         // 这个貌似是默认，就是水平触发，epoll的LT模式（水平触发）和ET模式（边沿触发）
    // 对方连接断开，会触发EPOLLIN和EPOLLRDHUP两个事件，EPOLLRDHUP就是若异常断开，会交给底层处理，上层不用处理，不懂为什么要加EPOLLRDHUP
    if (one_shot) { 
        // 防止同一个通信被不同的多个线程处理
        // 对于注册了 EPOLLONESHOT 事件的文件描述符，操作系统最多触发其上注册的一个可读、可写，
        // 或者异常事件，且只触发一次
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

    // 用epoll或者是ET边沿触发模式，是一次性把所有数据都读出来，读的时候应该是非阻塞的
    // 要不然没有数据的时候就会阻塞在那了，不懂
    // 设置文件描述符为非阻塞
    setnonblocking(fd);
}

// 从epoll中移除监听的文件描述符，从epollfd中移除fd
void removedfd(int epollfd, int fd) {
    // epoll_ctl(epollfd, fd, EPOLL_CTL_DEL, nullptr); // 优化，nullptr应该也行吧
    epoll_ctl(epollfd, fd, EPOLL_CTL_DEL, 0);
    close(fd);
}

// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP; // ev应该是自己的事件的值，或了之后就达到了修改文件描述符的目的
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}

void http_conn::init(int sockfd, const sockaddr_in &client_address) {
    m_sockfd = sockfd;
    m_address = client_address;

    // 端口复用 不懂 不明白为什么在这也要端口复用，因为是新端口吗？
    int reuse = 1; // 要在绑定之前设置端口复用
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); // 端口复用

    // 将数据传输的文件描述符加入到epoll中
    addfd(m_epollfd, m_sockfd, true); //epoll实例的文件描述符，要检测的文件描述符
    m_user_count++; // 总用户数+1
}

// 关闭连接
void http_conn::close_conn() {
    printf("测试close测试\n");
    if (m_sockfd != -1) {
        removedfd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

// 非阻塞的读 一次性读完数据
bool http_conn::read() {
    printf("读\n");
    // if (m_read_idx >= READ_BUFFER_SIZE) { //要让程序健壮一点
    //     return false;
    // }

    // // 读取到的字节
    // int bytes_read = 0;
    // while (true) {
    //     bytes_read = recv();
    //     if (bytes_read == -1) {
            
    //     }
    // }
}

// 非阻塞的写 一次性写完数据
bool http_conn::write() {
    printf("写\n");
}

/*
    逻辑：当有请求发送到服务器时，一次性把数据读完users[sockfd].read()，之后调用
    pool->append(users + sockfd)把任务追加到线程池中，然后线程池就会执行run()，
    不断的循环从队列里面去取，取到一个就执行任务的process()，执行process()就是做业务处理，
    解析，然后生成响应，就是生成响应的数据，数据有了之后，如果要写数据的话，events[i].events & EPOLLOUT
    就users[sockfd].write()一次性把数据写完，这个就是整体流程

    真正的业务处理：read()，write()，还有就是process()

*/
// 处理客户端请求
// 由线程池中的工作线程调用，这是处理HTTP请求的入口
void http_conn::process() {
    // 解析http请求，这里就是做业务逻辑的，处理一行一行的http请求，请求头什么的

    printf("process\n");

    // 生成响应
}



