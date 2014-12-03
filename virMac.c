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
	int id;
	char state[10];
	int reg[8];
	int pc;
	int sem;
	int waket;
	int * localMem;
}taskT;

int main (int argc, char * argv[]){
	
	FILE * fp;
	
	int command[3];
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
		
		//------initialize-----------
		tasks[k].id=k;
		strcpy(tasks[k].state,"READY");
		tasks[k].pc=0;
		tasks[k].sem=-1;
		tasks[k].waket=-1;
		
		if (NULL==(tasks[k].localMem=((int*)malloc (sizeof(int)*localSize[(tasks[k].body)])))){
			perror("malloc error");
			return (1);
		}
		
		
		tasks[k].localMem[(localSize[(tasks[k].body)])]=tasks[k].arg;
		
		
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
	curr=0;
	int progress=0;
	k=0;
	while (k<7){
		for (i=0;i<3;i++){
			command[i]=code[(tasks[curr].body)-1][tasks[curr].pc];
			printf("%x ",command[i]);
			tasks[curr].pc++;
		}
		printf("\n");
		
		switch(command[0]){
			// -----------load/store----------------------------
			case 0x01 :
				tasks[curr].reg[command[1]]=tasks[curr].localMem[command[2]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x02 :
				tasks[curr].reg[command[1]]=tasks[curr].localMem[command[2]+tasks[curr].reg[0]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x03 :
				tasks[curr].reg[command[1]]=globalMem[command[2]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x04 :
				tasks[curr].reg[command[1]]=globalMem[command[2]+tasks[curr].reg[0]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x05 :
				tasks[curr].localMem[command[2]]=tasks[curr].reg[command[1]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x06 :
				tasks[curr].localMem[command[2]+tasks[curr].reg[0]]=tasks[curr].reg[command[1]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x07 :
				globalMem[command[2]]=tasks[curr].reg[command[1]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x08 :
				globalMem[command[2]+tasks[curr].reg[0]]=tasks[curr].reg[command[1]];
				printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			//-----------registers--------------------
			case 0x09:
				tasks[curr].reg[command[1]]=command[2];
				progress++;
				break;
			case 0x0A:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]+tasks[curr].reg[command[2]];
				progress++;
				break;
			case 0x0B:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]-tasks[curr].reg[command[2]];
				progress++;
				break;
			case 0x0C:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]*tasks[curr].reg[command[2]];
				progress++;
				break;
			case 0x0D:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]/tasks[curr].reg[command[2]];
				progress++;
				break;
			case 0x0E:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]%tasks[curr].reg[command[2]];
				progress++;
				break;
				
			// --------- branches---------------------
				
			case 0x0F:
				if (tasks[curr].reg[command[1]]>0){
					tasks[curr].pc=tasks[curr].pc+command[2];
				}
				progress++;
				break;
			case 0x10:
				if (tasks[curr].reg[command[1]]>=0){
					tasks[curr].pc=tasks[curr].pc+command[2];
				}
				progress++;
				break;
			case 0x11:
				if (tasks[curr].reg[command[1]]<0){
					tasks[curr].pc=tasks[curr].pc+command[2];
				}
				progress++;
				break;
			case 0x12:
				if (tasks[curr].reg[command[1]]<=0){
					tasks[curr].pc=tasks[curr].pc+command[2];
				}
				progress++;
				break;
			case 0x13:
				if (tasks[curr].reg[command[1]]==0){
					tasks[curr].pc=tasks[curr].pc+command[2];
				}
				progress++;
				break;
			case 0x14:
				tasks[curr].pc=tasks[curr].pc+command[2];
				progress++;
				break;
				
			// ----------- synch ------------------
			case 0x15:
				globalMem[command[2]]--;
				if (globalMem[command[2]]<0) {
					strcpy(tasks[curr].state,"BLOCKED");
				}
				tasks[curr].sem=command[2];
				progress++;
				break;
			/*case 0x16:
				globalMem[command[2]]++;
				if (globalMem[command[2]]<=0) {
				
					strcpy(tasks[curr].state,"BLOCKED");
				}
				tasks[curr].sem=command[2];
				progress++;
				break;*/
		}
		k++;	
	}
	
	
	
	return (0);
}