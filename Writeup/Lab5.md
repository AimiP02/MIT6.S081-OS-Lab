# xv6-OS-Lab5-syscall

## 需要修改的文件

>
    [*]proc.h
    [*]memlayout.h
    [*]exec.c
    [*]vm.c
    [*]trap.c
    [*]syscall.c
    [*]proc.c
    [+]lab5.c

## 实验流程

### 确定任务

![xv6内存结构](https://tva4.sinaimg.cn/large/005J92LSly8h6kw1xc0iuj30qd0mg75e.jpg)

#### task.1

改变内存结构，让栈移动到内存空间的最顶端

#### task.2

让栈沿着内存从高地址往低地址生长

### 定义

在`proc.h`和`memlayout.h`中定义栈的页数和基址

```c
// proc.h
// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  uint numPages;               // lab 5
};
```

```c
// memlayout.h
// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNBASE 0x80000000         // First kernel virtual address
#define KERNLINK (KERNBASE+EXTMEM)  // Address where kernel is linked
#define STACKBASE (KERNBASE-1)	    // Lab5 ; First user virtual address
```

### 修改栈的位置

xv6在`exec.c`中分配用户虚拟内存，修改栈顶到定义的`USERTOP`，设置栈的页数为1

> `PGROUNDUP`和`PGROUNDDOWN`的作用是将传入的地址舍入到页大小的值，UP向上取整，DOWN向下取整
> ```c
> #define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
> #define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))
> ```

```c
  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  // if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
  //   goto bad;
  // clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  // sp = sz;

  if(allocuvm(pgdir, STACKBASE - PGSIZE, STACKBASE) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(STACKBASE - PGSIZE));
  sp = STACKBASE;

  curproc->numPage = 1;
  cprintf("New process %d, page: %d\n", curproc->pid, curproc->numPage);
```

再进入`vm.c`，修改`copyuvm()`，原本的循环现在只复制了父进程的数据和代码段，添加一个循环来复制父进程的栈段

当前进程栈的页数是`myproc()->numPages`，要复制所有页的栈段的范围就是`PGROUNDUP(USERTOP - (myproc()->numPages - 1) * PGSIZE)`到`USERTOP`

如果范围出错可能会导致qemu启动时无限reboot

```c
// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }
  }

  uint stackbase = PGROUNDDOWN(STACKBASE);

  for(i = stackbase; i > (stackbase - myproc()->numPage * PGSIZE); i -= PGSIZE) {
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }    
  }

  return d;

bad:
  freevm(d);
  return 0;
}
```

### 处理页面错误

在栈生长的过程中会出现页面错误，检查错误地址是否大于栈顶，如果是则分配一段新的内存页面，不是则让栈的页面数+1

```c
  case T_PGFLT:
    // check to see if fault address is from right under current top of stack
    // if yes, grow stack
    // - allocvum with correct parameters to allocate one page at the right place
    // - increment stack size counter 
    ;
    uint false_address = rcr2();
    if(false_address > STACKBASE)
      exit();
    false_address = PGROUNDDOWN(false_address);

    if(allocuvm(myproc()->pgdir, false_address, false_address + PGSIZE) == 0) {
      cprintf("Alloc new page error!\n");
      exit();
    }
    myproc()->numPage++;
    cprintf("Alloc new page, %d current page: %d\n", myproc()->pid, myproc()->numPage);
    break;
```

### 修改系统调用

系统调用会使用栈上的内存，所以修改系统调用到指定的用户栈段上

```c
int
fetchint(uint addr, int *ip)
{
  // struct proc *curproc = myproc();

  if(addr >= STACKBASE || addr+4 > STACKBASE)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;
  // struct proc *curproc = myproc();

  if(addr >= STACKBASE)
    return -1;
  *pp = (char*)addr;
  ep = (char*)STACKBASE;
  for(s = *pp; s < ep; s++){
    if(*s == 0)
      return s - *pp;
  }
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;
  // struct proc *curproc = myproc();
 
  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= STACKBASE || (uint)i+size > STACKBASE)
    return -1;
  *pp = (char*)i;
  return 0;
}
```

### 用户程序测试

```c
#include "types.h"
#include "user.h"
#include "stat.h"

#pragma GCC push_options
#pragma GCC optimize("O0")

int
factorial(int n)
{
    if(n < 0)
        return 0;
    else if(n == 0)
        return 1;
    else
        return n * factorial(n - 1);
}

int
main(int argc, char *argv[])
{
    if(argc != 2)
        exit();
    int n;
    n = atoi(argv[1]);
    factorial(n);
    exit();
}
```

输出如下

>
    SeaBIOS (version 1.13.0-1ubuntu1.1)


    iPXE (http://ipxe.org) 00:03.0 CA00 PCI2.10 PnP PMM+1FF8CA10+1FECCA10     CA00
                                                                                       
    
    
    Booting from Hard Disk..xv6...
    cpu1: starting 1
    cpu0: starting 0
    sb: size 1000 nblocks 941 ninodes 200 nlog 30 logstart 2 inodestart     32 bmap start 58
    New process 1, page: 1
    init: starting sh
    New process 2, page: 1
    $ ls
    New process 3, page: 1
    .              1 1 512
    ..             1 1 512
    README         2 2 2286
    cat            2 3 16264
    echo           2 4 15116
    forktest       2 5 9432
    grep           2 6 18484
    init           2 7 15704
    kill           2 8 15144
    ln             2 9 15004
    ls             2 10 17632
    mkdir          2 11 15248
    rm             2 12 15224
    sh             2 13 27860
    stressfs       2 14 16136
    usertests      2 15 67244
    wc             2 16 17000
    zombie         2 17 14816
    wolfietest     2 18 14972
    lab5           2 19 15236
    console        3 20 0
    $ lab5 100
    New process 4, page: 1
    $ lab5 1000
    New process 5, page: 1
    Alloc new page, 5 current page: 2
    Alloc new page, 5 current page: 3
    Alloc new page, 5 current page: 4
    Alloc new page, 5 current page: 5
    Alloc new page, 5 current page: 6
    Alloc new page, 5 current page: 7
    Alloc new page, 5 current page: 8
    $ lab5 12
    New process 6, page: 1
    $ 