#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[])
{
  int pp[2],cp[2];
  if(pipe(pp)<0 || pipe(cp)<0){
    printf("Unable to create pipe\n");
    exit();
   }
  if(!fork()){
    char x;
    read(pp[0],&x,sizeof(x));
    printf("%d: received ping\n",getpid());
    write(cp[1],&x,sizeof(x));
  }
  else{
    char x='a';
    write(pp[1],&x,sizeof(x));
    read(cp[0],&x,sizeof(x));
    printf("%d: received pong\n",getpid());
  }
  exit();
}
