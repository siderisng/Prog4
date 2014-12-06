#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define turns 2

uint8_t ** code;
int curr;
uint8_t * globalMem;
typedef struct task{
	uint8_t body;
	uint8_t arg;
	int id;
	char state[10];
	uint8_t reg[8];
	uint8_t pc;
	uint8_t sem;
	time_t waket;
	uint8_t * localMem;
}taskT;

int main (int argc, char * argv[]){
	
	srand(time(NULL));
	FILE * fp;
	uint8_t command[3];
	int i,k,flag=0, endflag=0;
	uint8_t magicbeg[4];
	uint8_t globalsize;
	uint8_t numofbodies;
	uint16_t totalcodesize;
	uint8_t notasks;
	uint8_t codeSize;
	uint8_t * localSize;
	taskT * tasks;
	
	
	
	
	if (argc < 2){ 
		printf ("Not enough arguments\n");
	}
	fp=fopen(argv[1], "r");
	
	//----------read header file---------------
	//----------magic beg----------------
	for (i=0;i<4;i++){
		fread (&magicbeg[i],1,1,fp);
		
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
	
	fread (&globalsize,1,1,fp);
	printf("%x", globalsize);
	printf("\n");
	//----------numofbodies------------
	
	fread (&numofbodies,1,1,fp);
	printf("%x", numofbodies);
	printf("\n");
	
	//-------totalsize------------
	
	fread (&totalcodesize,1,2,fp);
	printf("%x ", totalcodesize);
	
	printf("\n");
	
	//----notasks-----------
	
	fread (&notasks,1,1,fp);
	printf("%x", notasks);
	printf("\n");
	
	
	
	
	//---------initialize globals------------
	
	if (NULL==(globalMem=((uint8_t*)malloc (sizeof(uint8_t)*globalsize)))){
		perror("malloc error");
		return (1);
	}
	if (i==0){
		
		globalMem[i]=0x00;
	}else{
		for(i=0;i<globalsize;i++){
			fread (&globalMem[i],1,1,fp);
			printf("%x", globalMem[i]);
		}
		
	}
	printf("\n");
	if (NULL==(tasks=((taskT*)malloc (sizeof(taskT)*notasks)))){
		perror("malloc error");
		return (1);
	}
	
	
	if (NULL==(code=((uint8_t**)malloc (sizeof(uint8_t*)*numofbodies)))){
		perror("malloc error");
		return (1);
	}
	
	if (NULL==(localSize=((uint8_t*)malloc (sizeof(uint8_t)*numofbodies)))){
		perror("malloc error");
		return (1);
	}
	
	//---------store bodies into string------------
	//-----------header-----------------
	//----------magic beg----------------
	for (k=0;k<numofbodies;k++){
		printf ("HIII\n");
		
		for (i=0;i<4;i++){
			fread (&magicbeg[i],1,1,fp);
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
		
		fread (&localSize[k],1,1,fp);
		printf("%x", localSize[k]);
		printf("\n");
		
		
		//---------code size----------------
		
		
		fread (&codeSize,1,1,fp);
		printf("%x", codeSize);
		
		
		if (NULL==(code[k]=((uint8_t*)malloc (sizeof(uint8_t)*codeSize)))){
			perror("malloc error");
			return (1);
		}
		
		for (i=0;i<codeSize;i++){
			fread (&code[k][i] ,1,1,fp);
			if ((i%3)==0){
				printf ("\n");
			}
			
			printf("%x ", code[k][i]);
			
		}
		printf("\n");
	}
	
	
	
	//----------read task parameters etc----------
	for (k=0;k<notasks;k++){
		for (i=0;i<4;i++){
			fread (&magicbeg[i],1,1,fp);
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
		
		fread (&tasks[k].body ,1,1,fp);
		printf("%x", tasks[k].body);
		printf("\n");
		
		
		//------argument------------
		fread (&tasks[k].arg ,1,1,fp);
		printf("%x", tasks[k].arg);
		printf("\n");
		
		//------initialize-----------
		tasks[k].id=k;
		strcpy(tasks[k].state,"READY");
		tasks[k].pc=0;
		tasks[k].sem=-1;
		tasks[k].waket=-1;
		
		if (NULL==(tasks[k].localMem=((uint8_t*)malloc (sizeof(uint8_t)*localSize[(tasks[k].body)])))){
			perror("malloc error");
			return (1);
		}
		
		
		tasks[k].localMem[(localSize[(tasks[k].body)])]=tasks[k].arg;
		
		
	}
	
	printf ("HI2\n");
	//-----------footer---------------------
	
	for (i=0;i<4;i++){
		fread (&magicbeg[i],1,1,fp);
		printf("%x", magicbeg[i]);
		
	}
	printf("\n");
	
	
	if (magicbeg[0]==0xfe){
		if 	(magicbeg[1]==0xe1){
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
	while (1){
		for (i=0;i<3;i++){
			command[i]=code[(tasks[curr].body)-1][tasks[curr].pc];
			//printf("%x ",command[i]);
			tasks[curr].pc++;
		}
		//printf("\n");
		printf ("HIII from task :%d state:%s notasks:%d\n", tasks[curr].id,tasks[curr].state, notasks);	
		
		switch(command[0]){
			// -----------load/store----------------------------
			case 0x01 :
				tasks[curr].reg[command[1]]=tasks[curr].localMem[command[2]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x02 :
				tasks[curr].reg[command[1]]=tasks[curr].localMem[command[2]+tasks[curr].reg[0]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x03 :
				tasks[curr].reg[command[1]]=globalMem[command[2]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x04 :
				tasks[curr].reg[command[1]]=globalMem[command[2]+tasks[curr].reg[0]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x05 :
				tasks[curr].localMem[command[2]]=tasks[curr].reg[command[1]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x06 :
				tasks[curr].localMem[command[2]+tasks[curr].reg[0]]=tasks[curr].reg[command[1]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x07 :
				globalMem[command[2]]=tasks[curr].reg[command[1]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
				progress++;
				break;
			case 0x08 :
				globalMem[command[2]+tasks[curr].reg[0]]=tasks[curr].reg[command[1]];
				//	printf("%x\n",tasks[curr].reg[command[1]]);
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
				progress=0;
				i=0;
				progress=0;
				
				for (i=0;i<notasks;i++){
					if ((i!=curr)&&(strcmp(tasks[i].state,"BLOCKED"))&&(strcmp(tasks[i].state,"STOPPED"))){
						if (((!strcmp(tasks[i].state,"SLEEPING"))&&(time(NULL)>tasks[i].waket))){
							strcpy (tasks[i].state,"READY");
							tasks[i].waket=0;
							break;
						}
						else if ((strcmp(tasks[i].state,"SLEEPING"))){
							break;
						}
						
					}
					
				}
				if (i==notasks){
					printf ("Nowhere to go to from here :(");
					endflag=1;
					break;
				}
				curr=i;
				
				break;
				
			case 0x16:
				globalMem[command[2]]++;
				
				if (globalMem[command[2]]<=0) {
					for (i=0; i<notasks; i++){
						
						if (tasks[i].sem==command[2]){
							strcpy(tasks[i].state,"READY");
							tasks[i].sem=-1;
							break;
						}
					}
				}
				
				progress++;
				break;
				//----------------varia--------------
			case 0x17:
				
				progress=0;
				i=0;
				for (i=0;i<notasks;i++){
					if ((i!=curr)&&(strcmp(tasks[i].state,"BLOCKED"))&&(strcmp(tasks[i].state,"STOPPED"))){
						if (((!strcmp(tasks[i].state,"SLEEPING"))&&(time(NULL)>tasks[i].waket))){
							strcpy (tasks[i].state,"READY");
							tasks[i].waket=0;
							break;
						}
						else if ((strcmp(tasks[i].state,"SLEEPING"))){
							break;
						}
						
					}
					
				}
				if (i==notasks){
					printf ("Nowhere to YIELD to :(\n");
					
					
				}
				else{
					curr=i;}
					
					break;
					
			case 0x18:
				
				strcpy(tasks[i].state,"SLEEPING");
				
				tasks[curr].waket= command[2]+ time(NULL);
				
				break;
			case 0x19:
				
				printf ("%d:%d\n", tasks[curr].id, globalMem[command[2]]);
				
				
			case 0x1a:
				
				strcpy(tasks[curr].state,"STOPPED");
				
				progress=0;
				i=0;
				progress=0;
				i=0;
				for (i=0;i<notasks;i++){
					if ((i!=curr)&&(strcmp(tasks[i].state,"BLOCKED"))&&(strcmp(tasks[i].state,"STOPPED"))){
						if (((!strcmp(tasks[i].state,"SLEEPING"))&&(time(NULL)>tasks[i].waket))){
							strcpy (tasks[i].state,"READY");
							tasks[i].waket=0;
							break;
						}
						else if ((strcmp(tasks[i].state,"SLEEPING"))){
							break;
						}
						
					}
					
				}
				if (i==notasks){
					printf ("nowhere to go to");
					endflag=1;
					break;
				}
				curr=i;
				
				break;
				
		}
		if (progress==turns){
			progress=0;
			i=0;
			for (i=0;i<notasks;i++){
				if ((i!=curr)&&(strcmp(tasks[i].state,"BLOCKED"))&&(strcmp(tasks[i].state,"STOPPED"))){
					
					if (((!strcmp(tasks[i].state,"SLEEPING"))&&(time(NULL)>tasks[i].waket))){
						
						strcpy (tasks[i].state,"READY");
						tasks[i].waket=0;
						break;
					}
					else if ((strcmp(tasks[i].state,"SLEEPING"))){
						
						break;
					}
					
				}
				
			}
			
			if (i!=notasks){
				curr=i;
			}
			
		}
		
		
		if (endflag==1){
			printf ("end of programm\n");
			break;
		}
	}
	
	printf ("reg1 a %d, reg 2 b %d\n", tasks[0].reg[1], tasks[1].reg[2]);
	
	return (0);
}