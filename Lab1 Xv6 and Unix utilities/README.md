# Boot xv6

见环境配置

# sleep

Implement the UNIX program `sleep` for xv6; your `sleep` should pause for a user-specified number of ticks. A tick is a notion of time defined by the xv6 kernel, namely the time between two interrupts from the timer chip. Your solution should be in the file `user/sleep.c`.

- 在Makefile中的UPROG下面加入sleep
- 在user目录创建sleep.c文件
- sleep.c加入的三个头文件是模仿了其他user/目录的风格
- 首先进行参数的检查，异常则退出
- 直接调用sleep()函数即可，可以在别的程序中找到这种用法
- exit(0)退出程序

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char * argv[])
{
    if(argc != 2){
        fprintf(2, "Usage: sleep [times]\n");
        exit(1);
    }
    sleep(atoi(argv[1]));
    exit(0);
}
```

# pingpong

Write a program that uses UNIX system calls to ''ping-pong'' a byte between two processes over a pair of pipes, one for each direction. The parent should send a byte to the child; the child should print "<pid>: received ping", where <pid> is its process ID, write the byte on the pipe to the parent, and exit; the parent should read the byte from the child, print "<pid>: received pong", and exit. Your solution should be in the file `user/pingpong.c`.

- int fd[2]创建管道，fd[0]读、fd[1]写
- 管道用法：一般先创建一个管道，然后进程使用fork函数创建子进程，之后父进程关闭管道的读端，子进程关闭管道的写端
- 调用fork 后，父进程的 fork() 会返回子进程的 PID，子进程的fork返回 0
- 注意到write系统调用是`ssize_t write(int fd, const void *buf, size_t count);`，因此写入的字节是一个地址，由于我们声明buf是一个char类型，因此需要填入&buf

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230208224322.png)

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char * argv[])
{
    int fd1[2],fd2[2];
    pipe(fd1);
    pipe(fd2);
    int pid = fork();
    
    // child
    // print "<pid>: received ping"
    // write back to parent a byte and exit 
    // close fd[1]
    // zero for read and 1 for read 
    if(pid==0)  
    {
        close(fd1[1]);//close write 
        close(fd2[0]);//close read 
        char buf[3];
        read(fd1[0],&buf,1);//read a byte from parent
        printf("%d: received ping\n",getpid()); 
        write(fd2[1],&buf,1);//write back to parent 
        exit(0);
    }
    
    // parent
    // send a byte to the child 
    // close fd[0]
    else 
    {
        close(fd1[0]);//close read 
        close(fd2[1]);//close write 
        char buf[3];
        write(fd1[1],"A",1);// send a byte to the child
        read(fd2[0],&buf,1);
        printf("%d: received pong\n",getpid()); 
    }
    exit(0);
}

```

