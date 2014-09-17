/*
 CITS2002 Project 1 2014
 Name(s):           ITALO GOMES SANTANA, GUILHERME CASTAGNARA SUTILI
 Student number(s):	21382104, 21387071
 Date:              date-of-submission
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>


#define DISK_RPM          5400
#define DISK_SECTORS      63
#define SECTORS_PER_SEC   (DISK_RPM/60.0*DISK_SECTORS)


typedef struct IOEvents {
	int milliseconds;
	int diskSectorNumber;
	bool blocked;
} IOEvents;


typedef struct Process {
	int millisecondsSpawned, millisecondsFinished;
	int millisecondsExecuted,millisecondsLimit,millisecondsBlockedStart;
	IOEvents events[50];
	int currentIO,eventsCounter;
	bool performed,spawned;
} Process;

Process process[100];
int currentSector,schedullingTime,actualTime,processCounter;
bool stillProcessToRun;
int readingMilliseconds, readingPID, readingDiskSectorNumber;
char readingEventType[7], spawn[] = "spawn", io[] = "io";
int readyList[100], blockedList[100],readyListCounter,blockedListCounter;
int output1,output2,inExecution;
int executedTime;


void init();
int millisecondsToMicroseconds(int milliseconds);
void printStep(int PID,char string[],int diskSectorNumber);
int getCurrentSector();
bool CPU();
void moveReadyList();
bool allProcessPerformed();
void queueUpBlockedProcess();
int getSectorByTime(int arbitraryTime);	
void spawnProcess();
void printReadyList();


int main( int argc, char *argv[]) {

    if(argc != 3)
    {
        printf("You must provide 2 parameters.\n Example of input: ./output events3 20\n\n");
        return 0;
    }

    FILE *inputFile = fopen(argv[1],"r");
    if (inputFile == NULL)
    {
        printf("The program couldn't open the file. Please, check if the filepath is correct or the file permissions.\n\n");
    }

    init();
    schedullingTime = millisecondsToMicroseconds(atoi(argv[2]));
   
   
    
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
			process[readingPID].eventsCounter++;
			process[readingPID].currentIO = 0;

    	} else { 
    		process[readingPID].millisecondsLimit = millisecondsToMicroseconds(readingMilliseconds);
    	}

    	if(actualTime == 0){
    		actualTime = millisecondsToMicroseconds(readingMilliseconds);
    		readyList[readyListCounter++] = 0;
			process[0].spawned = true;

			printStep(0,"spawn -> ready",0);
			printStep(0,"ready -> running",0);
    	}

    }
    
    printReadyList();
	while(CPU()) {
		printReadyList();
		
		
	}

	printf("%d   %d\n",output1/1000,output2/1000);
	return 0;
}

bool CPU() {

	if(allProcessPerformed()) {
		return false;
	}
	
	inExecution = readyList[0];

	if(process[inExecution].currentIO > -1 && 
		process[inExecution].currentIO < process[inExecution].eventsCounter &&
		process[inExecution].events[process[inExecution].currentIO].milliseconds == process[inExecution].millisecondsExecuted &&
		!process[inExecution].events[process[inExecution].currentIO].blocked) {

			process[inExecution].events[process[inExecution].currentIO].blocked = true;
			process[inExecution].millisecondsBlockedStart = actualTime;
			printStep(inExecution,"ready -> blocked",process[inExecution].events[process[inExecution].currentIO].diskSectorNumber);
			executedTime = 0;
			moveReadyList();
			inExecution = readyList[0];
	}
	currentSector = getCurrentSector();

	queueUpBlockedProcess();
	
	if (readyListCounter > 0) {
		inExecution = readyList[0];	
		

		if(	process[inExecution].currentIO > -1 && 
			process[inExecution].currentIO < process[inExecution].eventsCounter &&
				currentSector == process[inExecution].events[process[inExecution].currentIO].diskSectorNumber &&
				!process[inExecution].events[process[inExecution].currentIO].blocked
			) {
			
			if(
				process[inExecution].currentIO + 1 < process[inExecution].eventsCounter &&
				process[inExecution].events[process[inExecution].currentIO + 1].milliseconds < process[inExecution].millisecondsExecuted + schedullingTime
				) {
				 

				executedTime = process[inExecution].events[process[inExecution].currentIO + 1].milliseconds - process[inExecution].events[process[inExecution].currentIO ].milliseconds;
				
				process[inExecution].millisecondsExecuted += executedTime;
				actualTime += executedTime;

			} else {

			 	if( process[inExecution].millisecondsExecuted + schedullingTime > process[inExecution].millisecondsLimit ){

						executedTime = process[inExecution].millisecondsLimit - process[inExecution].millisecondsExecuted;

						actualTime += executedTime;
						process[inExecution].millisecondsExecuted += executedTime;

				} else{
				
					process[inExecution].millisecondsExecuted += schedullingTime;
					actualTime += schedullingTime;
					executedTime = schedullingTime;
				}

			}
			
			process[inExecution].currentIO++;
			
			return true;
		} else if(	
					process[inExecution].currentIO > -1 && 
					process[inExecution].currentIO < process[inExecution].eventsCounter &&
					!process[inExecution].events[process[inExecution].currentIO].blocked &&
					process[inExecution].events[process[inExecution].currentIO].milliseconds > process[inExecution].millisecondsExecuted && 
					process[inExecution].events[process[inExecution].currentIO].milliseconds < process[inExecution].millisecondsExecuted + schedullingTime
				) {

					if(
						process[inExecution].events[process[inExecution].currentIO].milliseconds > process[inExecution].millisecondsExecuted && 
						process[inExecution].events[process[inExecution].currentIO].milliseconds <= process[inExecution].millisecondsExecuted + schedullingTime 
						) {
						 
						
						executedTime = process[inExecution].events[process[inExecution].currentIO].milliseconds - process[inExecution].millisecondsExecuted ;

						process[inExecution].millisecondsExecuted += executedTime;
						actualTime += executedTime;

					} else {
						 
						process[inExecution].millisecondsExecuted += schedullingTime;
						actualTime += schedullingTime;
						executedTime = schedullingTime;

					}

					process[inExecution].events[process[inExecution].currentIO].blocked = true;
					process[inExecution].millisecondsBlockedStart = actualTime;
					
					moveReadyList();
					
					printStep(inExecution,"running -> blocked",process[inExecution].events[process[inExecution].currentIO].diskSectorNumber);
					
					return true;
		} 
		else if(process[inExecution].millisecondsExecuted < process[inExecution].millisecondsLimit){

				if( process[inExecution].millisecondsExecuted + schedullingTime > process[inExecution].millisecondsLimit ){

						executedTime = process[inExecution].millisecondsLimit - process[inExecution].millisecondsExecuted;

						actualTime += executedTime;
						process[inExecution].millisecondsExecuted += executedTime;

				} else {
				
					process[inExecution].millisecondsExecuted += schedullingTime;
					actualTime += schedullingTime;
					executedTime = schedullingTime;
				}
			
		}
		
		if( process[inExecution].millisecondsExecuted >= process[inExecution].millisecondsLimit  ) {
			moveReadyList();
			printStep(inExecution,"ready -> exit",0);
			process[inExecution].performed = true;
			process[inExecution].millisecondsFinished = actualTime;
			output1 += process[inExecution].millisecondsFinished - process[inExecution].millisecondsSpawned;
		} else {
			
			
			moveReadyList();
			readyList[readyListCounter++] = inExecution;
			if(readyListCounter > 1){
				printStep(inExecution,"running -> ready",0);
			} else {
				printStep(inExecution,"running -> running",0);
			}
			
		}

	} else {

		actualTime ++;
		executedTime = 1;
		spawnProcess();
	}

	return true;
}

void queueUpBlockedProcess() {
	int sector = getSectorByTime(actualTime);
	for (int i = 0; i < processCounter; i++)
	{
		if(	process[i].currentIO > -1 && 
			process[i].currentIO < process[i].eventsCounter &&
			sector == process[i].events[process[i].currentIO].diskSectorNumber && 
			process[i].events[process[i].currentIO].blocked
		){
			printStep(i,"blocked -> ready",sector);
			if(readyListCounter == 0){
				printStep(i,"ready -> running",0);
			}
			process[i].events[process[i].currentIO].blocked = false;
			output2 += (actualTime - process[inExecution].millisecondsBlockedStart);
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

void spawnProcess(){
	
	if(executedTime == 1){
		for (int i = 0; i < processCounter; i++)
		{
			if(process[i].millisecondsSpawned == actualTime && !process[i].spawned){
				printStep(i,"spawn -> ready",0);
				readyList[readyListCounter++] = i;
				process[i].spawned = true;
				printReadyList();
			}
		}
	} else {
	
		for (int i = 0; i < processCounter; i++)
		{
			if(process[i].millisecondsSpawned > (actualTime - executedTime) && process[i].millisecondsSpawned < actualTime && !process[i].spawned){
				printStep(i,"spawn -> ready",-1);
				readyList[readyListCounter++] = i;
				process[i].spawned = true;
				printReadyList();
			}
		}
	}
	
}


void printReadyList() {
	return;
	if(readyListCounter > 0) {
		for (int i = readyListCounter-1; i >=0; i--)
		{
			printf("[%d]",readyList[i]);
		}
		printf("\n");
	} else {
		printf("[ ]\n");
	}
}



void moveReadyList() {
	
	spawnProcess();
    if(readyListCounter > 0) {
        for(int i=1 ; i < readyListCounter ; i++) {
            readyList[i-1] = readyList[i];
        }
        readyListCounter--;
    }
}

int getCurrentSector() {
	return getSectorByTime(actualTime);
}

int getSectorByTime(int arbitraryTime) {
	return ((int)((arbitraryTime/1000000.0) * SECTORS_PER_SEC)) % DISK_SECTORS;
}

void init() {
	currentSector 					= 0;
	schedullingTime 				= 0;
	stillProcessToRun 				= true;
	actualTime						= 0;
	processCounter					= 0;
	readyListCounter				= 0;
	blockedListCounter				= 0;
	output1							= 0;
	output2							= 0;
	inExecution						= 0;
}

int millisecondsToMicroseconds(int milliseconds) {
	return milliseconds * (int)pow(10,3);
}

void printStep(int PID,char string[],int diskSectorNumber) {
	if(diskSectorNumber == -1  ){
		
		printf("@%d    %d    %s",process[PID].millisecondsSpawned,PID,string);

	} else {
		printf("@%d    %d    %s",actualTime,PID,string);
		if(diskSectorNumber != 0 ){
			printf("    %d",diskSectorNumber);
		}
	}
	printf("\n");

}
