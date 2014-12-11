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
#define turns 10 //commands executed before switching task




//------------variables---------------
uint8_t ** code; // store code here
uint8_t * sizeOfBody;// size of code for each body
uint8_t globalsize; // number of globals
int * globalMem; //global memory
uint8_t notasks; // number of tasks
int nofCPUZ = N;
pthread_mutex_t check ,mon;
char * toPrint;
int nextCurr;
int *blockedFlag;
//-------------structs-------------
typedef struct task{  //memory for each task
	int anothID;
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
	taskT * cpuTask;
	pthread_mutex_t locked;

	
}CPU;
CPU * unit;

//-----------functions-------------


void *CPUact(void * toDo);
void shareTask ();


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
	printf ("HIIII\n");
	
	//-----------init Mutex------------
	if (pthread_mutex_init(&check, NULL) != 0)
	{
		perror ("Mutex error");
		return 1;
	}
	
	if (pthread_mutex_init(&mon, NULL) != 0)
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
	printf ("HIIII\n");
	
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
	if (NULL==(toPrint=((char*)malloc (sizeof(char)*globalsize)))){
		perror("malloc error");
		return (1);
	}
	
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
	
	
	shareTask (unit, tasks);
	
	
	if (NULL==(cpuThr=(pthread_t*)malloc(sizeof(pthread_t)*nofCPUZ))){
		perror ("Memory allocation for threads");
	}
	
	for (i=0; i<nofCPUZ; i++){
		
		thrCheck = pthread_create( &cpuThr[i], NULL, CPUact , (void*)unit[i].cpuTask);
		if(thrCheck){
			fprintf(stderr,"Error - pthread_create() return code: %d\n",thrCheck);
			exit(EXIT_FAILURE);
		}
		
	}

	
	
	
	for (i=0; i<nofCPUZ; i++){
		pthread_join(cpuThr[i], NULL);
	}
	
	
	
	
	//free (sizeOfBody);
	//free (globalMem);
	//for (i=0;i>notasks;i++){
	//	free (tasks[i].localMem);
	//}
	//free (tasks);
	//free (forGl);
	//free (localSize);
	//fclose (fp);
	
	
	
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
	
	
	if (NULL==(blockedFlag=(int*)malloc(sizeof (int)*nofCPUZ))){
				perror ("Memory allocation error");
				
			}
	for (i=0; i< nofCPUZ; i++){
			
		
		
			if (NULL==(unit[i].cpuTask=(taskT*)malloc(sizeof (taskT)*(unit[i].nofCpuTasks)))){
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
			unit[i].cpuTask[k]= tasks[toShare];
			
			toShare++;
			if (toShare==notasks){break;}
			
		}
		if (toShare==notasks){break;}
	}
	
	
}





//--------------------------PROGRAM EXEC----------------------
void * CPUact(void * toDo){
	taskT * taskPre;
	int progress =0;
	int flag=0,i;
	uint8_t command[3];
	int endflag=0;
	int curr=0;
	taskPre= (taskT*)toDo;
	
	
	
	while (1){	
		
		
		//read command
		for (i=0;i<3;i++){
			command[i]=code[(taskPre[curr].body)-1][taskPre[curr].pc];
			taskPre[curr].pc++;
			printf ("task : %d command:%x\n", taskPre[curr].id,command[i]);
		}
		
		printf ("\n");
		
		switch(command[0]){
			
			// -----------load/store----------------------------
			//LLOAD
			case 0x01 :
					pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=taskPre[curr].localMem[command[2]];
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//LLOADi	
			case 0x02 :
				taskPre[curr].reg[command[1]]=taskPre[curr].localMem[command[2]+taskPre[curr].reg[0]];
				progress++;
				break;
				//GLOAD	
			case 0x03 :
				
				pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=globalMem[command[2]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				//GLOADi	
			case 0x04 :
				pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=globalMem[command[2]+taskPre[curr].reg[0]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				//LSTORE	
			case 0x05 :
					pthread_mutex_unlock (&check);
				taskPre[curr].localMem[command[2]]=taskPre[curr].reg[command[1]];
				progress++;
					pthread_mutex_lock (&check);
				break;
				//LSTOREi	
			case 0x06 :
					pthread_mutex_lock (&check);
				taskPre[curr].localMem[command[2]+taskPre[curr].reg[0]]=taskPre[curr].reg[command[1]];
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//GSTORE	
			case 0x07 :
				pthread_mutex_lock (&check);
				globalMem[command[2]]=taskPre[curr].reg[command[1]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				//GSTOREi	
			case 0x08 :
				pthread_mutex_lock (&check);
				globalMem[command[2]+taskPre[curr].reg[0]]=taskPre[curr].reg[command[1]];
				progress++;
				pthread_mutex_unlock (&check);
				break;
				
				//-----------registers--------------------
				
				//SET	
			case 0x09:
					pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=command[2];
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//ADD	
			case 0x0A:
					pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=taskPre[curr].reg[command[1]]+taskPre[curr].reg[command[2]];
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//SUB	
			case 0x0B:
					pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=taskPre[curr].reg[command[1]]-taskPre[curr].reg[command[2]];
				progress++;
					pthread_mutex_unlock (&check);
				
				break;
				//MUL	
			case 0x0C:
					pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=taskPre[curr].reg[command[1]]*taskPre[curr].reg[command[2]];
				progress++;
					pthread_mutex_unlock (&check);
				break;
				
				//DIV	
			case 0x0D:
					pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=taskPre[curr].reg[command[1]]/taskPre[curr].reg[command[2]];
				progress++;
					pthread_mutex_unlock (&check);
				break;
				
				//MOD	
			case 0x0E:
					pthread_mutex_lock (&check);
				taskPre[curr].reg[command[1]]=taskPre[curr].reg[command[1]]%taskPre[curr].reg[command[2]];
				progress++;
					pthread_mutex_unlock (&check);
				break;
				
				// --------- branches---------------------
				//BRGZ	
			case 0x0F:
					pthread_mutex_lock (&check);
				if (taskPre[curr].reg[command[1]]>0){
					taskPre[curr].pc=(taskPre[curr].pc+command[2])%(sizeOfBody[((taskPre[curr].body)-1)]);
				}
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//BRGEZ	
			case 0x10:
					pthread_mutex_lock (&check);
				if (taskPre[curr].reg[command[1]]>=0){
					taskPre[curr].pc=(taskPre[curr].pc+command[2])%(sizeOfBody[((taskPre[curr].body)-1)]);
				}
				progress++;
					pthread_mutex_unlock (&check);
				
				break;
				//BRLZ	
			case 0x11:
					pthread_mutex_lock (&check);
				if (taskPre[curr].reg[command[1]]<0){
					taskPre[curr].pc=(taskPre[curr].pc+command[2])%(sizeOfBody[((taskPre[curr].body)-1)]);
				}
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//BRLEZ	
			case 0x12:
					pthread_mutex_lock (&check);
				if (taskPre[curr].reg[command[1]]<=0){
					taskPre[curr].pc=(taskPre[curr].pc+command[2])%(sizeOfBody[((taskPre[curr].body)-1)]);
				}
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//BREZ	
			case 0x13:
					pthread_mutex_lock (&check);
				if (taskPre[curr].reg[command[1]]==0){
					taskPre[curr].pc=(taskPre[curr].pc+command[2])%(sizeOfBody[((taskPre[curr].body)-1)]);
				}
				progress++;
					pthread_mutex_unlock (&check);
				break;
				//BRA	
			case 0x14:
				pthread_mutex_lock (&check);
				taskPre[curr].pc=(taskPre[curr].pc+command[2])%(sizeOfBody[((taskPre[curr].body)-1)]);
				progress++;
				pthread_mutex_unlock (&check);
				break;
				
				// ----------- synch ------------------
			//DOWN	
			case 0x15:
				pthread_mutex_lock (&check);
				globalMem[command[2]]--;
				progress++;
				
				if (globalMem[command[2]]<0) {
					strcpy(taskPre[curr].state,"BLOCKED");
					
					tasks[taskPre[curr].id].sem=command[2];
					
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
							if ((tasks[i].slaveTO!=taskPre[curr].slaveTO)&&(blockedFlag[tasks[i].slaveTO]!=0)){
								

								blockedFlag[tasks[i].slaveTO]=0;
								nextCurr =tasks[i].anothID;
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
				strcpy(taskPre[curr].state,"SLEEPING");
				
				taskPre[curr].waket= command[2]+ time(NULL);
					pthread_mutex_unlock (&check);
				break;
				//PRINT INTEGER	
			case 0x19:
				pthread_mutex_lock (&check);
				progress++;
				printf ("%d:%d\n", taskPre[curr].id, globalMem[command[2]]);
				pthread_mutex_unlock (&check);
				break;
				//EXIT	
			case 0x1a:
				
				strcpy(taskPre[curr].state,"STOPPED");
				flag=1;
				
				break;
			//PRINT STRING
			case 0x1b:
				
				pthread_mutex_lock (&check);
				progress++;
				for (i=command[2]; i< globalsize; i++){
					if (globalMem[i]==0){break;}
					
					toPrint[i] = globalMem[command[2]+i];
					
					
				}
				toPrint[i]='\0';
				
				printf ("%d:%s\n", taskPre[curr].id, toPrint);
				pthread_mutex_unlock (&check);
				break;
				
		}
		//--------end of case-------------
		
		
			
		pthread_mutex_lock (&check);
		//check if it is time to change task
		if ((progress==turns)||(flag==1)){
			
			progress=0;
			
			//find next task which is not blocked
			
			for (i=0;i<unit[taskPre[curr].slaveTO].nofCpuTasks;i++){
				
				if ((i!=curr)&&(strcmp(taskPre[i].state,"BLOCKED"))&&(strcmp(taskPre[i].state,"STOPPED"))){
					
					if (((!strcmp(taskPre[i].state,"SLEEPING"))&&(time(NULL)>taskPre[i].waket))){
						
						strcpy (taskPre[i].state,"READY");
						taskPre[i].waket=0;
						break;
					}
					else if ((strcmp(taskPre[i].state,"SLEEPING"))){
						
						break;
					}
				}
				
			}
				
			
			//if there is not such task terminate program
			if ((i==unit[taskPre[curr].slaveTO].nofCpuTasks)&&((!strcmp(taskPre[curr].state,"BLOCKED"))||(!strcmp(taskPre[curr].state,"STOPPED")))){
				
				endflag=1;
			}//ele change task
			else if ((i==unit[taskPre[curr].slaveTO].nofCpuTasks)&&((strcmp(taskPre[curr].state,"BLOCKED"))&&(strcmp(taskPre[curr].state,"STOPPED")))){

			}
			else {
				curr=i;
			}
			flag=0;
		}
		pthread_mutex_unlock (&check);
		//if there is not available task terminate
		if (endflag==1){
		
			blockedFlag[taskPre[curr].slaveTO]=1;
			taskPre[curr].anothID=curr;
			pthread_mutex_lock (&unit[taskPre[curr].slaveTO].locked);
			printf ("HIIII\n");
			endflag=0;
			curr= nextCurr;
			progress=0;
		}

	
	}
	
	return (NULL);
}
