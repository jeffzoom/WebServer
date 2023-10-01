/*
    实现 ps aux | grep xxx 父子进程间通信
    
    子进程： ps aux, 子进程结束后，将数据发送给父进程
    父进程：获取到数据，过滤
    pipe()
    execlp()
    子进程将标准输出 stdout_fileno 重定向到管道的写端。  dup2
*/
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>


int main(){
    int pipefd[2];
    int ret = pipe(pipefd); // 创建管道
    if(ret == -1){
        perror("pipe");
        exit(0);
    }

    pid_t pid = fork(); // 创建子进程
    if(pid > 0){
        // 父进程
        close(pipefd[1]); // 关闭写端
        char buf[1024] = {0};

        int len = -1;
        while((len = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) { 
        // while(read(pipefd[0], buf, sizeof(buf)-1) > 0){          
            printf("%s", buf); // 过滤数据输出
            memset(buf, 0, 1024);
        }
        wait(NULL);
        
    } else if(pid == 0) {
        // 子进程 向管道写数据
        close(pipefd[0]); // 关闭读端  fd1->写端 ---- 读端->fd0 

        // 文件描述符的重定向 stdout_fileno -> fd[1]
        dup2(pipefd[1], STDOUT_FILENO); // #define	STDOUT_FILENO	1
        
        execl("/bin/ps", "ps", "aux", NULL);
        //execlp("ps", "ps", "aux", NULL);
        //write() // 不用这个
        
        perror("execlp");
        exit(0);
    } else {
        perror("fork");
        exit(0);
    }
    return 0;
        
}