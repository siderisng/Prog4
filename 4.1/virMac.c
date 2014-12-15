#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define turns 10 //commands executed before switching task
#define type8_t uint8_t //uint8_t for unsigned int8_t for signed
#define type16_t uint16_t//uint16_t for unsigned int16_t for signed

type8_t ** code; // store code here
type8_t * sizeOfBody;// size of code for each body
int curr; //current executed task
int * globalMem; //global memory
char * toPrint;


typedef struct task{  //memory for each task
	type8_t body;    // body to link task to
	type8_t arg;     //task arguments
	int id;          //task id
	char state[10];  //task state
	int reg[8];      //task registers (we used 8 reg0 is idx)
	type8_t pc;      // next command fotype8_t sem;r task
	int sem;     //sem in which task is blocked
	time_t waket;    // time to wait if sleep is called
	int * localMem;  //local mem for each task
}taskT;

int main (int argc, char * argv[]){
	
	srand(time(NULL)); //init time
	FILE * fp;        //for file reading
	type8_t command[3];    //commands to execute
	int i,k,flag=0, endflag=0;  
	type8_t magicbeg[4]; //for reading the magicbegz
	type8_t globalsize; // number of globals
	type8_t numofbodies;// number of bodies
	type16_t totalcodesize;// size of code
	type8_t notasks; // number of tasks
	type8_t codeSize; // size of code for each body
	type8_t * localSize; //size of locals
	type8_t * forGl; // reads globalinitials and stores them to globalMem(unsigned->signed)
	taskT * tasks;  //array of tasks
	
	
	
	//open file
	if (argc < 2){ 
		printf ("Not enough arguments\n");
	}
	
	if( access( argv[1], R_OK ) != -1 ) {
    fp=fopen(argv[1], "r");
	} else {
    perror ("Please, try again");
	return (1);
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
	
	//init global memory
	if (NULL==(toPrint=((char*)malloc (sizeof(char)*globalsize)))){
		perror("malloc error");
		return (1);
	}
	
	if (NULL==(globalMem=((int*)malloc (sizeof(int)*globalsize)))){
		perror("malloc error");
		return (1);
	}
	if (NULL==(forGl=((type8_t*)malloc (sizeof(type8_t)*globalsize)))){
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
	if (NULL==(sizeOfBody=((type8_t*)malloc (sizeof(type8_t)*numofbodies)))){
		perror("malloc error");
		return (1);
	}	
	
	//init tasks
	if (NULL==(tasks=((taskT*)malloc (sizeof(taskT)*notasks)))){
		perror("malloc error");
		return (1);
	}
	
	//init array of codes
	if (NULL==(code=((type8_t**)malloc (sizeof(type8_t*)*numofbodies)))){
		perror("malloc error");
		return (1);
	}
	
	//init localSize
	if (NULL==(localSize=((type8_t*)malloc (sizeof(type8_t)*numofbodies)))){
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
		
		if (NULL==(code[k]=((type8_t*)malloc (sizeof(type8_t)*codeSize)))){
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
	curr=0;
	int progress=0;
	k=0;
	
	
	while (1){
		
		//read command
		for (i=0;i<3;i++){
			command[i]=code[(tasks[curr].body)-1][tasks[curr].pc];
			tasks[curr].pc++;
			
		}
		
		
		
		switch(command[0]){
			
			// -----------load/store----------------------------
			//LLOAD
			case 0x01 :
				tasks[curr].reg[command[1]]=tasks[curr].localMem[command[2]];
				progress++;
				break;
			//LLOADi	
			case 0x02 :
				tasks[curr].reg[command[1]]=tasks[curr].localMem[command[2]+tasks[curr].reg[0]];
				progress++;
				break;
			//GLOAD	
			case 0x03 :
				tasks[curr].reg[command[1]]=globalMem[command[2]];
				progress++;
				break;
			//GLOADi	
			case 0x04 :
				tasks[curr].reg[command[1]]=globalMem[command[2]+tasks[curr].reg[0]];
				progress++;
				break;
			//LSTORE	
			case 0x05 :
				tasks[curr].localMem[command[2]]=tasks[curr].reg[command[1]];
				progress++;
				break;
			//LSTOREi	
			case 0x06 :
				tasks[curr].localMem[command[2]+tasks[curr].reg[0]]=tasks[curr].reg[command[1]];
				progress++;
				break;
			//GSTORE	
			case 0x07 :
				globalMem[command[2]]=tasks[curr].reg[command[1]];
				progress++;
				break;
			//GSTOREi	
			case 0x08 :
				globalMem[command[2]+tasks[curr].reg[0]]=tasks[curr].reg[command[1]];
				progress++;
				break;
				
				//-----------registers--------------------
			
			//SET	
			case 0x09:
				tasks[curr].reg[command[1]]=command[2];
				progress++;
				break;
			//ADD	
			case 0x0A:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]+tasks[curr].reg[command[2]];
				progress++;
				break;
			//SUB	
			case 0x0B:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]-tasks[curr].reg[command[2]];
				progress++;
				

				break;
			//MUL	
			case 0x0C:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]*tasks[curr].reg[command[2]];
				progress++;
				break;
			//DIV	
			case 0x0D:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]/tasks[curr].reg[command[2]];
				progress++;
				break;
			//MOD	
			case 0x0E:
				tasks[curr].reg[command[1]]=tasks[curr].reg[command[1]]%tasks[curr].reg[command[2]];
				progress++;
				break;
				
				// --------- branches---------------------
			//BRGZ	
			case 0x0F:
				if (tasks[curr].reg[command[1]]>0){
					tasks[curr].pc=(tasks[curr].pc+(command[2]*3))%(sizeOfBody[((tasks[curr].body)-1)]);
				}
				progress++;
				
				break;
			//BRGEZ	
			case 0x10:
				if (tasks[curr].reg[command[1]]>=0){
					tasks[curr].pc=(tasks[curr].pc+(command[2]*3))%(sizeOfBody[((tasks[curr].body)-1)]);
				}
				progress++;
				
				break;
			//BRLZ	
			case 0x11:
				if (tasks[curr].reg[command[1]]<0){
					tasks[curr].pc=(tasks[curr].pc+(command[2]*3))%(sizeOfBody[((tasks[curr].body)-1)]);
				}
				progress++;
				
				break;
			//BRLEZ	
			case 0x12:
				if (tasks[curr].reg[command[1]]<=0){
					tasks[curr].pc=(tasks[curr].pc+(command[2]*3))%(sizeOfBody[((tasks[curr].body)-1)]);
				}
				progress++;
				
				break;
			//BREZ	
			case 0x13:
				if (tasks[curr].reg[command[1]]==0){
					tasks[curr].pc=(tasks[curr].pc+(command[2]*3))%(sizeOfBody[((tasks[curr].body)-1)]);
				}
				progress++;
				
				break;
			//BRA	
			case 0x14:
				tasks[curr].pc=(tasks[curr].pc+(command[2]*3))%(sizeOfBody[((tasks[curr].body)-1)]);
				progress++;
				
				break;
				
				// ----------- synch ------------------
			//DOWN	
			case 0x15:
				
				globalMem[command[2]]--;
				progress++;
				
				if (globalMem[command[2]]<0) {
					strcpy(tasks[curr].state,"BLOCKED");
					tasks[curr].sem=command[2];
					flag=1;
				}
				
				break;
			//UP	
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
				
			//YIELD	
			case 0x17:
				
				progress++;
				
				flag=1;
				
				break;
			
			//SLEEP	
			case 0x18:
				
				progress++;
				strcpy(tasks[i].state,"SLEEPING");
				
				tasks[curr].waket= command[2]+ time(NULL);
				
				break;
			//PRINT INTEGER
			case 0x19:
				progress++;
				//printf("Task %d:",tasks[curr].id);
				printf ("%d\n", globalMem[command[2]]);
				
				break;
			//EXIT	
			case 0x1a:
				
				strcpy(tasks[curr].state,"STOPPED");
				flag=1;
				
				break;
				
			//PRINT STRING
			case 0x1b:
				k=0;
				
				progress++;
				for (i=command[2]; i< globalsize; i++){
					if (globalMem[i]==0){break;}
					
					toPrint[k] = globalMem[i];
					k++;
					
				}
				toPrint[i]='\0';
				
				//printf("Task %d:",tasks[curr].id);
				printf ("%s\n", toPrint);
				
				break;
				
		
		}
		//--------end of case-------------
		
		
		//check if it is time to change task
		if ((progress==turns)||(flag==1)){
			
			progress=0;
			
			//find next task which is not blocked
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
			
			//if there is not such task terminate program
			if ((i==notasks)&&((!strcmp(tasks[curr].state,"BLOCKED"))||(!strcmp(tasks[curr].state,"STOPPED")))){
				
				endflag=1;
			}else if ((i==notasks)&&((!strcmp(tasks[curr].state,"READY"))));
			else if (i!=notasks){
				curr=i;
			}
			flag=0;
		}
		
		//if there is not available task terminate
		if (endflag==1){
			printf ("End of programm\n");
			break;
		}
	}
	free (sizeOfBody);
	free (globalMem);
	for (i=0;i>notasks;i++){
		free (tasks[i].localMem);
	}
	free (tasks);
	free (forGl);
	free (localSize);
	fclose (fp);
	
	
	
	return (0);
}
