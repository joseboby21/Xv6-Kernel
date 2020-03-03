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

struct run {
  struct run *next;
};



struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];




void
kinit()
{
  for(int i=0;i<NCPU;++i){
    initlock(&kmem[i].lock, "kmem");
  }
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
kalloc(void){
  struct run *r;
  int steal_pages = 1;
  int cpu;
  int count =0;
  push_off();
  cpu = cpuid();
  pop_off();
  
  acquire(&kmem[cpu].lock);
  if(!kmem[cpu].freelist){
    for(int i=0;i<NCPU;++i){
      if(count ==steal_pages)
        break;
      if(i!=cpu){  // So that the process does not try to take page from itself
        if(kmem[i].freelist){     // To check if the searched process have free pages
          for(int j=0;j<steal_pages-count;++j){
            acquire(&kmem[i].lock);
            if(kmem[i].freelist){
              r  = kmem[i].freelist;
              kmem[i].freelist = r->next;
              r->next = kmem[cpu].freelist;
              kmem[cpu].freelist = r;
              release(&kmem[i].lock);
              count ++;
            }
            else{
              release(&kmem[i].lock);
              break;
            }
          }
        }
      }
    }

  }

  
  if(!kmem[cpu].freelist){
    release(&kmem[cpu].lock);
    return 0;
  }
  r = kmem[cpu].freelist;
  if(r)
    kmem[cpu].freelist = r->next;
  release(&kmem[cpu].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  return (void*)r;
}
