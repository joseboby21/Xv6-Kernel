#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[])
{
  int time;
  if(argc !=2){
    printf("Format is %s <ticks>\n",argv[0]);
    exit();
  }
  time = atoi(argv[1]);
  if(time>0){
    sleep(time);
    exit();
  }
  else{
   printf("Time should be greater than zero\n");
    exit();
  }
}
