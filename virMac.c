#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


int ** code;
int curr;
int * globalMem;
typedef struct task{
	int body;
	int arg;
}taskT;

int main (int argc, char * argv[]){
	
	FILE * fp;
	
	int i,k,flag=0;
	int magicbeg[4];
	int globalsize;
	int numofbodies;
	int totalcodesize[2];
	int notasks;
	int codeSize;
	int * localSize;
	taskT * tasks;
	
	if (argc < 2){ 
		printf ("Not enough arguments\n");
	}
	fp=fopen(argv[1], "r");
	
	//----------read header file---------------
	//----------magic beg----------------
	for (i=0;i<4;i++){
		fscanf(fp,"%x",&magicbeg[i]);
		printf("%x", magicbeg[i]);
		
	}
	printf("\n");
	
	
	if (magicbeg[0]==0xde){
		if (magicbeg[1]==0xad){
			if (magicbeg[2]==0xbe){
				if(magicbeg[3]==0xaf){
					flag=1;
				}
			}
		}
	}
	
	if (flag!=1){
		printf("wrong magicbeg\n");
		return(1);
	}
	
	flag=0;
	
	
	
	//---------global size----------------
	
	fscanf(fp, "%x",&globalsize);
	printf("%x", globalsize);
	printf("\n");
	//----------numofbodies------------
	
	fscanf(fp,"%x",&numofbodies);
	printf("%x", numofbodies);
	printf("\n");
	
	//-------totalsize------------
	for(i=0;i<2;i++){
		fscanf(fp,"%x",&totalcodesize[i]);
		printf("%x", totalcodesize[i]);
	}
	printf("\n");
	
	//----notasks-----------
	
	fscanf(fp,"%x",&notasks);
	printf("%x", notasks);
	printf("\n");
	
	
	
	
	//---------initialize globals------------
	
	if (NULL==(globalMem=((int*)malloc (sizeof(int)*globalsize)))){
		perror("malloc error");
		return (1);
	}
	for(i=0;i<globalsize;i++){
		fscanf(fp,"%x",&globalMem[i]);
		printf("%x", globalMem[i]);
	}
	printf("\n");
	if (NULL==(tasks=((taskT*)malloc (sizeof(taskT)*notasks)))){
		perror("malloc error");
		return (1);
	}
	
	
	if (NULL==(code=((int**)malloc (sizeof(int*)*numofbodies)))){
		perror("malloc error");
		return (1);
	}
	
	if (NULL==(localSize=((int*)malloc (sizeof(int)*numofbodies)))){
		perror("malloc error");
		return (1);
	}
	
	//---------store bodies into string------------
	//-----------header-----------------
	//----------magic beg----------------
	for (k=0;k<numofbodies;k++){
		
		
		for (i=0;i<4;i++){
			fscanf(fp,"%x",&magicbeg[i]);
			printf("%x", magicbeg[i]);
			
		}
		printf("\n");
		
		
		if (magicbeg[0]==0xde){
			if (magicbeg[1]==0xad){
				if (magicbeg[2]==0xc0){
					if(magicbeg[3]==0xde){
						flag=1;
					}
				}
			}
		}
		
		if (flag!=1){
			printf("wrong magicbeg\n");
			return(1);
		}
		flag=0;
		
		//---------------local size--------------
		
		fscanf(fp,"%x",&localSize[k]);
		printf("%x", localSize[k]);
		printf("\n");
		
		
		//---------code size----------------
		
		
		fscanf(fp,"%x",&codeSize);
		printf("%x", codeSize);
		printf("\n");
		
		if (NULL==(code[k]=((int*)malloc (sizeof(int)*codeSize)))){
			perror("malloc error");
			return (1);
		}
		for (i=0;i<codeSize;i++){
			fscanf(fp,"%x", &code[k][i]);
			printf("%x", code[k][i]);
		}
		printf("\n");
	}
	
	
	
	//----------read task parameters etc----------
	for (k=0;k<notasks;k++){
		for (i=0;i<4;i++){
			fscanf(fp,"%x",&magicbeg[i]);
			printf("%x", magicbeg[i]);
			
		}
		printf("\n");
		
		
		if (magicbeg[0]==0xde){
			if (magicbeg[1]==0xad){
				if (magicbeg[2]==0xba){
					if(magicbeg[3]==0xbe){
						flag=1;
					}
				}
			}
		}
		
		if (flag!=1){
			printf("wrong magicbeg\n");
			return(1);
		}
		flag=0;
		
		
		//----task body------------------
		fscanf(fp,"%x",&tasks[k].body);
		printf("%x", tasks[k].body);
		printf("\n");

		//------argument------------
		fscanf(fp,"%x",&tasks[k].arg);
		printf("%x", tasks[k].arg);
		printf("\n");
	}
	
	//-----------footer---------------------
	
	for (i=0;i<4;i++){
		fscanf(fp,"%x",&magicbeg[i]);
		printf("%x", magicbeg[i]);
		
	}
	printf("\n");
	
	
	if (magicbeg[0]==0xfe){
		if (magicbeg[1]==0xe1){
			if (magicbeg[2]==0xde){
				if(magicbeg[3]==0xad){
					flag=1;
				}
			}
		}
	}
	
	if (flag!=1){
		printf("wrong magicbeg\n");
		return(1);
	}
	flag=0;
	
	
	
	
	
	
	
	//----------execute tasks---------------------
	
	
	
	
	return (0);
}