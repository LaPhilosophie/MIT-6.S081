# Lab3 pgtable

写在lab开始之前：

- ~~这个lab很明显和前面不是一个难度~~
- 需要补充一些虚拟内存的知识
  - 看一下配套的xv6指导手册
  - B站的课程视频录播

## 页表

虚拟地址中间的index有27位，因此页表项就有`2^27`个，也即是`0~2^27-1`，将index对应的PPN取出和offset拼在一起就成了PA（physical address）

xv6 运行在 Sv39 RISC-V 上，这意味着只使用 64 位虚拟地址的底部 39 位，顶部 25 位未被使用。因此下面图中只需要关注低39位

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/202304171551847.png)

刚刚是单级页表，但是出于一些考虑，xv6使用三级页表的映射方式

Q：3级page table为什么会比一个超大的page table更好呢？

> Frans教授：这是个好问题，这的原因是，3级page table中，大量的PTE都可以不存储。比如，对于最高级的page table里面，如果一个PTE为空，那么你就完全不用创建它对应的中间级和最底层page table，以及里面的PTE。所以，这就是像是在整个虚拟地址空间中的一大段地址完全不需要有映射一样。
>
> 3级page table就像是按需分配这些映射块3级page table就像是按需分配这些映射块

对于页表的每一项的内容，`0~63`位如下

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/202304171551739.png)

xv6中定义的一些宏如下，和pte作`与`运算之后可以得到页表的某一位，比如pte & PTE_V可以得知页表是否有效

```c
#define PTE_V (1L << 0) // valid 		00001
#define PTE_R (1L << 1) // readable 	00010
#define PTE_W (1L << 2) // writeable	00100
#define PTE_X (1L << 3) // executable	01000
```

PA2PTE(pa)将虚拟地址右移截断12位之后再左移10位，可以得到对应的页表项

PTE2PA(pte)将页表项右移截断10位之后再左移12位，可以得到对应的虚拟地址

```c
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)
```

## Print a page table

在sh.c中加入代码：

```c
// Execute cmd.  Never returns.*
__attribute__((noreturn)) 
void
runcmd(struct cmd *cmd) {
	.....
}
```

这样做是为了防止报递归错

在exec.c中加入下面代码，以打印出进程1的页表

```c
  if(p->pid==1) 
    vmprint(p->pagetable);
  
  return argc; // this ends up in a0, the first argument to main(argc, argv)

```

在 kernel/defs.h 中定义 vmprint 的原型

```c
// vm.c
void            vmprint(pagetable_t);
void            dfs_vmprint(pagetable_t ,int );
```

我们需要在vm.c中编写vmprint函数的代码

先观察一下给出的输出

```
page table 0x0000000087f6e000
..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
.. ..0: pte 0x0000000021fda401 pa 0x0000000087f69000
.. .. ..0: pte 0x0000000021fdac1f pa 0x0000000087f6b000
.. .. ..1: pte 0x0000000021fda00f pa 0x0000000087f68000
.. .. ..2: pte 0x0000000021fd9c1f pa 0x0000000087f67000
..255: pte 0x0000000021fdb401 pa 0x0000000087f6d000
.. ..511: pte 0x0000000021fdb001 pa 0x0000000087f6c000
.. .. ..510: pte 0x0000000021fdd807 pa 0x0000000087f76000
.. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
```

对该输出格式的几点解释：

- 第一行格式是`page table %p`，%p是一级页表入口的地址，pagetable[i]就是一级页表第i项
- 第二行开始，是dfs的结果。从0到512进行循环，看pte是否有效，如果有效那么找到pte指向的下一个条目，直到找到叶子节点
- 顶级页表页面只有条目0和255的映射。条目0的下一个级别的页表只映射了索引0，而该索引0的底层则映射了条目0、1和2。顶级页表255同理

先写出vmprint函数，把第一行打印出来，然后进入dfs循环

```c
void 
vmprint(pagetable_t pagetable)
{
  printf("page table %p\n", pagetable);
  dfs_vmprint(pagetable, 0);
}
```

`pte & PTE_V`用来判断页表是否有效，如果有效就先将对应的所有信息打印出来

`pte & (PTE_R|PTE_W|PTE_X)) == 0`用来判断是否有下一级的页表，如果有的话，就将该pte转化为虚拟地址，进入递归

写这部分代码要仔细参考阅读源码中的freewalk函数，很有帮助

```c
void dfs_vmprint(pagetable_t pagetable,int level)
{
    // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V)){//只要有效就打印出来
      // this PTE points to a lower-level page table.
      
      printf("..");//输出..的格式控制
      for(int j=0;j<level;j++) printf(" ..");
      
      printf("%d: pte %p pa %p\n", i, pte, PTE2PA(pte));//输出页表项信息
    
      if((pte & (PTE_R|PTE_W|PTE_X)) == 0)//如果有下一级的页表
      {
        uint64 child = PTE2PA(pte);
        dfs_vmprint((pagetable_t)child, level+1);
      }
    }
  }
}
```

输出结果

```shell
$ make qemu
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

xv6 kernel is booting

hart 2 starting
hart 1 starting
page table 0x0000000087f6e000
..0: pte 0x0000000021fda801 pa 0x0000000087f6a000
.. ..0: pte 0x0000000021fda401 pa 0x0000000087f69000
.. .. ..0: pte 0x0000000021fdac1f pa 0x0000000087f6b000
.. .. ..1: pte 0x0000000021fda00f pa 0x0000000087f68000
.. .. ..2: pte 0x0000000021fd9c1f pa 0x0000000087f67000
..255: pte 0x0000000021fdb401 pa 0x0000000087f6d000
.. ..511: pte 0x0000000021fdb001 pa 0x0000000087f6c000
.. .. ..510: pte 0x0000000021fdd807 pa 0x0000000087f76000
.. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
init: starting sh
$ 
```

## A kernel page table per process 

Xv6只有一个内核页表，只要在内核中执行，就会使用它。内核页表是物理地址的直接映射，也即是内核虚拟地址 x 映射到物理地址 x。每个进程也会有自己的用户页表，该页表只包含此进程的用户空间的地址映射。而这些用户页表和用户地址在内核页表中是不存在的。因此就会出现一个问题：假设用户进程调用write系统调用并传递一个指针，由于该指针是用户级的地址，在内核中是无效的，因此实际过程中会先将用户级的虚拟地址转换为物理地址，这样就会造成一定程度的开销。本任务就是为了解决这个问题，让每个用户级的进程都有一个内核页表

**任务：修改内核，以便每个进程在内核中执行时使用其自己的内核页表副本**

我们自然想到要在 `struct proc`中加上进程的内核页表条目 kpgtable

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
  pagetable_t kpgtable;        // Kernel page table
};
```

后面要用的几个重要函数

| 函数名称                         | 作用                                                    |
| -------------------------------- | ------------------------------------------------------- |
| walk                             | 通过虚拟地址得到 PTE                                    |
| mappages                         | 将虚拟地址映射到物理地址                                |
| copyin                           | 将用户虚拟地址的数据复制到内核空间地址                  |
| copyout                          | 将内核数据复制到用户虚拟地址                            |
| kvminit                          | 创建内核的页表，对全局的`kernel_pagetable`进行映射      |
| kvmmap                           | 调用 mappages，将一个虚拟地址范围映射到一个物理地址范围 |
| kvminithart                      | 映射内核页表。它将根页表页的物理地址写入寄存器 satp 中  |
| procinit                         | 为每个进程分配一个内核栈                                |
| kalloc                           | 分配物理页                                              |
| proc_pagetable(struct proc **p*) | 为给定的进程创建用户态页表                              |
| kvminithart                      | 映射内核页表。它将根页表页的物理地址写入寄存器 satp 中  |

相关函数调用链：

```
main 
kvminit -> kvmmap
procinit
userinit -> allocproc
```

之后，我们要对进程的内核页表进行初始化

参考kvminit函数和kvmmap函数，这两个函数用来添加对全局kernel_pagetable的映射

```c
/*
 * create a direct-map page table for the kernel.
 */
void
kvminit()
{
  kernel_pagetable = (pagetable_t) kalloc();
  memset(kernel_pagetable, 0, PGSIZE);

  // uart registers
  kvmmap(UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  kvmmap(VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // CLINT
  kvmmap(CLINT, CLINT, 0x10000, PTE_R | PTE_W);

  // PLIC
  kvmmap(PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  kvmmap(KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  kvmmap((uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kernel_pagetable, va, sz, pa, perm) != 0)
    panic("kvmmap");
}
```

类似的，我们创建ukvmmap和ukvminit这两个函数，完成对进程结构体中的内核页表的映射

```c
void
ukvmmap(pagetable_t kpgtable,uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kpgtable, va, sz, pa, perm) != 0)
    panic("ukvmmap");
}

pagetable_t
ukvminit()
{
  pagetable_t kpgtable=(pagetable_t) kalloc();
  memset(kpgtable, 0, PGSIZE);
  ukvmmap(kpgtable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
  ukvmmap(kpgtable, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);
  ukvmmap(kpgtable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);
  ukvmmap(kpgtable, PLIC, PLIC, 0x400000, PTE_R | PTE_W);
  ukvmmap(kpgtable, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);
  ukvmmap(kpgtable, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);
  ukvmmap(kpgtable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
  return kpgtable;
}
```

在defs.h中需要添加

```c
void            ukvmmap(pagetable_t ,uint64 , uint64 , uint64 , int );
pagetable_t     ukvminit();
```

根据前面函数调用的分析，我们可以发现这个main中userinit -> allocproc的调用。allocproc函数返回一个未被利用的进程，并且做初始化工作，比如下面的这一些代码来自allocproc，做了用户页表的初始化工作

```c
  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
```

同样的，既然已经在结构体里面加了kpgtable，那么也要给进程的内核态页表做初始化工作

```c
  p->kpgtable=ukvminit();
  if(p->kpgtable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
```

在procinit函数中，为了给进程初始化内核栈，有这样一段代码：

```c
      // Allocate a page for the process's kernel stack.
      // Map it high in memory, followed by an invalid
      // guard page.
      char *pa = kalloc();
      if(pa == 0)
        panic("kalloc");
      uint64 va = KSTACK((int) (p - proc));
      kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
      p->kstack = va;
```

类似的，在allocproc函数中，我们需要初始化内核栈并将其映射到进程单独的内核页表，需要加上：（在这之后可以将procinit中初始化内核栈的代码注释掉）

```c
  p->kpgtable=ukvminit();
  if(p->kpgtable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  char *pa = kalloc();
  if(pa == 0)
    panic("kalloc");
  uint64 va = KSTACK((int) (p - proc));
  ukvmmap(p->kpgtable, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  p->kstack = va;
```

freeproc函数，要将进程的内核页表进行清除

```c
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
  /* add here */
  if (p->kstack)
  {
      pte_t* pte = walk(p->kpgtable, p->kstack, 0);
      if (pte == 0)
          panic("freeproc: walk");
      kfree((void*)PTE2PA(*pte));
  }
  p->kstack = 0;

  if (p->kpgtable)
    freewalk_kproc(p->kpgtable);
  /* add here */
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}
```

我们需要模仿freewalk重新实现freewalk_kproc，使得仅仅解除页表的内存映射而不像freewalk一样清除物理页数据

```c
void
freewalk_kproc(pagetable_t pagetable) {
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V)){
      pagetable[i] = 0;
      if ((pte & (PTE_R|PTE_W|PTE_X)) == 0)
      {
        uint64 child = PTE2PA(pte);
        freewalk_kproc((pagetable_t)child);
      }
    }
  }
  kfree((void*)pagetable);
}
```

 在调度程序scheduler中，调度程序之前，我们需要切换到进程自己的内核页表处，并使用sfence.vma刷新当前 CPU 的 TLB

```c
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    
    int found = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        //切换到进程自己的内核页表处
        w_satp(MAKE_SATP(p->kpgtable));
        //使用sfence.vma刷新当前 CPU 的 TLB
        sfence_vma();
        //调度进程
        swtch(&c->context, &p->context);
        //切换回内核页表
        kvminithart();
        
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;

        found = 1;
      }
      release(&p->lock);
    }
#if !defined (LAB_FS)
    if(found == 0) {
      intr_on();
      asm volatile("wfi");
    }
#else
    ;
#endif
  }
}
```

给 vm.c 添加头文件：

```c
#include "spinlock.h"
#include "proc.h"//如果顺序颠倒会报结构体定义错误
```

修改 `kvmpa` 函数

```c
pte = walk(myproc()->kpgtable, va, 0);
```



## Simplify `copyin/copyinstr`

> 内核的 copyin 函数读取用户指针指向的内存。它通过将它们转换为物理地址来实现这一点，而内核可以直接取消对它们的引用。它通过在软件中遍历过程页表来执行这种转换。在实验室的这一部分中，您的工作是向每个进程的内核页表(在前一节中创建)添加用户映射，以允许 copyin (以及相关的字符串函数 copinstr)直接取消引用用户指针。

首先声明copyin_new和copyinstr_new

```c
int copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);

int copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max);
```

修改copyin函数，使得改为执行copyin_new

```c
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  return copyin_new(pagetable, dst, srcva, len);
}

int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  return copyinstr_new(pagetable, dst, srcva, max);
}
```

增加一个u2kvmcopy函数，完成页表的复制

```c
void 
u2kvmcopy(pagetable_t upagetable, pagetable_t kpagetable, uint64 oldsz, uint64 newsz)
{
  oldsz = PGROUNDUP(oldsz);
  for (uint64 i = oldsz; i < newsz; i += PGSIZE) {
    pte_t* pte_from = walk(upagetable, i, 0);
    if(pte_from == 0) 
      panic("pte_from");
    pte_t* pte_to = walk(kpagetable, i, 1);
    if(pte_to == 0) 
      panic("pte_to");
    uint64 pa = PTE2PA(*pte_from);
    uint flag = (PTE_FLAGS(*pte_from)) & (~PTE_U);
    *pte_to = PA2PTE(pa) | flag;
  }
}
```

userinit加上：

```c
u2kvmcopy(np->pagetable, np->kpgtable, 0, np->sz);
```

fork加上：

```c
u2kvmcopy(np->pagetable, np->kpgtable, 0, np->sz);
```

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
  u2kvmcopy(np->pagetable, np->kpgtable, 0, np->sz);
  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  np->state = RUNNABLE;

  release(&np->lock);

  return pid;
}
```

在exec函数加上

```c
u2kvmcopy(np->pagetable, np->kpgtable, 0, np->sz);
  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if(sp < stackbase)
      goto bad;
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
```

growproc

```c
// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
    u2kvmcopy(p->pagetable, p->kpgtable, sz-n, sz);
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}
```

完结撒花（~~泪目~~）

![](https://cdn.jsdelivr.net/gh/LaPhilosophie/image/img/image-20230520212417687.png)

