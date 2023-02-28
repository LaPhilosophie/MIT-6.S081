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