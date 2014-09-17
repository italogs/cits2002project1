/*
 CITS2002 Project 1 2014
 Name(s):           ITALO GOMES SANTANA, GUILHERME CASTAGNARA SUTILI
 Student number(s):	21382104, 21387071
 Date:              date-of-submission
 */


/*** 
	INCLUDES AND DEFINES - BEGIN
***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>


#define DISK_RPM          5400
#define DISK_SECTORS      63
#define SECTORS_PER_SEC   (DISK_RPM/60.0*DISK_SECTORS)

/*** 
	INCLUDES AND DEFINES - END
***/


/*** 
	GLOBAL VARIABLES  - BEGIN
***/

/**
	I have made use of structs to control the processes
	Easier to manage data 
**/
typedef struct IOEvents {
	int diskSectorNumber,milliseconds;
	bool blocked,requested;
} IOEvents;

typedef struct Process {
	int millisecondsSpawned,millisecondsFinished,
		millisecondsExecuted,millisecondsLimit,millisecondsBlockedStart,
		currentIO,eventsCounter;
	bool performed,spawned;
	IOEvents events[50];
} Process;

Process process[100];

// Variables for array boundaries
int processCounter,readyList[100], blockedList[100],readyListCounter,blockedListCounter;
int totalTurnaroundTime,totalBlockedTime,globalTime;
/*** 
	GLOBAL VARIABLES  - END
***/	


/*** 
	FUNCTION PROTOTYPES - BEGIN
***/
void initGlobalVariables();

int millisecondsToMicroseconds(int milliseconds);
int getCurrentSector();
int getSectorByTime(int arbitraryTime);	

bool allProcessPerformed();
bool CPU(int timeQuantum);

void moveReadyList(int executedTime);

void queueUpBlockedProcess();
void spawnUnblockProcess(int executedTime);


void printStep(int PID,char string[],int diskSectorNumber,int absoluteTime);
void printReadyList();
/*** 
	FUNCTION PROTOTYPES - END
***/


int main( int argc, char *argv[]) {


	//Checking if the inputs are correct and returning a message if these are incorrect
    if(argc != 3) {
        printf("You must provide 2 parameters.\n Example: ./schedule eventsfile 20\n\n");
        return 0;
    }

    FILE *inputFile = fopen(argv[1],"r");
    if (inputFile == NULL) {
        printf("The program couldn't open the file. Please, check the filepath and the file permissions.\n\n");
    }

    initGlobalVariables();
    int timeQuantum = millisecondsToMicroseconds(atoi(argv[2]));
   
    int readingMilliseconds, readingPID, readingDiskSectorNumber;
    char readingEventType[7], spawn[] = "spawn", io[] = "io";

    //Reading the whole file and setting its data to the designated variables 
    while(fscanf(inputFile,"%d %s %d %d",&readingMilliseconds,readingEventType,&readingPID,&readingDiskSectorNumber) > 0 ) {
    	
    	if(strcmp(readingEventType,spawn) == 0){

			process[readingPID].millisecondsSpawned = millisecondsToMicroseconds(readingMilliseconds);

			process[readingPID].millisecondsFinished = 0;
			
			process[readingPID].millisecondsExecuted = 0;
			process[readingPID].millisecondsLimit = 0;
			process[readingPID].performed = false;
			process[readingPID].spawned = false;

			process[readingPID].currentIO = -1;
			process[readingPID].eventsCounter = 0;
			processCounter++;

    	} else if(strcmp(readingEventType,io) == 0){

			process[readingPID].events[process[readingPID].eventsCounter].milliseconds = millisecondsToMicroseconds(readingMilliseconds);
			process[readingPID].events[process[readingPID].eventsCounter].diskSectorNumber = readingDiskSectorNumber;
			process[readingPID].events[process[readingPID].eventsCounter].blocked = false;
			process[readingPID].eventsCounter++;
			process[readingPID].currentIO = 0;

    	} else { 
    		process[readingPID].millisecondsLimit = millisecondsToMicroseconds(readingMilliseconds);
    	}

    	if(globalTime == 0){
    		globalTime = millisecondsToMicroseconds(readingMilliseconds);
    		readyList[readyListCounter++] = 0;
			process[0].spawned = true;

			printStep(0,"spawn -> ready",-1,globalTime);
			printStep(0,"ready -> running",-1,globalTime);
    	}

    }
    
    //The most important function of the program.
    //Please, check inside it for more details.
	while(CPU(timeQuantum)) {}
	

	//output to the user. 
	printf("%d   %d\n",totalTurnaroundTime/1000,totalBlockedTime/1000);
	return 0;
}


//when this function returns FALSE, it stands for that every process has been performed(complete execution).
//otherwise, the CPU will proceed.
bool CPU(int timeQuantum) {

	//checking if all process were performed
	if(allProcessPerformed()) {
		return false;
	}
	
	int processInExecution = readyList[0];
	int executedTime = 0;
	int currentSector = getCurrentSector();

	// if a process is elegible to be blocked according to its milliseconds 
	if(process[processInExecution].currentIO > -1 && 
		process[processInExecution].currentIO < process[processInExecution].eventsCounter &&
		!process[processInExecution].events[process[processInExecution].currentIO].requested &&
		process[processInExecution].events[process[processInExecution].currentIO].milliseconds == process[processInExecution].millisecondsExecuted &&
		!process[processInExecution].events[process[processInExecution].currentIO].blocked
		) {
			
			process[processInExecution].events[process[processInExecution].currentIO].blocked = true;
			process[processInExecution].events[process[processInExecution].currentIO].requested = true;
			process[processInExecution].millisecondsBlockedStart = globalTime;
			printStep(processInExecution,"ready -> blocked",process[processInExecution].events[process[processInExecution].currentIO].diskSectorNumber,globalTime);
			executedTime = 0;
			moveReadyList(executedTime);
			processInExecution = readyList[0];
	}
	
	// queueing up blocked process that now are elegible to be part of the ready list.
	queueUpBlockedProcess();



	processInExecution = readyList[0];


	//if there is at least one process in the ready list, so the cpu will execute it and apply some rules.
	if (readyListCounter > 0) {
		processInExecution = readyList[0];	

		//The CPU will execute an IO request until the next IO request or reach the time quantum limit.
		if(	process[processInExecution].currentIO > -1 && 
			process[processInExecution].currentIO < process[processInExecution].eventsCounter &&
			currentSector == process[processInExecution].events[process[processInExecution].currentIO].diskSectorNumber &&
			!process[processInExecution].events[process[processInExecution].currentIO].blocked
		) {
			
			if(
				process[processInExecution].currentIO + 1 < process[processInExecution].eventsCounter &&
				process[processInExecution].events[process[processInExecution].currentIO + 1].milliseconds < process[processInExecution].millisecondsExecuted + timeQuantum
			) {
				 

				executedTime = process[processInExecution].events[process[processInExecution].currentIO + 1].milliseconds - process[processInExecution].events[process[processInExecution].currentIO ].milliseconds;
				
				process[processInExecution].millisecondsExecuted += executedTime;
				globalTime += executedTime;

			} else {

			 	if( process[processInExecution].millisecondsExecuted + timeQuantum > process[processInExecution].millisecondsLimit ){

						executedTime = process[processInExecution].millisecondsLimit - process[processInExecution].millisecondsExecuted;

						globalTime += executedTime;
						process[processInExecution].millisecondsExecuted += executedTime;

				} else{
					executedTime = timeQuantum;
					process[processInExecution].millisecondsExecuted += executedTime;
					globalTime += executedTime;
					
				}

			}

			process[processInExecution].currentIO++;
			return true;
		} else if(
				//If the CPU finds an IO request, it will execute until the CPU reach the limit and block the IO request
				process[processInExecution].currentIO > -1 && 
				process[processInExecution].currentIO < process[processInExecution].eventsCounter &&
				!process[processInExecution].events[process[processInExecution].currentIO].blocked &&
				process[processInExecution].events[process[processInExecution].currentIO].milliseconds > process[processInExecution].millisecondsExecuted && 
				process[processInExecution].events[process[processInExecution].currentIO].milliseconds < process[processInExecution].millisecondsExecuted + timeQuantum
			) {	

			if(
				process[processInExecution].events[process[processInExecution].currentIO].milliseconds > process[processInExecution].millisecondsExecuted && 
				process[processInExecution].events[process[processInExecution].currentIO].milliseconds <= process[processInExecution].millisecondsExecuted + timeQuantum 
				) {

				executedTime = process[processInExecution].events[process[processInExecution].currentIO].milliseconds - process[processInExecution].millisecondsExecuted ;

				process[processInExecution].millisecondsExecuted += executedTime;
				globalTime += executedTime;

			} else {
				executedTime = timeQuantum; 
				process[processInExecution].millisecondsExecuted += executedTime;
				globalTime += executedTime;

			}

			
			process[processInExecution].millisecondsBlockedStart = globalTime;
			process[processInExecution].events[process[processInExecution].currentIO].blocked = true;
			process[processInExecution].events[process[processInExecution].currentIO].requested = true;
			moveReadyList(executedTime);

			
			printStep(processInExecution,"running -> blocked",process[processInExecution].events[process[processInExecution].currentIO].diskSectorNumber,globalTime);
			return true;

		}

		if(process[processInExecution].millisecondsExecuted < process[processInExecution].millisecondsLimit){

			//If the process executed time plus the time quantum have overcome the limit, the cpu will execute only the necessary time.
			if( process[processInExecution].millisecondsExecuted + timeQuantum > process[processInExecution].millisecondsLimit ){

				executedTime = process[processInExecution].millisecondsLimit - process[processInExecution].millisecondsExecuted;
				globalTime += executedTime;
				process[processInExecution].millisecondsExecuted += executedTime;

			} else {

				//otherwise, the cpu will execute the whole timeQuantum.
				executedTime = timeQuantum;
				process[processInExecution].millisecondsExecuted += executedTime;
				globalTime += executedTime;
			}

		}
		

		//If the process reached its limit, the CPU will exit it.
		if( process[processInExecution].millisecondsExecuted >= process[processInExecution].millisecondsLimit  ) {
			moveReadyList(executedTime);
			printStep(processInExecution,"ready -> exit",-1,globalTime);
			process[processInExecution].performed = true;
			process[processInExecution].millisecondsFinished = globalTime;
			totalTurnaroundTime += process[processInExecution].millisecondsFinished - process[processInExecution].millisecondsSpawned;
		} else {
		//If not, the CPU will mark it as ready and queue again at the end.
			moveReadyList(executedTime);
			readyList[readyListCounter++] = processInExecution;
			if(readyListCounter > 1){
				printStep(processInExecution,"running -> ready",-1,globalTime);
			} else {
				printStep(processInExecution,"running -> running a", -1,globalTime);
			}
			
		}

	} else {
		// if there is no process in the ready list, the global time will increase one unit of time and wait for 
		// and eventual process to spawn.
		globalTime ++;
		executedTime = 1;
		spawnUnblockProcess(executedTime);
	}

	return true;
}

void queueUpBlockedProcess() {
	int sector = getSectorByTime(globalTime);
	for (int i = 0; i < processCounter; i++)
	{
		if(	process[i].currentIO > -1 && 
			process[i].currentIO < process[i].eventsCounter &&

			sector == process[i].events[process[i].currentIO].diskSectorNumber && 
			process[i].events[process[i].currentIO].blocked
		){
			printStep(i,"blocked -> ready",sector,globalTime);
			if(readyListCounter == 0){
				printStep(i,"ready -> running", -1,globalTime);
			}
			process[i].events[process[i].currentIO].blocked = false;
			totalBlockedTime += (globalTime - process[i].millisecondsBlockedStart);
			readyList[readyListCounter++] = i;
		}
	}
}

bool allProcessPerformed() {
	for (int i = 0; i < processCounter; i++)
	{
		if(!process[i].performed) {
			return false;
		}
	}
	return true;
}

void spawnUnblockProcess(int executedTime){
	
	if(executedTime == 1){
		for (int i = 0; i < processCounter; i++)
		{
			if(process[i].millisecondsSpawned == globalTime && !process[i].spawned){
				printStep(i,"spawn -> ready",-1,globalTime);
				readyList[readyListCounter++] = i;
				process[i].spawned = true;
			}
		}
		queueUpBlockedProcess();
	} else {

		for (int k = (globalTime - executedTime + 1); k <= globalTime; k++)
		{
			for (int i = 0; i < processCounter; i++)
			{
				int sector = getSectorByTime(k);				
				if(process[i].millisecondsSpawned == k && !process[i].spawned){
					printStep(i,"spawn -> ready",-1,k);
					readyList[readyListCounter++] = i;
					process[i].spawned = true;
					printReadyList();
				} 
				else if(
					process[i].currentIO > -1 && 
					process[i].currentIO < process[i].eventsCounter &&
					!process[i].events[process[i].currentIO].requested && 
					sector == process[i].events[process[i].currentIO].diskSectorNumber && 
					process[i].events[process[i].currentIO].blocked
				){
					printStep(i,"blocked -> ready 2",sector,k);
					if(readyListCounter == 0) {
						printStep(i,"ready -> running",-1,k);
					}
					process[i].events[process[i].currentIO].blocked = false;
					totalBlockedTime += (globalTime - process[i].millisecondsBlockedStart);
					readyList[readyListCounter++] = i;
				}
			}
		}

	}
	
}

void initGlobalVariables() {
	globalTime						= 0;
	processCounter					= 0;
	readyListCounter				= 0;
	blockedListCounter				= 0;
	totalTurnaroundTime				= 0;
	totalBlockedTime				= 0;
}

void moveReadyList(int executedTime) {
	spawnUnblockProcess(executedTime);
    if(readyListCounter > 0) {
        for(int i=1 ; i < readyListCounter ; i++) {
            readyList[i-1] = readyList[i];
        }
        readyListCounter--;
    }
}

int getCurrentSector() {
	return getSectorByTime(globalTime);
}

int getSectorByTime(int arbitraryTime) {
	return ((int)((arbitraryTime/1000000.0) * SECTORS_PER_SEC)) % DISK_SECTORS;
}

int millisecondsToMicroseconds(int milliseconds) {
	return milliseconds * (int)pow(10,3);
}

/* DEBUG FUNCTIONS */
void printStep(int PID,char string[],int diskSectorNumber,int absoluteTime) {
	return;
	printf("@%d    %d    %s",absoluteTime,PID,string);
	if(diskSectorNumber >= 0 ){
		printf("    %d",diskSectorNumber);
	}
	printf("\n");
}

void printReadyList() {
	return;
	if(readyListCounter > 0) {
		for (int i = readyListCounter-1; i >=0; i--)
		{
			printf("[%d]",readyList[i]);
		}
		printf("\n");
	}
}


