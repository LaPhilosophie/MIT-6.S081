# 前置准备

- 看完[p4、p5](https://www.bilibili.com/video/BV19k4y1C7kA)

- 搞清楚[lab要求](https://pdos.csail.mit.edu/6.S081/2020/labs/traps.html)
- 对riscv指令集、函数调用约定、trap机制有一定理解

# RISC-V assembly ([easy](https://pdos.csail.mit.edu/6.S081/2020/labs/guidance.html))

通过make fs.img 命令可以将user/call.c文件转化为user/call.asm文件

```c
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
  printf("%d %d\n", f(8)+1, 13);
  exit(0);
}

```

不难观察出：

1. `s0`寄存器（也称为`fp`或帧指针）：它用于保存当前函数的帧指针，即指向当前函数栈帧的基址。在函数调用过程中，`s0`寄存器通常被用于存储上一个函数的栈帧基址，以便在函数返回时恢复调用者的栈帧。
2. `sp`寄存器（也称为堆栈指针）：它用于指示当前的栈顶位置，即栈指针。栈是一种后进先出（LIFO）的数据结构，用于存储局部变量、函数调用信息和其他临时数据。在函数调用期间，栈会动态增长和收缩，而`sp`寄存器会随着栈的变化而移动。
3. a0保存第一个参数，同时具有存储返回值的用途（也即是x，返回x+3）。a1存储第二个参数，以此类推
4. f函数被内联优化，没有新开辟栈帧，它的内容和g函数相同
5. ra寄存器存储函数的返回地址

```asm

user/_call:     file format elf64-littleriscv


Disassembly of section .text:

0000000000000000 <g>:
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
   0:	1141                	addi	sp,sp,-16 #给栈指针分配内存
   2:	e422                	sd	s0,8(sp) #保存s0寄存器的值
   4:	0800                	addi	s0,sp,16 
  return x+3;
}
   6:	250d                	addiw	a0,a0,3 # a0是第一个参数，也即是x，返回x+3
   8:	6422                	ld	s0,8(sp)
   a:	0141                	addi	sp,sp,16
   c:	8082                	ret

000000000000000e <f>:
#f函数被内联优化，没有新开辟栈帧，它的内容和g函数相同
int f(int x) {
   e:	1141                	addi	sp,sp,-16
  10:	e422                	sd	s0,8(sp)
  12:	0800                	addi	s0,sp,16
  return g(x);
}
  14:	250d                	addiw	a0,a0,3
  16:	6422                	ld	s0,8(sp)
  18:	0141                	addi	sp,sp,16
  1a:	8082                	ret

000000000000001c <main>:

void main(void) {
  1c:	1141                	addi	sp,sp,-16 # 初始化工作
  1e:	e406                	sd	ra,8(sp)
  20:	e022                	sd	s0,0(sp)
  22:	0800                	addi	s0,sp,16
  printf("%d %d\n", f(8)+1, 13);
  24:	4635                	li	a2,13
  26:	45b1                	li	a1,12 # f(8)+1
  28:	00000517          	auipc	a0,0x0
  2c:	79050513          	addi	a0,a0,1936 # 7b8 <malloc+0xea>
  30:	00000097          	auipc	ra,0x0
  34:	5e6080e7          	jalr	1510(ra) # 616 <printf>
  exit(0);
  38:	4501                	li	a0,0
  3a:	00000097          	auipc	ra,0x0
  3e:	274080e7          	jalr	628(ra) # 2ae <exit>

```



## Q1

Which registers contain arguments to functions? For example, which register holds 13 in main's call to `printf`?

```
reg    | name  | saver  | description
-------+-------+--------+------------
x0     | zero  |        | hardwired zero
x1     | ra    | caller | return address
x2     | sp    | callee | stack pointer
x3     | gp    |        | global pointer
x4     | tp    |        | thread pointer
x5-7   | t0-2  | caller | temporary registers
x8     | s0/fp | callee | saved register / frame pointer
x9     | s1    | callee | saved register
x10-11 | a0-1  | caller | function arguments / return values
x12-17 | a2-7  | caller | function arguments
x18-27 | s2-11 | callee | saved registers
x28-31 | t3-6  | caller | temporary registers
pc     |       |        | program counter
```

a0保存第一个参数，同时具有存储返回值的用途（也即是x，返回x+3）。a1存储第二个参数，以此类推

## Q2

Where is the call to function `f` in the assembly code for main? Where is the call to `g`? (Hint: the compiler may inline functions.)

代码`li	a1,12 `直接将f(8)+1的结果存入a1中，并没有调用函数的过程，这里存在编译器的优化

## Q3

At what address is the function `printf` located?

```
0000000000000616 <printf>:

void
printf(const char *fmt, ...)
{
 616:	711d                	addi	sp,sp,-96
```

616

> `jalr 1510(ra)`指令用于调用`printf`函数，其中偏移量1510是相对于全局地址0x0**的偏移**。（全局地址不确定）

## Q4

Run the following code.

```
	unsigned int i = 0x00646c72;
	printf("H%x Wo%s", 57616, &i);
     
```

What is the output? [Here's an ASCII table](http://web.cs.mun.ca/~michael/c/ascii-table.html) that maps bytes to characters.

The output depends on that fact that the RISC-V is little-endian. If the RISC-V were instead big-endian what would you set `i` to in order to yield the same output? Would you need to change `57616` to a different value?

[Here's a description of little- and big-endian](http://www.webopedia.com/TERM/b/big_endian.html) and [a more whimsical description](http://www.networksorcery.com/enp/ien/ien137.txt).

57616是0xE110

%s打印出&i指向十六进制数对应的ASCII码，遇0停止打印。变量`i`存储了字符串"rld\0"

最终打印出HE110 World



## Q5

In the following code, what is going to be printed after `'y='`? (note: the answer is not a specific value.) Why does this happen?

```
	printf("x=%d y=%d", 3);
```

y=a2寄存器的值

这里printf函数相当于有三个参数，第一个是字符串（a0保存），第二个是3（a1保存），第三个（（a2保存））

# Backtrace ([moderate](https://pdos.csail.mit.edu/6.S081/2020/labs/guidance.html))

要求：**在 kernel/printf.c 中实现一个 backtrace ()函数。在 sys _ sleep 中插入对此函数的调用，然后运行 bttest，它调用 sys _ sleep。你的输出应该如下:**

```
backtrace:
0x000000080002cda
0x000000080002bb6
0x0000000080002898
```

编译器在每个堆栈帧中放入一个包含调用者帧指针地址的帧指针。您的回溯应该使用这些帧指针来遍历堆栈，并在每个堆栈帧中打印保存的返回地址。

> 补充一下基础知识：
>
> 栈指针：sp（stack pointer），指向栈的低地址
>
> 帧指针：fp（frame pointer），指向栈的高地址
>
> fp-8：返回地址
>
> fp-16：preview fp
>
> 内核会为栈分配一页的内存，因此可以使用 PGROUNDUP（fp）和 PGROUNDDOWN（fp）定位栈的上下边界，以终止循环

具体的栈的内存布局参考lecture中教授手画图

- 将backtrace的原型添加到 kernel/defs.h，这样就可以在 sys _ sleep 中调用backtrace
  - 在defs.h中添加`void backtrace(void);`

- GCC 编译器将当前正在执行的函数的帧指针存储在寄存器 s0中。向 kernel/riscv.h 添加以下函数，并在backtrace中调用这个函数来读取当前帧指针。这个函数使用内联来读取 s0：

```asm
static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x) );
  return x;
}
```

在printf.c文件中添加函数：

```c
void backtrace(void)
{
  uint64 fp=r_fp();//get fp 
  uint64 up_addr=PGROUNDUP(fp);//up bound
  printf("backtrace:\n");//fmt output 
  while(fp<up_addr)
  {
    uint64 ret_addr=*(uint64 *)(fp-8);
    printf("%p\n",ret_addr);// attention that after %p is \n
    fp=*(uint64 *)(fp-16);
  }
}
```

bttest.c中如是写：

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  sleep(1);
  exit(0);
}

```

因此bttest作为测试文件会调用sleep，也即是sys_sleep。我们在sys_sleep中加入如下代码：

```c
uint64
sys_sleep(void)
{
  int n;
  uint ticks0;
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace();// 
  return 0;
}
```

这样的话，sleep返回之前会调用backtrace函数

类似的，为了更好的debug，在panic函数中加入如下代码：

```c
void
panic(char *s)
{
  pr.locking = 0;
  printf("panic: ");
  printf(s);
  printf("\n");
  backtrace();
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}
```

这样会在panic输出之后，打印出返回地址

> ps ：做到此处发现了的qemu卡死的问题，发现是2020版本lab的传统艺能。apt remove qemu*之后使用源码编译qemu5版本即可
>
> Next, retrieve and extract the source for QEMU 5.1.0:
>
> ```
> $ wget https://download.qemu.org/qemu-5.1.0.tar.xz
> $ tar xf qemu-5.1.0.tar.xz
> ```
>
> Build QEMU for riscv64-softmmu:
>
> ```
> $ cd qemu-5.1.0
> $ ./configure --disable-kvm --disable-werror --prefix=/usr/local --target-list="riscv64-softmmu"
> $ make
> $ sudo make install
> $ cd ..
> ```



# Alarm ([hard](https://pdos.csail.mit.edu/6.S081/2020/labs/guidance.html))

目标：

向 xv6添加一个特性，它将在进程使用 CPU 时间时**周期性**发出警报。更一般地说，实现用户级中断/错误处理程序的基本形式; 例如，可以使用类似的东西来处理应用程序中的页面错误。

hint：

- 添加一个新的系统调用sigalarm(interval, handler)，如果应用程序调用 sigAlarm (n，fn) ，那么在程序消耗的每 n 个 CPU 时间之后，内核应该调用应用程序函数 fn。当 fn 返回时，应用程序应该从停止的地方恢复
- 如果一个应用程序调用 sigAlarm (0,0) ，内核应该停止生成周期性的alarm调用
- user/alarmtest.c是测试文件，需要将其添加到makefile。添加了 sigAlarm 和 sigreturn 系统调用，它才能正确编译
- 可以在 user/alarmtest.asm 中看到 alarmtest 的汇编代码，这可能有助于调试
- alarmtest 中在 test0 调用了 sigalarm(2, periodic)，要求内核强制每隔2个tick调用 period () ，然后spin一段时间

test0：修改内核以跳转到用户空间中的警报处理程序

先修改makefile编译出alarmtest

```c
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
	$U/_alarmtest\
```

放入 user/user.h 中的正确声明:

```c
// system calls
int sigalarm(int ticks, void (*handler)());
int sigreturn(void);
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
```

更新 user/usys.pl (生成 user/usys.S)

```c
#!/usr/bin/perl -w

# Generate usys.S, the stubs for syscalls.

print "# generated by usys.pl - do not edit\n";

print "#include \"kernel/syscall.h\"\n";

sub entry {
    my $name = shift;
    print ".global $name\n";
    print "${name}:\n";
    print " li a7, SYS_${name}\n";
    print " ecall\n";
    print " ret\n";
}
	
entry("fork");
entry("exit");
entry("wait");
entry("pipe");
entry("read");
entry("write");
entry("close");
entry("kill");
entry("exec");
entry("open");
entry("mknod");
entry("unlink");
entry("fstat");
entry("link");
entry("mkdir");
entry("chdir");
entry("dup");
entry("getpid");
entry("sbrk");
entry("sleep");
entry("uptime");
entry("sigalarm");//
entry("sigreturn");//
```

 kernel/syscall.h 

```c
// System call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
#define SYS_sigalarm 22
#define SYS_sigreturn 23
```

kernel/syscall.c

```c

extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
extern uint64 sys_exec(void);
extern uint64 sys_exit(void);
extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);
extern uint64 sys_sigalarm(void);//
extern uint64 sys_sigreturn(void);//

static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_sigalarm] sys_sigalarm,//
[SYS_sigreturn] sys_sigreturn,//
};
```

在sysproc.c中加入sys_sigalarm和sys_sigreturn函数处理代码

```c
uint64 
sys_sigalarm(void)
{
  int n;
  uint64 fn;
  if(argint(0, &n) < 0) {
      return -1;
  }
  if(argaddr(1, &fn) < 0) {
      return -1;
  }
  return sigalarm(n, (void(*)())(fn));
}

uint64
sys_sigreturn(void)
{
  return sigreturn();
}

```

关于test1的提示：

- 解决方案将要求您保存和恢复寄存器-- 您需要保存和恢复哪些寄存器才能正确地恢复被中断的代码？(提示: 会有很多)。
- 当计时器关闭时，用户陷阱在 struct proc 中保存足够的状态，这样签名返回就可以正确地返回到被中断的用户代码。
- 防止对处理程序的重入调用——如果处理程序还没有返回，内核就不应该再次调用它。

硬件时钟每次都强制执行一个中断，这个中断在 kernel/trap.c 中的 usertrap ()中处理。每次时钟发生硬件中断，都统计一次，到了次数就执行handler

在proc中加入如下变量：

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
  
  int alarm_interval; //n in sigalarm(n, (void(*)())(fn));
  void(*alarm_handler)();// fn in sigalarm(n, (void(*)())(fn));
  int alarm_ticks;//after alarm_ticks will 
  struct trapframe *saved_trapframe;
  int alarm_flag;
};
```

完善sigalarm和sigreturn函数，并在defs.h中加入声明

```c
// defs.h
int             sigalarm(int, void (*)());
int             sigreturn(void);

//trap.c
int sigalarm(int ticks, void (*handler)())
{
  struct proc *p = myproc();
  p->alarm_interval=ticks;
  p->alarm_handler=handler;
  p->alarm_ticks=ticks;
  return 0;
} 

int sigreturn(void)
{
  struct proc *p = myproc();
  *p->trapframe = *p->saved_trapframe;
  p->alarm_flag = 0;
    return 0;
}

```

allocproc函数和freeproc函数也需要更新

- 进程创建和销毁时结构体的字段的设置

对进程的字段进行初始化和销毁以及页的申请

- 使用kalloc给p->saved_trapframe分配空间
- 使用kfree((void *)p->saved_trapframe);销毁空间

```c
// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
   //
  if((p->saved_trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  p->alarm_flag=0;//
  p->alarm_interval = 0;
  p->alarm_handler = 0;
  p->alarm_ticks = 0;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
    
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  
  if(p->saved_trapframe)
    kfree((void *)p->saved_trapframe);
    
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  
  p->alarm_flag=0;//
  p->alarm_interval = 0;
  p->alarm_handler = 0;
  p->alarm_ticks = 0;
}
```

评分

![](https://img.gls.show/img/b48525193508d98e10d9e31c3878fbf3.png)