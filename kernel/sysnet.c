//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//

// called by protocol handler layer to deliver UDP packets

void
sockclose(struct sock * s)
{
  acquire(&s->lock);
  acquire(&lock);
  struct sock * temp = sockets;
  struct sock * prev =0;
  while(temp){
    if (s->raddr == temp->raddr &&
        s->lport == temp->lport &&
	      s->rport == temp->rport){
          if(prev){
            acquire(&prev->lock);
            prev->next = s->next;
            release(&prev->lock);
            break;
          }else{
            sockets = sockets->next;
            break;
          }
          
    }
    prev = temp;
    temp = temp->next;
  }
  release(&lock);
  while(!mbufq_empty(&s->rxq)){
      struct mbuf * m = mbufq_pophead(&s->rxq);
      mbuffree(m);
  }
  release(&s->lock);
  kfree(s);
  
}

int
sockwrite(struct sock *s, uint64 addr, int n){
  int i=0,N=n,b;
  struct proc *p = myproc();
  acquire(&s->lock);
  while(N){
    struct mbuf *m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    b = (N<MBUF_SIZE - MBUF_DEFAULT_HEADROOM)? N:MBUF_SIZE - MBUF_DEFAULT_HEADROOM;
    copyin(p->pagetable, (char*)m->head,(uint64)addr+i,b);
    m->len = b;
    i = i + b;
    N = N-b;
    net_tx_udp(m,s->raddr,s->lport,s->rport);
  }
  release(&s->lock);
  return n;
}

int
sockread(struct sock *s, uint64 addr, int n){
  struct proc *p = myproc();
  acquire(&s->lock);
  while(mbufq_empty(&s->rxq)){
    sleep(s,&s->lock);
  }
  struct mbuf * m = mbufq_pophead(&s->rxq);
  copyout(p->pagetable, addr, (char*)m->head, m->len);
  n = m->len;
  mbuffree(m);
  release(&s->lock);
  return n;
}
void 
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //
  acquire(&lock);
  struct sock * temp = sockets;
  while(temp){
    if (raddr == temp->raddr &&
        lport == temp->lport &&
	      rport == temp->rport){
          acquire(&temp->lock);
          mbufq_pushtail(&temp->rxq,m);
          wakeup(temp);
          release(&temp->lock);
          release(&lock);
          return;
        }
        temp = temp->next;
  }
  release(&lock);
  mbuffree(m);
}
