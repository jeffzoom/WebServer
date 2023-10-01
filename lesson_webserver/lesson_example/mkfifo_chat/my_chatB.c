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

    // 打开管道
    int fdr = open("fifo_1", O_RDONLY); // 只读
    if (fdr == -1) {
        perror("open");
        exit(0);
    }
    printf("打开管道fifo_1成功，等待写入...\n");
    int fdw = open("fifo_2", O_WRONLY); // 只写
    if (fdw == -1) {
        perror("open");
        exit(0);
    }
    printf("打开管道fifo_2成功，等待读取...\n");

    char buf[128] = {0};
    while (1) {
        // 先读后写
        memset(buf, 0, 128);
        // memset(buf, 0, 128);
        // ret = read(fd1, buf, strlen(buf));
        ret = read(fdr, buf, 128);
        if(ret <= 0) {
            perror("read");
            exit(0);
        }
        printf("buf: %s\n", buf);

        memset(buf, 0, 128);
        // memset(buf, 0, 128);
        fgets(buf, 128, stdin); // 从指定的流 stream 读取一行，并把它存储在 _s 所指向的字符串内。当读取 (n-1) 个字符时，或者读取到换行符时，或者到达文件末尾时，它会停止，
        ret = write(fdw, buf, strlen(buf));
        if(ret == -1) {
            perror("write");
            break;
        }
    }

    // 关闭文件描述符
    close(fdr);
    close(fdw);

    return(0); 
}
