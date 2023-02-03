## Boot xv6

见环境配置

# sleep

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