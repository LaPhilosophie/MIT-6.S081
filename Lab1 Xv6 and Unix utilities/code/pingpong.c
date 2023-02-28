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
