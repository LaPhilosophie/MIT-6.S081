# 前置知识

> 建议听课+熟读xv6手册，并理解清楚lab要求

先切换到syscall分支，并在sh.c中加入代码：

```c
// Execute cmd.  Never returns.*
__attribute__((noreturn)) 
void
runcmd(struct cmd *cmd) {
	.....
}
```

这样做是为了防止报递归错

这个实验要求我们添加两个系统调用，所以我们需要先搞清楚riscv架构的系统调用流程

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/QQ%E6%88%AA%E5%9B%BE20230302004808.png)

用户执行trace命令 - > ecall `<SYS_trace>`  - > 触发trap - > 调用sys_trace

来自用户空间的系统调用、中断、异常都可以引发trap，来自用户空间的trap的处理路径是：

- uservec(kernel/trampoline.S:16)
- usertrap(kernel/trap.c:37)
- usertrapret(kernel/trap.c:90) 返回时
- userret(kernel/trampoline.S:16)

详细的内容见xv6文档

# System call tracing

为了添加trace系统调用，我们需要：

- 在Makefile中添加$U/_trace，这样的话就可以编译出可执行文件，从而在用户命令行中调用trace命令

```makefile
UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\
	$U/_trace
```

- 将trace系统调用的原型添加到 `user/user.h`

```c
int trace(int);
```

- trace的stub添加到 `user/usys.pl`

```c
entry("trace");
```

usys.pl文件会生成*usys.S*文件，形如：

```c
#include "kernel/syscall.h"
.global trace
trace:
 li a7, SYS_trace
 ecall
 ret
```

- 在syscall.h中添加trace的系统调用号

```c
#define SYS_trace  22
```

- 在syscall.c中添加

```c
extern uint64 sys_trace(void);

static uint64 (*syscalls[])(void) = {
	.....
	
	[SYS_trace]   sys_trace,
}
```

- 在proc.h的proc结构体中添加字段mask

```c
// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  struct proc *parent;         // Parent process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  
  int mask;
};
```

- 在sysproc.c中添加sys_trace，照猫画虎，可以参考fork的实现。其中argint获取trace的参数，也即是mask。将当前进程的mask字段设置为要追踪的参数

```c
uint64
sys_trace(void)
{
  int mask;
  if(argint(0, &mask) < 0)//获取trace的参数：mask
    return -1;
	myproc()->mask=mask;
  return 0;
}
```

- 修改 fork ()(参见 kernel/proc.c) ，跟踪掩码从父进程复制到子进程

```c
// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  np->parent = p;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  np->state = RUNNABLE;

  release(&np->lock);

  np->mask=p->mask;//给子进程的mask赋值

  return pid;
}
```

- 添加要索引到的系统调用名称数组

```c
char  *syscall_name[] = {
  "","fork", "exit", "wait", "pipe", "read",
  "kill", "exec", "fstat", "chdir", "dup", "getpid", "sbrk", "sleep",
  "uptime", "open", "write", "mknod", "unlink", "link", "mkdir","close","trace","sysinfo"
};
```

- 修改 kernel/syscall.c 中的 syscall ()函数以打印跟踪输出

```c
void
syscall(void)
{
  int num;
  struct proc *p = myproc();//myproc()：宏，当前的进程
  // trap 代码将用户寄存器保存到当前进程的 trapframe 中
  num = p->trapframe->a7;//num是系统调用号
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();//a0保存返回值
    if((p->mask>>num)&1)
      printf("%d: syscall %s -> %d\n",p->pid,syscall_name[num],p->trapframe->a0);//进程id、系统调用的名称、返回值
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}

/*

$ trace 32 grep hello README
3: syscall read -> 1023
3: syscall read -> 966
3: syscall read -> 70
3: syscall read -> 0

*/
```

# Sysinfo 

In this assignment you will add a system call, `sysinfo`, that collects information about the running system. The system call takes one argument: a pointer to a `struct sysinfo` (see `kernel/sysinfo.h`). The kernel should fill out the fields of this struct: the `freemem` field should be set to the number of bytes of free memory, and the `nproc` field should be set to the number of processes whose `state` is not `UNUSED`. We provide a test program `sysinfotest`; you pass this assignment if it prints "sysinfotest: OK".

任务：添加一个系统调用 sysinfo，它收集关于正在运行的系统的信息

- 系统调用有一个参数: 一个指向 struct sysinfo 的指针(参见 kernel/sysinfo.h)
- 内核应该填充这个结构的字段:
  - freemem 字段应该设置为可用内存的字节数
  - nproc 字段应该设置为状态不是 UNUSED 的进程数

Sysinfo 需要将一个 struct sysinfo 复制回用户空间; 请参阅 sys_fstat ()(kernel/sysfile.c)和 filestat()(kernel/file.c)以获得如何使用 copy out ()实现这一点的示例。

- 添加$U/_sysinfotest
- 为了收集空闲内存量，向 kernel/kalloc.c 添加一个函数getFreeByte()

这块重点是理解run、kmem结构体的实现，并知道访问资源的时候需要加锁。可以参考一下kalloc.c文件中别的函数的实现，很多地方直接套用就行

- kmem.freelist是空闲链表，当链表的next指针不为空时说明有一个空闲资源块，遍历次数就是页的数量
- 由于要求求出字节，因此需要`return PGSIZE*total;`（每页的字节数乘以total）

```c
int getFreeByte()
{
  struct run *p;
  int total=0;

  acquire(&kmem.lock);//加锁
  p=kmem.freelist;
  while(p)
  {
    total++;
    p=p->next;
  }
  release(&kmem.lock);
  return PGSIZE*total;//每页的字节数*total
}
```

- 为了收集进程数，向 kernel/proc.c 添加一个函数getProcNum()

proc.c中有一个全局变量proc[NPROC]，proc[0]是第一个进程，proc[NPROC-1]是最后一个进程，直接for循环遍历即可得到所有的进程结构体的指针，然后取出该指针的state字段和UNUSED比较即可

```c
struct proc proc[NPROC];

struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  struct proc *parent;         // Parent process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  int mask;
};

int getProcNum()
{
  int total=0;
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) 
  {
    acquire(&p->lock);
    if(p->state!=UNUSED)
      total++;
    release(&p->lock);
  }
  return total;
}
```

- 在sysproc.c中添加函数sys_sysinfo(void)
  - 使用argaddr函数 从 trapframe 中以指针的形式检索第 n 个系统调用，关于argaddr函数的用法，在文件的别的函数处可以参考
  - 声明一个sysinfo类型的结构体变量，然后使用之前写好的两个函数对它进行赋值
  - 由于需要将一个sysinfo类型的结构体变量赋值回用户空间，因此需要使用copyout函数，注意这个函数的用法，比较难写对。一定要有`if(xxx) return -1 ;`的形式，不然的话会报错（在这里卡了很久）

```c
uint64
sys_sysinfo(void)
{
  /*int sysinfo(struct sysinfo *)
  struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
  };
  */

  struct proc *p=myproc();
  uint64 addr;
  struct sysinfo res;//注意这里不可以是指针，需要声明一个sysinfo类型的结构体变量

  if(argaddr(0, &addr) < 0)//addr是指向sysinfo结构体的指针
    return -1;
  
  res.freemem=getFreeByte();
  res.nproc=getProcNum();

  if(copyout(p->pagetable, addr, (char*)&res, sizeof(res))<0)
    return -1;

/* 如果这样写会失败
  copyout(p->pagetable, addr, (char*)&res, sizeof(res));
*/
  return 0;
}
```

有一些头文件的添加，函数或者结构体的声明、定义的细节，暂且略过

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/202303212225098.png)
