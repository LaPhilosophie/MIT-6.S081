官方文档：https://pdos.csail.mit.edu/6.828/2020/labs/util.html

# Boot xv6

```shell
$ git clone git clone git://g.csail.mit.edu/xv6-labs-2020 # 拉取仓库
$ cd xv6-labs-2020
$ git checkout util # 切换分支
$ make 
$ make qemu
$ make grade # 自动评测所有的程序
$ ./grade-lab-util sleep # 评测单个程序sleep
```

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

> 报错：user/sh.c:58:1: error: infinite recursion detected [-Werror=infinite-recursion]，解决办法：
>
> ```c
> // Execute cmd.  Never returns.*
> __attribute__((noreturn)) 
> void
> runcmd(struct cmd *cmd) {
>  	.....
> }
> ```

# pingpong

Write a program that uses UNIX system calls to ''ping-pong'' a byte between two processes over a pair of pipes, one for each direction. The parent should send a byte to the child; the child should print "<pid>: received ping", where <pid> is its process ID, write the byte on the pipe to the parent, and exit; the parent should read the byte from the child, print "<pid>: received pong", and exit. Your solution should be in the file `user/pingpong.c`.

- int fd[2]创建管道，fd[0]读、fd[1]写
- 管道用法：一般先创建一个管道，然后进程使用fork函数创建子进程，之后父进程关闭管道的读端，子进程关闭管道的写端
- 调用fork 后，父进程的 fork() 会返回子进程的 PID，子进程的fork返回 0
- 注意到write系统调用是`ssize_t write(int fd, const void *buf, size_t count);`，因此写入的字节是一个地址，由于我们声明buf是一个char类型，因此需要填入&buf
- 这里踩了一个坑，把pingpong写成了pingpang导致错误

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

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230208224322.png)

# primes

Write a concurrent version of prime sieve using pipes. This idea is due to Doug McIlroy, inventor of Unix pipes. The picture halfway down [this page](http://swtch.com/~rsc/thread/) and the surrounding text explain how to do it. Your solution should be in the file `user/primes.c`.

Your goal is to use `pipe` and `fork` to set up the pipeline. The first process feeds the numbers 2 through 35 into the pipeline. For each prime number, you will arrange to create one process that reads from its left neighbor over a pipe and writes to its right neighbor over another pipe. Since xv6 has limited number of file descriptors and processes, the first process can stop at 35.

- 使用pipe和fork来设置管道
- 由于xv6文件描述符很少，所以需要关闭所有不必要的文件描述符，否则将会导致描述符耗尽
- 主要的素数进程应该只有在所有的输出都打印出来之后，并且在所有其他的素数进程都退出之后才能退出
- 关于read的用法：当管道的写端关闭时，read 返回零，这个可以控制while的终止条件
- 修改Makefile 的 UPROGS

普通的C语言写法：

```c
void get_primes2(){
    for(int i=2;i<=n;i++){

        if(!st[i]) primes[cnt++]=i;//把素数存起来
        for(int j=i;j<=n;j+=i){//不管是合数还是质数，都用来筛掉后面它的倍数
            st[j]=true;
        }
    }
}

void get_primes1(){
    for(int i=2;i<=n;i++){
        if(!st[i]){
            primes[cnt++]=i;
            for(int j=i;j<=n;j+=i) st[j]=true;//可以用质数就把所有的合数都筛掉；
        }
    }
}
```

并发编程的思路不同于以上方式，我们可以给出伪代码：

```c
p = get a number from left neighbor
print p
loop:
    n = get a number from left neighbor
    if (p does not divide n)
        send n to right neighbor
```

一个生成过程可以将数字2、3、4、 ... ... 35输入管道的左端: 管道中的第一个过程消除了2的倍数，第二个过程消除了3的倍数，第三个过程消除了5的倍数，依此类推

> https://swtch.com/~rsc/thread/

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230227125558.png)



```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

void foo(int *left_fd)
{
    close(left_fd[WRITE]);
    int prime,i;
    read(left_fd[READ],&prime,sizeof(int));//第一个数字必定是素数
    printf("prime %d\n",prime);//符合格式的输出
    
    if(read(left_fd[READ],&i,sizeof(int))!=0)
    {
        int right_fd[2];
        pipe(right_fd);
        int pid=fork();
        if(pid==0)
        {
            foo(right_fd);
        }
        else if(pid>0)
        {
            close(right_fd[READ]);
            if(i%prime!=0)//注意已经读取了一个i，这里要判断一下
            {
                write(right_fd[WRITE],&i,sizeof(int));
            }
            while(read(left_fd[READ],&i,sizeof(int))!=0)
            {
                if(i%prime!=0)
                {
                    write(right_fd[WRITE],&i,sizeof(int));
                }
            }
            close(right_fd[WRITE]);
            close(left_fd[READ]);
            wait(0);//父进程要等待子进程
        }
    }
}

int main(int argc,char * argv[])
{
    int left_fd[2];
    pipe(left_fd);
    
    int pid = fork();
    
    if(pid==0)//child
    {
        foo(left_fd);
    } 
    else if(pid >0)
    {
        close(left_fd[READ]);
        int i;
        for(i=2;i<=35;i++)//第一轮，把2~35全都传递给右边
        {
            write(left_fd[WRITE],&i,sizeof(int));
        }
        close(left_fd[WRITE]);
        wait(0);//主要的素数进程应该只有在所有的输出都打印出来之后，并且在所有其他的素数进程都退出之后才能退出
    }
    
    exit(0);
}
```

# find

Write a simple version of the UNIX find program: find all the files in a directory tree whose name matches a string. Your solution should be in the file `user/find.c`.

- 功能：查找目录树中名称与字符串匹配的所有文件
- 可以参考user/ls.c 以了解如何读取目录
- 避开`.`和`..`的递归

思路：

打开目录，使用while循环得到目录下所有文件/子目录的dirent结构体de

- 如果是de.name是.或者..，那么直接continue跳过
- 如果使用字符串拼接获取文件的相对路径buf
  - 如果buf的st类型是文件类型，且文件名字和我们需要搜索的文件名字相同，那么就直接输出文件信息，这就是我们要找的文件
  - 如果buf的st类型是目录，且不为空，那么就继续递归搜索下去

> 涉及到read、fstat等函数的使用
>
> 涉及到dirent、stat结构体的使用

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char * path,char *file)
{
    int fd;
    struct dirent de;
    struct stat st;
    char buf[512], *p;

    if((fd = open(path, 0)) < 0){//打开目录，获得对应的文件描述符
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    
    while(read(fd, &de, sizeof(de)) == sizeof(de))//使用while循环得到目录下所有文件/子目录的dirent结构体de
    {
        if(strcmp(de.name,".")==0||strcmp(de.name,"..")==0)
            continue;//跳过
        
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        char *pp=de.name;
        while(*pp!=0) //字符串拼接
        {
            *p++ =*pp++;
        }
        *p=0;//别忘了加终止符
        
        if(stat(buf, &st) < 0)//获取buf的st
        {
            printf("find: cannot stat %s\n", buf);
            continue;
        }
        if(st.type==T_FILE)//如果buf的st类型是文件类型
        {
            if(strcmp(de.name,file)==0)//如果文件名字和我们需要搜索的文件名字相同，那么就直接输出文件信息
            {
                printf("%s\n",buf);
            }
        }
        else if(st.type==T_DIR)//如果buf的st类型是目录类型
        {
            if (de.inum == 0) //目录下没有文件，如果缺少这个语句xargs命令将会报错
			    continue;
            find(buf,file);//递归，继续搜索
        }

    }
}

int main(int argc, char *argv[])
{
    char * path=argv[1];//查找目录
    char * file=argv[2];//查找文件

    find(path,file);

    exit(0);
}
```

# xargs

Write a simple version of the UNIX xargs program: read lines from the standard input and run a command for each line, supplying the line as arguments to the command. Your solution should be in the file `user/xargs.c`.

- 对于管道连接的命令，以`find . b | xargs grep hello`为例，argv[0]是 xargs，argv[1]是grep，一共有三个参数，如果想要读取find . b，那么需要从标准输入中读取，也即是使用read函数读取fd为0时的数据
- 每一次读取一行，将该行所有空格替换为\0，这样命令就可以被分割。然后将argv[]指向这些命令。如果遇到换行符，执行fork，父进程等待子进程结束

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

#define STDIN 0

int main(int argc,char *argv[])
{
    char buf[1024];
    char c;
    char *Argv[MAXARG];
    int index=0;
    int i;

    for(i=1;i<=argc-1;i++)
    {
        Argv[i-1]=argv[i];//ignore xargs(argv[0])
    }
    
    while(1)
    {
        index=0;
        memset(buf,0,sizeof(buf));
        char *p=buf;
        i=argc-1;//注意i要写在这里
        while(1)
        {
            int num=read(STDIN,&c,1);//读取标准输入,注意是&c
            if(num!=1)
                exit(0);//程序的终止条件
            if(c==' '||c=='\n')
            {
                buf[index++]='\0';
                Argv[i++]=p;//参数
                p=&buf[index];//更新参数首地址
                if(c=='\n') 
                    break;
            }
            else //character 
            {
                buf[index++]=c;
            }
        }
        Argv[i]=0;
        int pid = fork();
        if(pid==0)
        {
            exec(Argv[0],Argv);
        }
        else
        {
            wait(0);
        }
    }
    exit(0);
}
```



![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/20230301002218.png)