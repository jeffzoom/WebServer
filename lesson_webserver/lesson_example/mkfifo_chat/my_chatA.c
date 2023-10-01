#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main() {

    // 判断管道文件1是否存在
    int ret = access("fifo_1", F_OK);
    if (ret == -1) {
        printf("管道1不存在，创建管道\n");
        ret = mkfifo("fifo_1", 0664); // 八进制0110110100
        if (ret == -1) {
            perror("mkfifo");
            exit(0);
        }
    }

    // 判断管道文件2是否存在
    ret = access("fifo_2", F_OK);
    if (ret == -1) {
        printf("管道2不存在，创建管道\n");
        ret = mkfifo("fifo_2", 0664); // 八进制0110110100
        if (ret == -1) {
            perror("mkfifo");
            exit(0);
        }
    }

    //////////////////////// 用fd1 和fd2表示文件描述符太垃圾了 
    //////////////////////// 要用可以表示含义的变量名fdr fdw
    // 打开管道
    int fdw = open("fifo_1", O_WRONLY); // 只写
    //fd1=-1;
    if (fdw == -1) {
        perror("open");
        exit(0);
    }
    printf("打开管道fifo_1成功，等待写入...\n");
    int fdr = open("fifo_2", O_RDONLY); // 只读
    if (fdr == -1) {
        perror("open");
        exit(0);
    }
    printf("打开管道fifo_2成功，等待读取...\n");

    char buf[128] = {0};
    while (1) {
        // 先写后读
        // memset(buf, 0, sizeof(buf)-1);
        memset(buf, 0, 128);
        fgets(buf, 128, stdin); // 从指定的流 stream 读取一行，并把它存储在 _s 所指向的字符串内。当读取 (n-1) 个字符时，或者读取到换行符时，或者到达文件末尾时，它会停止，
        ret = write(fdw, buf, strlen(buf));
        if(ret == -1) {
            perror("write");
            exit(0);
        }

        memset(buf, 0, 128);
        // memset(buf, 0, 128);
        // ret = read(fd1, buf, strlen(buf));
        ret = read(fdr, buf, 128);
        if(ret <= 0) {
            perror("read");
            break;
        }
        printf("buf: %s\n", buf);
    }

    // 关闭文件描述符
    close(fdw);
    close(fdr);

    return(0); 
}
