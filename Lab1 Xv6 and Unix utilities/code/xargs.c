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