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