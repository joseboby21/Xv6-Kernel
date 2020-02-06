#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void readDirectory(char * dir,char * filename){
	char buf[512], *p;
  	int fd;
  	struct dirent de;
  	struct stat st;
  	if((fd = open(dir, 0)) < 0){
    	fprintf(2, "find cannot open %s\n", dir);
    	return;
    }
    if(stat(dir, &st) < 0){
        fprintf(2,"find: cannot stat %s\n", buf);
   }
   if(st.type==T_FILE){
   		fprintf(2,"The directory path  given is path to a file\n");
   		exit();
   }
  	strcpy(buf, dir);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0 || strcmp(de.name,".")==0 || strcmp(de.name,"..")==0 )
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        fprintf(2,"find: cannot stat %s\n", buf);
        continue;
      }
      if(st.type == T_DIR){
        if(strcmp(de.name,filename)==0){
          printf("%s\n",buf);
          return;
        }
      	readDirectory(buf,filename);
      }
      else if(st.type == T_FILE){
      	if(strcmp(de.name,filename)==0){
      		printf("%s\n",buf);
      	}
      }
    }
}

int main (int argc, char * argv[]){
	if(argc !=3){
		fprintf(2,"Usage: find <directory> <filename>\n");
		exit();
	}
	readDirectory(argv[1],argv[2]);
  exit();
}
