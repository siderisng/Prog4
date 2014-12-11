#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define N 4
#define turns 4 //commands executed before switching task




//------------variables---------------
uint8_t ** code; // store code here
uint8_t * sizeOfBody;// size of code for each body
uint8_t globalsize; // number of globals
int * globalMem; //global memory
uint8_t notasks; // number of tasks
int nofCPUZ = N;
pthread_mutex_t check;
int terminateFlag=0, reallyTerminate=0;
int nextCurr;
int *blockedFlag;




//-------------structs-------------
typedef struct task{  //memory for each task

	int slaveTO;
	uint8_t body;    // body to link task to
	uint8_t arg;     //task arguments
	int id;          //task id
	char state[10];  //task state
	int reg[8];      //task registers (we used 8 reg0 is idx)
	uint8_t pc;      // next command fouint8_t sem;r task
	int sem;     //sem in which task is blocked
	time_t waket;    // time to wait if sleep is called
	int * localMem;  //local mem for each task
}taskT;

taskT * tasks;  //array of tasks

typedef struct cpu{
	int id;
	int nofCpuTasks;
	int * slaveTasks;
	pthread_mutex_t locked;

	
}CPU;
CPU * unit;

//-----------functions-------------



void shareTask ();
void *CPUact(void * cpuCurrId);


//------------main------------------
int main (int argc, char * argv[]){
	
	srand(time(NULL)); //init time
	FILE * fp;        //for file reading
	int i,k,flag=0, thrCheck;  
	uint8_t magicbeg[4]; //for reading the magicbegz
	
	uint8_t numofbodies;// number of bodies
	uint16_t totalcodesize;// size of code
	uint8_t codeSize; // size of code for each body
	uint8_t * localSize; //size of locals
	uint8_t * forGl; // reads globalinitials and stores them to globalMem(unsigned->signed)
	pthread_t * cpuThr;
	
	
	//open file
	if (argc < 2){ 
		printf ("Not enough arguments\n");
	}
	fp=fopen(argv[1], "r");
	
	
	//-----------init Mutex------------
	if (pthread_mutex_init(&check, NULL) != 0)
	{
		perror ("Mutex error");
		return 1;
	}
	
	//----------read header file---------------
	
	//----------magic beg----------------
	
	printf ("MagicBeg: ");
	//check if magicbeg is right
	
	for (i=0;i<4;i++){
		fread (&magicbeg[i],1,1,fp);
		
		printf("%x ", magicbeg[i]);
		
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
		printf("Wrong magicbeg. Expected DE AD BE AF\n");
		return(1);
	}
	
	flag=0;
	
	
	
	//---------global size----------------
	
	fread (&globalsize,1,1,fp);
	printf("Global Size: %x", globalsize);
	printf("\n");
	
	//----------numofbodies------------
	
	fread (&numofbodies,1,1,fp);
	printf("Number of Bodies %x", numofbodies);
	printf("\n");
	
	//-------totalsize------------
	
	fread (&totalcodesize,1,2,fp);
	printf("Total code size: %x ", totalcodesize);
	
	printf("\n");
	
	//----notasks-----------
	
	fread (&notasks,1,1,fp);
	printf("Number of tasks: %x", notasks);
	printf("\n");
	
	
	
	
	//---------initialize globals------------
	
	//init global memory
	
	
	if (NULL==(globalMem=((int*)malloc (sizeof(int)*globalsize)))){
		perror("malloc error");
		return (1);
	}
	if (NULL==(forGl=((uint8_t*)malloc (sizeof(uint8_t)*globalsize)))){
		perror("malloc error");
		return (1);
	}
	//read initial values
	printf ("Global Initials: ");
	for(i=0;i<globalsize;i++){
		fread (&forGl[i],1,1,fp);
		globalMem[i]=forGl[i];
		printf("%x ", globalMem[i]);
	}
	printf("\n");
	
	//init sizeOfBody
	if (NULL==(sizeOfBody=((uint8_t*)malloc (sizeof(uint8_t)*numofbodies)))){
		perror("malloc error");
		return (1);
	}	
	
	//init cpuz
	if (NULL==(unit=((CPU*)malloc (sizeof(CPU)*nofCPUZ)))){
		perror("malloc error");
		return (1);
	}
	for (i=0; i< nofCPUZ; i++){
		
		unit[i].nofCpuTasks=0;
		
	}
	
	
	//init tasks
	if (NULL==(tasks=((taskT*)malloc (sizeof(taskT)*notasks)))){
		perror("malloc error");
		return (1);
	}
	
	
	//init array of codes
	if (NULL==(code=((uint8_t**)malloc (sizeof(uint8_t*)*numofbodies)))){
		perror("malloc error");
		return (1);
	}
	
	//init localSize
	if (NULL==(localSize=((uint8_t*)malloc (sizeof(uint8_t)*numofbodies)))){
		perror("malloc error");
		return (1);
	}
	
	
	//-----------Bodies-----------------
	
	//----------magic beg----------------
	//repeat for every code body
	for (k=0;k<numofbodies;k++){
		
		printf ("MagicBeg for body %d:", (k+1));
		for (i=0;i<4;i++){
			fread (&magicbeg[i],1,1,fp);
			printf("%x ", magicbeg[i]);
			
		}
		printf("\n");
		
		//check magicbeg
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
			printf("Wrong magicbeg for body %d. Expected DE AD C0 DE\n", (k+1));
			return(1);
		}
		flag=0;
		
		//---------------local size--------------
		
		fread (&localSize[k],1,1,fp);
		printf("Local Size for body %d : %x", (k+1),localSize[k]);
		printf("\n");
		
		
		//---------code size----------------
		
		
		fread (&codeSize,1,1,fp);
		printf("Code size for %d body: %x\n",k, codeSize);
		sizeOfBody[k]=codeSize;
		
		if (NULL==(code[k]=((uint8_t*)malloc (sizeof(uint8_t)*codeSize)))){
			perror("malloc error");
			return (1);
		}
		
		//read code for each body
		printf ("Code for %d body:\n", k);
		for (i=0;i<codeSize;i++){
			fread (&code[k][i] ,1,1,fp);
			if ((i%3)==0){
				printf ("\n");
			}
			
			printf("%x ", code[k][i]);
			
		}
		printf("\n\n");
	}
	
	
	
	//----------read task parameters etc----------
	
	//repeat for every task
	for (k=0;k<notasks;k++){
		printf ("MagicBeg for task %d: ", k);
		for (i=0;i<4;i++){
			fread (&magicbeg[i],1,1,fp);
			printf("%x ", magicbeg[i]);
			
		}
		printf("\n");
		
		//check magic beg
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
			printf("Wrong magicbeg for Task %d. Expected DE AD BA BE\n", k);
			return(1);
		}
		flag=0;
		
		
		//----task body------------------
		
		fread (&tasks[k].body ,1,1,fp);
		printf("Task %d Body: %x", k , tasks[k].body);
		printf("\n");
		
		
		//------argument------------
		fread (&tasks[k].arg ,1,1,fp);
		printf("Task %d argument: %x", k , tasks[k].arg);
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
		
		//insert argument
		
		tasks[k].localMem[(localSize[(tasks[k].body)])]=tasks[k].arg;
		
		
	}
	
	
	//-----------footer---------------------
	printf ("Magic Beg: ");
	for (i=0;i<4;i++){
		fread (&magicbeg[i],1,1,fp);
		printf("%x ", magicbeg[i]);
		
	}
	printf("\n");
	
	//check magic beg
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
		printf("Wrong magicbeg. Expected FE E1 EL DE AD\n");
		return(1);
	}
	flag=0;
	
	
	printf ("Starting program...\n");
	
	
	
	
	//----------execute tasks---------------------
	
	
	shareTask ();
	
	
	if (NULL==(cpuThr=(pthread_t*)malloc(sizeof(pthread_t)*nofCPUZ))){
		perror ("Memory allocation for threads");
	}
	
	for (i=0; i<nofCPUZ; i++){
		
		thrCheck = pthread_create( &cpuThr[i], NULL, CPUact , (void *)i);
		if(thrCheck){
			fprintf(stderr,"Error - pthread_create() return code: %d\n",thrCheck);
			exit(EXIT_FAILURE);
		}
		
	}

	
	
	
	for (i=0; i<nofCPUZ; i++){
		pthread_join(cpuThr[i], NULL);
	}
	
	
	
	free (code);
	free (sizeOfBody);
	free (globalMem);
	for (i=0;i>notasks;i++){
		free (tasks[i].localMem);
	}
	free (tasks);
	free (forGl);
	free (localSize);
	fclose (fp);
	free (blockedFlag);
	
	for (i=0; i<nofCPUZ; i++){
		pthread_mutex_destroy (&unit[i].locked);
		free (unit[i].slaveTasks);}
	free (unit);
	free (cpuThr);
	
	
	return (0);
}








//-------------------Share tasks-------------------

void shareTask(){
	
	int toShare;
	int i, k=0;
	toShare= notasks;
	
	
	
	do{
		
		for (i=0; i< nofCPUZ; i++){
			unit[i].nofCpuTasks++;
			toShare--;
			
			if (toShare==0){
				if ((k==0)&&(i!=(nofCPUZ-1))){
					nofCPUZ=(i+1);
				}
				break;
				
			}
		}
		k++;
		
			
	}while (toShare>0);
	

	
	//initialize cpuz
	if (NULL==(blockedFlag=(int*)malloc(sizeof (int)*nofCPUZ))){
				perror ("Memory allocation error");
				
			}
	for (i=0; i< nofCPUZ; i++){
			
			unit[i].id=i;
			if (NULL==(unit[i].slaveTasks=(int*)malloc(sizeof (int)*unit[i].nofCpuTasks))){
				perror ("Memory allocation error");
				
			}
			if (pthread_mutex_init(&unit[i].locked, NULL) != 0)
			{
				perror ("Mutex error");
			}
			pthread_mutex_lock (&(unit[i].locked));
			blockedFlag[i]=0;
	}
	
	
	toShare=0;
	
	for (i=0; i< nofCPUZ; i++){
		
		for (k=0; k< unit[i].nofCpuTasks; k++){
			
			tasks[toShare].slaveTO= i;
			unit[i].slaveTasks[k]= tasks[toShare].id;
			
			toShare++;
			if (toShare==notasks){break;}
			
		}
		if (toShare==notasks){break;}
	}
	
	
}





//--------------------------PROGRAM EXEC----------------------
void * CPUact(void * cpuCurrId){
	int thisCPU;
	int progress =0;
	int flag=0; //used to show that it
	int i,k;
	uint8_t command[3];
	int endflag=0;
	int curr=0;
	thisCPU=(int)cpuCurrId;
	char * toPrint;
	
	if (NULL==(toPrint=((char*)malloc (sizeof(char)*globalsize)))){
		perror("malloc error");
		
	}
	
	while (1){	
		
		
		//read command
		for (i=0;i<3;i++){
			command[i]=code[(tasks[unit[thisCPU].slaveTasks[curr]].body)-1][tasks[unit[thisCPU].slaveTasks[curr]].pc];
			tasks[unit[thisCPU].slaveTasks[curr]].pc++;
			
		}
		
		//printf ("\n");
		
		switch(command[0]){
			
			// -----------load/store----------------------------
			//LLOAD
			case 0x01 :
					
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=tasks[unit[thisCPU].slaveTasks[curr]].localMem[command[2]];
				progress++;
					
				break;
				//LLOADi	
			case 0x02 :
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=tasks[unit[thisCPU].slaveTasks[curr]].localMem[command[2]+tasks[unit[thisCPU].slaveTasks[curr]].reg[0]];
				progress++;
				break;
				//GLOAD	
			case 0x03 :
				
				pthread_mutex_lock (&check);
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=globalMem[command[2]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				//GLOADi	
			case 0x04 :
				pthread_mutex_lock (&check);
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=globalMem[command[2]+tasks[unit[thisCPU].slaveTasks[curr]].reg[0]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				//LSTORE	
			case 0x05 :
				
				tasks[unit[thisCPU].slaveTasks[curr]].localMem[command[2]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]];
				progress++;
				
				break;
				//LSTOREi	
			case 0x06 :
					
				tasks[unit[thisCPU].slaveTasks[curr]].localMem[command[2]+tasks[unit[thisCPU].slaveTasks[curr]].reg[0]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]];
				progress++;
					
				break;
				//GSTORE	
			case 0x07 :
				pthread_mutex_lock (&check);
				globalMem[command[2]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				//GSTOREi	
			case 0x08 :
				pthread_mutex_lock (&check);
				globalMem[command[2]+tasks[unit[thisCPU].slaveTasks[curr]].reg[0]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				
				//-----------registers--------------------
				
				//SET	
			case 0x09:
					
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=command[2];
				progress++;
					
				break;
				//ADD	
			case 0x0A:
					
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]+tasks[unit[thisCPU].slaveTasks[curr]].reg[command[2]];
				progress++;
				
				break;
				//SUB	
			case 0x0B:
					
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]-tasks[unit[thisCPU].slaveTasks[curr]].reg[command[2]];
				progress++;
				
				
				break;
				//MUL	
			case 0x0C:
					
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]*tasks[unit[thisCPU].slaveTasks[curr]].reg[command[2]];
				progress++;
			
				break;
				
				//DIV	
			case 0x0D:
					
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]/tasks[unit[thisCPU].slaveTasks[curr]].reg[command[2]];
				progress++;
					
				break;
				
				//MOD	
			case 0x0E:
				
				tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]=tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]%tasks[unit[thisCPU].slaveTasks[curr]].reg[command[2]];
				progress++;
					
				break;
				
				// --------- branches---------------------
				//BRGZ	
			case 0x0F:
					
				if (tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]>0){
					tasks[unit[thisCPU].slaveTasks[curr]].pc=(tasks[unit[thisCPU].slaveTasks[curr]].pc+(command[2]*3))%(sizeOfBody[((tasks[unit[thisCPU].slaveTasks[curr]].body)-1)]);
				}
				progress++;
		
				break;
				//BRGEZ	
			case 0x10:
				
				if (tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]>=0){
					tasks[unit[thisCPU].slaveTasks[curr]].pc=(tasks[unit[thisCPU].slaveTasks[curr]].pc+(command[2]*3))%(sizeOfBody[((tasks[unit[thisCPU].slaveTasks[curr]].body)-1)]);
				}
				progress++;
			
				
				break;
				//BRLZ	
			case 0x11:
					
				if (tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]<0){
					tasks[unit[thisCPU].slaveTasks[curr]].pc=(tasks[unit[thisCPU].slaveTasks[curr]].pc+(command[2]*3))%(sizeOfBody[((tasks[unit[thisCPU].slaveTasks[curr]].body)-1)]);
				}
				progress++;
				
				break;
				//BRLEZ	
			case 0x12:
					
				if (tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]<=0){
					tasks[unit[thisCPU].slaveTasks[curr]].pc=(tasks[unit[thisCPU].slaveTasks[curr]].pc+(command[2]*3))%(sizeOfBody[((tasks[unit[thisCPU].slaveTasks[curr]].body)-1)]);
				}
				progress++;
				
				break;
				//BREZ	
			case 0x13:
					
				if (tasks[unit[thisCPU].slaveTasks[curr]].reg[command[1]]==0){
					tasks[unit[thisCPU].slaveTasks[curr]].pc=(tasks[unit[thisCPU].slaveTasks[curr]].pc+(command[2]*3))%(sizeOfBody[((tasks[unit[thisCPU].slaveTasks[curr]].body)-1)]);
				}
				progress++;
					
				break;
				//BRA	
			case 0x14:
				
				tasks[unit[thisCPU].slaveTasks[curr]].pc=(tasks[unit[thisCPU].slaveTasks[curr]].pc+(command[2]*3))%(sizeOfBody[((tasks[unit[thisCPU].slaveTasks[curr]].body)-1)]);
				progress++;
				
				break;
				
				// ----------- synch ------------------
			//DOWN	
			case 0x15:
				pthread_mutex_lock (&check);
				globalMem[command[2]]--;
				progress++;
				
				if (globalMem[command[2]]<0) {
					strcpy(tasks[unit[thisCPU].slaveTasks[curr]].state,"BLOCKED");
					
					tasks[tasks[unit[thisCPU].slaveTasks[curr]].id].sem=command[2];
					
					flag=1;

				}
				
				
				
				pthread_mutex_unlock (&check);
				break;
				
				
			//UP	
			case 0x16:
				pthread_mutex_lock (&check);
				globalMem[command[2]]++;
				
				if (globalMem[command[2]]<=0) {
					for (i=0; i<notasks; i++){

						

						if (tasks[i].sem==command[2]){
							strcpy(tasks[i].state,"READY");
							tasks[i].sem=-1;
							
							if (blockedFlag[tasks[i].slaveTO]!=0){

								blockedFlag[tasks[i].slaveTO]=0;
								nextCurr =tasks[i].id;
								pthread_mutex_unlock (&unit[tasks[i].slaveTO].locked);
							}
							break;
						}
					}
					
				}
				
				progress++;
				pthread_mutex_unlock (&check);
				break;
				
				//----------------varia--------------
				
				//YIELD	
			case 0x17:
				
				progress++;
				
				flag=1;
				
				break;
				
				//SLEEP	
			case 0x18:
				pthread_mutex_lock (&check);
				progress++;
				strcpy(tasks[unit[thisCPU].slaveTasks[curr]].state,"SLEEPING");
				
				tasks[unit[thisCPU].slaveTasks[curr]].waket= command[2]+ time(NULL);
					pthread_mutex_unlock (&check);
				break;
				//PRINT INTEGER	
			case 0x19:
				pthread_mutex_lock (&check);
				progress++;
				printf ("%d:%d\n", tasks[unit[thisCPU].slaveTasks[curr]].id, globalMem[command[2]]);
				pthread_mutex_unlock (&check);
				break;
				//EXIT	
			case 0x1a:
				
				strcpy(tasks[unit[thisCPU].slaveTasks[curr]].state,"STOPPED");
				flag=1;
				
				break;
			//PRINT STRING
			case 0x1b:
				
				k=0;
				pthread_mutex_lock (&check);
				progress++;
				for (i=command[2]; i< globalsize; i++){
					if (globalMem[i]==0){break;}
					
					toPrint[k] = globalMem[i];
					k++;
					
				}
				toPrint[i]='\0';
				
				
				printf ("%s\n", toPrint);
				pthread_mutex_unlock (&check);
				break;
				
				
		}
		//--------end of case-------------
		
		
			
		pthread_mutex_lock (&check);
		//check if it is time to change task
		if ((progress==turns)||(flag==1)){
			
			progress=0;
			
			//find next task which is not blocked
			
			for (i=0;i<unit[thisCPU].nofCpuTasks;i++){
				
				if ((i!=curr)&&(strcmp(tasks[unit[thisCPU].slaveTasks[i]].state,"BLOCKED"))&&(strcmp(tasks[unit[thisCPU].slaveTasks[i]].state,"STOPPED"))){
					
					if (((!strcmp(tasks[unit[thisCPU].slaveTasks[i]].state,"SLEEPING"))&&(time(NULL)>tasks[unit[thisCPU].slaveTasks[i]].waket))){
						
						strcpy (tasks[unit[thisCPU].slaveTasks[i]].state,"READY");
						tasks[unit[thisCPU].slaveTasks[i]].waket=0;
						break;
					}
					else if ((strcmp(tasks[unit[thisCPU].slaveTasks[i]].state,"SLEEPING"))){
						
						break;
					}
				}
				
			}
				
			
			//if there is not such task terminate program
			if ((i==unit[tasks[unit[thisCPU].slaveTasks[curr]].slaveTO].nofCpuTasks)&&((!strcmp(tasks[unit[thisCPU].slaveTasks[curr]].state,"BLOCKED"))||(!strcmp(tasks[unit[thisCPU].slaveTasks[curr]].state,"STOPPED")))){
				
				endflag=1;
			}//else change task
			else if ((i==unit[tasks[unit[thisCPU].slaveTasks[curr]].slaveTO].nofCpuTasks)&&((!strcmp(tasks[unit[thisCPU].slaveTasks[curr]].state,"READY"))));
			else {
				
				curr=i;
			}
			flag=0;
		}
		pthread_mutex_unlock (&check);
		//if there is not available task terminate
		
		if (endflag==1){
		k=0;
		for (i=0; i<nofCPUZ; i++){
			
			if (blockedFlag[i]!=0){
				k++;
				
			}
			
			}
			if (k==nofCPUZ-1){
				terminateFlag=1;
			}
			
		}
		
		if ((endflag==1)&&(terminateFlag==1)){
			reallyTerminate=1;
			for (i=0; i<nofCPUZ;i++){
				pthread_mutex_unlock (&unit[i].locked);
			}
			
		}
		else if (endflag==1){
			blockedFlag[tasks[unit[thisCPU].slaveTasks[curr]].slaveTO]=1;
			
			pthread_mutex_lock (&unit[tasks[unit[thisCPU].slaveTasks[curr]].slaveTO].locked);
		
			endflag=0;
			for (i=0; i<unit[tasks[unit[thisCPU].slaveTasks[curr]].slaveTO].nofCpuTasks; i++){
				
				if (tasks[unit[thisCPU].slaveTasks[i]].id==nextCurr){
					
					curr=i;
					break;
				}
			}
			progress=0;
		}
	if (reallyTerminate==1){
		break;
	}

	}
	free (toPrint);
	
	return (NULL);
}
