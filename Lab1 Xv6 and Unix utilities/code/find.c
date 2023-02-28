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
        fprintf(2, "find ERROR: cannot open %s\n", path);
        return;
    }
    
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    
    while(read(fd, &de, sizeof(de)) == sizeof(de))
    {
        
        if(strcmp(de.name,".")==0||strcmp(de.name,"..")==0)
            continue;//跳过
        
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        char *pp=de.name;
        while(*pp!=0)
        {
            *p++ =*pp++;
        }
        *p=0;
        
        if(stat(buf, &st) < 0)
        {
            printf("find: cannot stat %s\n", buf);
            continue;
        }
        if(st.type==T_FILE)
        {
            if(strcmp(de.name,file)==0)
            {
                printf("%s\n",buf);
            }
        }
        else if(st.type==T_DIR)
        {
            if (de.inum == 0) //目录下没有文件
			    continue;
            find(buf,file);
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