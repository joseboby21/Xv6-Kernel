// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct spinlock common;

struct run {
  struct run *next;
};



struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[8];




void
kinit()
{
  int cpu;
  push_off();
  cpu = cpuid();
  pop_off();
  initlock(&kmem[cpu].lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int cpu;
  push_off();
  cpu = cpuid();
  pop_off();


  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  //struct proc *p = myproc();
  struct run *r;
  int cpu;
  push_off();
  cpu = cpuid();
  pop_off();

  acquire(&kmem[cpu].lock);
  if(!kmem[cpu].freelist){
    for(int i=0;i<8;++i){
      if(i!=cpu){
        if(kmem[i].freelist){
          for(int j=0;j<5;++j){
            if(!kmem[cpu].freelist && kmem[i].freelist){
               acquire(&kmem[i].lock);
               kmem[cpu].freelist= kmem[i].freelist;
               kmem[i].freelist = kmem[i].freelist->next;
               kmem[cpu].freelist->next = 0;
               release(&kmem[i].lock);
            }
            else if(kmem[i].freelist){
              acquire(&kmem[i].lock);
              r  = kmem[i].freelist;
              kmem[i].freelist = r->next;
              r->next = kmem[cpu].freelist;
              kmem[cpu].freelist = r;
              release(&kmem[i].lock);
            }
            else
              break;
          }

        }
      }
    }

  }

  
  if(!kmem[cpu].freelist){
    //printf("System ran out of memory\n");
    release(&kmem[cpu].lock);
    return 0;
    //p->killed = 1;
    //exit(1);
  }
  //release(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  if(r)
    kmem[cpu].freelist = r->next;
  release(&kmem[cpu].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
