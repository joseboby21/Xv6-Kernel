#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"


int main(int argc, char *argv[]){
	char x;
	int k,p;
	char *argsv[MAXARG];
	for(k=0,p=1; ;++k,++p){
		if(argv[p]!=0)
			argsv[k] = argv[p];
		else
			break;
	}
	for(;;){
		int j =0;
		char *args = malloc(512*sizeof(char));
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
					fprintf(2,"Max argument number exceeded\n");
					exit();
				}
				else{
					args[j++] = 0;
					argsv[k++] =args;
					break;
			    }
			}
			else
				args[j++] = x;

	    }
	}
	exit();
}
