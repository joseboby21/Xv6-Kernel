#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"


int main(int argc, char *argv[]){
	char x;
	int k,t;
	char *argsv[MAXARG];
	char args[512];
	char * temp;
	char *p = args;
	argsv[0] = argv[1];
	for(k=1,t=2; ;++k,++t){
		if(argv[t]!=0)
			argsv[k] = argv[t];
		else
			break;
	}
	for(;;){
		temp = p;
		for(; ;){
			if(read(0,&x,sizeof(x))==0){
				if(fork()){
					wait();
					exit();
				}
				else{
					exec(argsv[0],argsv);
					fprintf(2,"exec failed\n");
					exit();
				}
			}
			else if(x == '\n'){
				if(k==MAXARG){
					fprintf(2,"Max argument size exceeded\n");
					exit();
				}
				else{
					*(p++) = 0;
					argsv[k++] =temp;
					break;
			    }
			}
			else
				*(p++) = x;
		}
	}
	exit();
}
