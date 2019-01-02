// os345p3.c - Jurassic Park
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>
#include "os345.h"
#include "os345park.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore* parkMutex;						// protect park access
extern Semaphore* fillSeat[NUM_CARS];			// (signal) seat ready to fill
extern Semaphore* seatFilled[NUM_CARS];		// (wait) passenger seated
extern Semaphore* rideOver[NUM_CARS];			// (signal) ride over
extern TCB tcb[];
extern int clocks;

int numDeltaClock;
Delta* deltaClock[MAX_TASKS];
int timeTaskID;
int gDriverId;

Semaphore* dcChange;
Semaphore* dcMutex;
Semaphore* resourceMutex;
Semaphore* driverMutex;
Semaphore* ticketMutex;

Semaphore* parkCapacity;
Semaphore* parkCapacityTickets;
Semaphore* parkCapacityMuseum;
Semaphore* parkCapacityGiftShop;
Semaphore* parkAlertDriver;

Semaphore* parkRider;
Semaphore* parkSeat;
Semaphore* parkDriver;
Semaphore* parkTicket;
Semaphore* parkNeedDriver;

Semaphore* visitorNeedDriver;
Semaphore* visitorNeedTicket;
Semaphore* visitorAcquireTicket;


Semaphore* driverNeedPassenger;
Semaphore* driverRiderResource;
Semaphore* driverResource;
Semaphore* driverRiderResourceAcquired;
Semaphore* driverResourceAcquired;

Semaphore* gSemaphore;


// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char**);
void CL3_dc(int, char**);

void printDeltaClock(void);
int insertDeltaClock(int, Semaphore*);

int carTask(int argc, char* argv[]);
int visitorTask(int argc, char* argv[]);
int driverTask(int argc, char* argv[]);

void createVisitor(int, Semaphore*);

void changeToTicketLine();
void reduceTicketAmount();
void waitForTicket(Semaphore*);

void addVisitorToParkLine();

void changeToMuseumLine();
void enterMuseum();
void waitForMuseum(Semaphore*);

void changeToTourCarLine();
void enterTourCar();
void waitForTourCar(Semaphore*);
void placeVisitorInCar(int);
void removeVisitorFromCar(int);

Semaphore* getPassegnerWaitSem();
void passPassengerWaitSem(Semaphore*);
Semaphore* getDriverWaitSem(int);
void passDriverWaitSem(Semaphore*, int);
void updateDriver(int, int);

void changeToGiftShopLine();
void enterGiftShop();
void waitForGiftShop(Semaphore*);

void exitPark();




// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_project3(int argc, char* argv[])
{
	char buf[32];
	char idBuf[32];
	char* newArgv[2];

	// mutex
	dcChange = createSemaphore("dcChange", BINARY, 1);
	dcMutex = createSemaphore("dcMutex", BINARY, 1);
	resourceMutex = createSemaphore("resourceMutex", BINARY, 1);
	ticketMutex = createSemaphore("ticketMutex", BINARY, 1);
	driverMutex = createSemaphore("driverMutex", BINARY, 1);

	// counting
	parkCapacity = createSemaphore("parkCapacity", COUNTING, MAX_IN_PARK);
	parkCapacityTickets = createSemaphore("parkTickets", COUNTING, MAX_TICKETS);
	parkCapacityMuseum = createSemaphore("parkCapacityMuseum", COUNTING, MAX_IN_MUSEUM);
	parkCapacityGiftShop = createSemaphore("parkCapacityGiftShop", COUNTING, MAX_IN_GIFTSHOP);
	parkAlertDriver = createSemaphore("parkAlertDriver", COUNTING, 0);

	//binary
	parkRider = createSemaphore("parkRider", BINARY, 0);
	parkSeat = createSemaphore("parkSeat", BINARY, 0);
	parkDriver = createSemaphore("parkDriver", BINARY, 0);
	parkTicket = createSemaphore("parkTicket", BINARY, 0);
	parkNeedDriver = createSemaphore("parkNeedDriver", BINARY, 0);

	visitorNeedDriver = createSemaphore("visitorNeedDriver", BINARY, 0);
	visitorNeedTicket = createSemaphore("visitorNeedTicket", BINARY, 0);
	visitorAcquireTicket = createSemaphore("visitorAcquireTicket", BINARY, 0);

	driverNeedPassenger = createSemaphore("driverNeedPassenger", BINARY, 0);
	driverRiderResource = createSemaphore("driverRiderResource", BINARY, 0);
	driverResource = createSemaphore("driverResource", BINARY, 0);
	driverRiderResourceAcquired = createSemaphore("driverRiderResourceAcquired", BINARY, 0);
	driverResourceAcquired = createSemaphore("driverResourceAcquired", BINARY, 0);

	// start park
	sprintf(buf, "jurassicPark");
	newArgv[0] = buf;
	createTask( buf,				// task name
		jurassicTask,				// task
		MED_PRIORITY,				// task priority
		1,								// task count
		newArgv);					// task argument

	// wait for park to get initialized...
	while (!parkMutex) SWAP;
	printf("\nStart Jurassic Park...");

	//?? create car, driver, and visitor tasks here

	// create car tasks
	for (int i = 0; i < NUM_CARS; i++) {
		sprintf(buf, "car%d", i);	SWAP;
		sprintf(idBuf, "%d", i);	SWAP;
		newArgv[0] = buf;
		newArgv[1] = idBuf;
		createTask(buf,
			carTask,
			MED_PRIORITY,
			2,
			newArgv);		SWAP;
	}

	// create driver tasks
	for (int i = 0; i < NUM_DRIVERS; i++) {
		sprintf(buf, "driver%d", i);	SWAP;
		sprintf(idBuf, "%d", i);		SWAP;
		newArgv[0] = buf;
		newArgv[1] = idBuf;
		createTask(buf,
			driverTask,
			MED_PRIORITY,
			2,
			newArgv);		SWAP;
	}

	// create visitor tasks
	for (int i = 0; i < NUM_VISITORS; i++) {
		sprintf(buf, "visitor%d", i);	SWAP;
		sprintf(idBuf, "%d", i);		SWAP;
		newArgv[0] = buf;
		newArgv[1] = idBuf;
		createTask(buf,
			visitorTask,
			MED_PRIORITY,
			2,
			newArgv);		SWAP;
	}


	return 0;
} // end project3

int carTask(int argc, char* argv[]) {
	char buf[32];           		                
	int myID = atoi(argv[1]);		                SWAP;	
	int i;                                          SWAP;
	sprintf(buf, "%sTourOver", argv[0]);
	Semaphore* passengerWait[NUM_SEATS];
	Semaphore* driverDone;
	printf("Starting carTask%d", myID);		        SWAP;

	while (1) {
		for (i = 0; i < NUM_SEATS; i++) {
			SEM_WAIT(fillSeat[myID]);               SWAP;
			SEM_SIGNAL(parkRider);               SWAP;
			SEM_WAIT(parkSeat);                    SWAP;
			passengerWait[i] = getPassegnerWaitSem(); SWAP;								   
			SEM_SIGNAL(seatFilled[myID]);            SWAP; 
														  
		}


		SEM_WAIT(driverMutex);
		{
			SEM_SIGNAL(visitorNeedDriver);                 SWAP;
			SEM_SIGNAL(parkAlertDriver);               SWAP;
			driverDone = getDriverWaitSem(myID);    SWAP;
		}
		SEM_SIGNAL(driverMutex);
		SEM_WAIT(rideOver[myID]);                   SWAP;
		SEM_SIGNAL(driverDone);                     SWAP;

		for (i = 0; i < NUM_SEATS; i++)
		{
			SEM_SIGNAL(passengerWait[i]);           SWAP;
		}

		if (myPark.numExitedPark == NUM_VISITORS) {
			break;
		}
	}

	return 0;
}

int visitorTask(int argc, char* argv[]) {

	char buf[32];
	sprintf(buf, "%sWait", argv[0]);
	Semaphore* visitorWait = createSemaphore(buf, BINARY, 0);

	createVisitor(100, visitorWait);

	addVisitorToParkLine();

	// the full adventure of a visitor
	SEM_WAIT(parkCapacity);
	changeToTicketLine();
	waitForTicket(visitorWait);
	changeToMuseumLine();
	waitForMuseum(visitorWait);
	changeToTourCarLine();
	waitForTourCar(visitorWait);
	changeToGiftShopLine();
	waitForGiftShop(visitorWait);
	exitPark();
	SEM_SIGNAL(parkCapacity);

	return 0;
}


int driverTask(int argc, char* argv[]) {
	char buf[32];
	Semaphore* driverDone;
	int myID = atoi(argv[1]);						SWAP;	
	printf(buf, "Starting driverTask%d", myID);		SWAP;
	sprintf(buf, "driverDone%d", myID); 		    SWAP;
	driverDone = createSemaphore(buf, BINARY, 0);	SWAP; 	

	while (1) {
		SEM_WAIT(parkAlertDriver);		                SWAP;
															
		if (SEM_TRYLOCK(visitorNeedDriver)) {
			passDriverWaitSem(driverDone, myID);  	SWAP;
																
			SEM_SIGNAL(parkDriver);		        SWAP;
															
			SEM_WAIT(driverDone);		            SWAP;	

			updateDriver(myID, 0);               SWAP;
			
		}
		else if (SEM_TRYLOCK(visitorNeedTicket)) {
			updateDriver(myID, -1);              SWAP;
			
			SEM_WAIT(parkCapacityTickets);		                SWAP;
															
			SEM_SIGNAL(parkTicket);		        SWAP;
															
			SEM_WAIT(visitorAcquireTicket);                    SWAP;
			updateDriver(myID, 0);
			
		}
		else if (myPark.numExitedPark == NUM_VISITORS) {
			
			break;
		}
		else {
			
		}
	}

	return 0;

}


// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
	printf("\nDelta Clock");
	// ?? Implement a routine to display the current delta clock contents

	printDeltaClock();

	return 0;
} // end CL3_dc


void printDeltaClock(void)
{
	int i;
	for (i=0; i<numDeltaClock; i++)
	{
		printf("\n%4d%4d  %-20s", i, deltaClock[i]->time, deltaClock[i]->sem->name);
	}
	return;
}

int insertDeltaClock(int time, Semaphore *sem)
{
	// delta clock is full
	if (numDeltaClock >= MAX_TASKS) { 
		return -1;
	}

	SEM_WAIT(dcMutex);
	Delta* deltaTemp = (Delta*)malloc(sizeof(Delta)); SWAP;
	deltaTemp->time = time; SWAP;
	deltaTemp->sem = sem; SWAP;

	for (int i = numDeltaClock; i > -1; i--) {
		if (i == 0 || deltaTemp->time < deltaClock[i - 1]->time) {
			deltaClock[i] = deltaTemp; SWAP;
			// if not last on stack
			// determine adjusted delta from new insert to one right below
			if (i > 0) { 
				deltaClock[i - 1]->time -= deltaTemp->time; SWAP;
			}
			// everyting below this point on the stack should already be the right times
			// done with insert
			break;
		}

		// adjust times based on new insert
		deltaTemp->time -= deltaClock[i - 1]->time; SWAP;
		deltaClock[i] = deltaClock[i - 1]; SWAP;
	}

	// added a delta clock
	numDeltaClock++; SWAP;


	SEM_SIGNAL(dcMutex);
	return 0;
}

int adjustDeltaClock(int numClocks) {
	if (!dcChange) {
		return 0;
	}

	if (numDeltaClock == 0) {
		clocks = 0;
		return 0;
	}

	if (dcMutex->state && numDeltaClock > 0) {
		// subtract clocks from first delta clock in stack
		int remainderClocks = numDeltaClock - deltaClock[numDeltaClock - 1]->time; 
		deltaClock[numDeltaClock - 1]->time -= numDeltaClock; 

		// while we keep satisfying delta clock time count 
		while (numDeltaClock > 0 && deltaClock[numDeltaClock - 1]->time <= 0) { 
			// free delta
			SEM_SIGNAL(deltaClock[numDeltaClock - 1]->sem);
			free(deltaClock[numDeltaClock - 1]); 
			deltaClock[numDeltaClock - 1] = NULL; 
			numDeltaClock--; 

			// use remaining clocks on lower deltas
			if (remainderClocks > 0 && numDeltaClock > 0) {
				int remainderClocksTemp = remainderClocks - deltaClock[numDeltaClock - 1]->time;
				deltaClock[numDeltaClock - 1]->time -= remainderClocks;
				remainderClocks = remainderClocksTemp;
			}
			SEM_SIGNAL(dcChange);
		}

		clocks = 0; 
	}

	return 0;
}


// ***********************************************************************
// test delta clock
//int P3_tdc(int argc, char* argv[])
//{
//	createTask( "DC Test",			// task name
//		dcMonitorTask,		// task
//		10,					// task priority
//		argc,					// task arguments
//		argv);
//
//	timeTaskID = createTask( "Time",		// task name
//		timeTask,	// task
//		10,			// task priority
//		argc,			// task arguments
//		argv);
//	return 0;
//} // end P3_tdc



// ***********************************************************************
// monitor the delta clock task
int dcMonitorTask(int argc, char* argv[])
{
	int i, flg;
	char buf[32];
	// create some test times for event[0-9]
	Semaphore* event[10];
	int ttime[10] = {
		90, 300, 50, 170, 340, 300, 50, 300, 40, 110	};

	for (i=0; i<10; i++)
	{
		sprintf(buf, "event[%d]", i);
		event[i] = createSemaphore(buf, BINARY, 0);
		insertDeltaClock(ttime[i], event[i]);
	}
	printDeltaClock();

	while (numDeltaClock > 0)
	{
		SEM_WAIT(dcChange);
		flg = 0;
		for (i=0; i<10; i++)
		{
			if (event[i]->state ==1)			{
					printf("\n  event[%d] signaled", i);
					event[i]->state = 0;
					flg = 1;
				}
		}
		if (flg) printDeltaClock();
	}
	printf("\nNo more events in Delta Clock");

	// kill dcMonitorTask
	tcb[timeTaskID].state = S_EXIT;
	return 0;
} // end dcMonitorTask


extern Semaphore* tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char* argv[])
{
	char svtime[64];						// ascii current time
	while (1)
	{
		SEM_WAIT(tics1sec);
		printf("\nTime = %s", myTime(svtime));
	}
	return 0;
} // end timeTask

void createVisitor(int max, Semaphore* sem)
{
	insertDeltaClock(rand() % max, sem);

	SEM_WAIT(sem);
}

void changeToTicketLine()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numOutsidePark--;        SWAP;
		myPark.numInPark++;             SWAP;
		myPark.numInTicketLine++;       SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void reduceTicketAmount()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numTicketsAvailable--;   SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void waitForTicket(Semaphore* visitorWait)
{
	createVisitor(30, visitorWait);

	SEM_WAIT(ticketMutex);           SWAP;
	{
		SEM_SIGNAL(visitorNeedTicket);		    SWAP;
		SEM_SIGNAL(parkAlertDriver);		SWAP;
		SEM_WAIT(parkTicket);		    SWAP;
		SEM_SIGNAL(visitorAcquireTicket);		    SWAP;

		reduceTicketAmount();
	}
	SEM_SIGNAL(ticketMutex);         SWAP;
}


void addVisitorToParkLine()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numOutsidePark++;        SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}



void changeToMuseumLine()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numInTicketLine--;       SWAP;
		myPark.numInMuseumLine++;       SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void enterMuseum()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numInMuseumLine--;       SWAP;
		myPark.numInMuseum++;           SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void waitForMuseum(Semaphore* visitorWait)
{
	createVisitor(30, visitorWait);

	SEM_WAIT(parkCapacityMuseum);           SWAP;
	{
		enterMuseum();
		createVisitor(30, visitorWait);
	}
	SEM_SIGNAL(parkCapacityMuseum);         SWAP;
}

void changeToTourCarLine()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numInMuseum--;			SWAP;
		myPark.numInCarLine++;          SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void enterTourCar()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numInCarLine--;          SWAP;
		myPark.numInCars++;             SWAP;
		myPark.numTicketsAvailable++;   SWAP;

		SEM_SIGNAL(parkCapacityTickets);
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void waitForTourCar(Semaphore* visitorWait)
{
	createVisitor(30, visitorWait);

	SEM_WAIT(parkRider);             SWAP;
	enterTourCar();                       SWAP;
	passPassengerWaitSem(visitorWait);  SWAP;
	SEM_WAIT(visitorWait);              SWAP;
}

void placeVisitorInCar(int myId)
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.cars[myId].passengers++; SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void removeVisitorFromCar(int myId)
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.cars[myId].passengers--; SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

Semaphore* getPassegnerWaitSem()
{
	Semaphore* waitSem;

	SEM_SIGNAL(driverNeedPassenger);          SWAP;
	SEM_WAIT(driverRiderResource);       SWAP;

	waitSem = gSemaphore;                   SWAP;

	SEM_SIGNAL(driverRiderResourceAcquired);  SWAP;


	return waitSem;
}

void passPassengerWaitSem(Semaphore *sem)
{

	SEM_WAIT(resourceMutex);          SWAP;
	SEM_SIGNAL(parkSeat);            SWAP;
	SEM_WAIT(driverNeedPassenger);      SWAP;

	gSemaphore = sem;                 SWAP;

	SEM_SIGNAL(driverRiderResource);        SWAP;
	SEM_WAIT(driverRiderResourceAcquired);       SWAP;
	SEM_SIGNAL(resourceMutex);        SWAP;

}

Semaphore* getDriverWaitSem(int carId)
{
	Semaphore* waitSem;

	SEM_SIGNAL(parkNeedDriver); SWAP;
	SEM_WAIT(driverResource); SWAP;

	waitSem = gSemaphore; SWAP;
	updateDriver(gDriverId, carId + 1); SWAP;

	SEM_SIGNAL(driverResourceAcquired); SWAP;

	return waitSem;
}


void passDriverWaitSem(Semaphore *sem, int driverId)
{
	SEM_WAIT(resourceMutex); SWAP;
	SEM_WAIT(parkNeedDriver); SWAP;

	gSemaphore = sem; SWAP;
	gDriverId = driverId; SWAP;

	SEM_SIGNAL(driverResource); SWAP;
	SEM_WAIT(driverResourceAcquired); SWAP;
	SEM_SIGNAL(resourceMutex); SWAP;
}

void updateDriver(int driverId, int position)
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.drivers[driverId] = position; SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void changeToGiftShopLine()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numInCars--;             SWAP;
		myPark.numInGiftLine++;         SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void enterGiftShop()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numInGiftLine--;         SWAP;
		myPark.numInGiftShop++;         SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

void waitForGiftShop(Semaphore* visitorWait)
{
	createVisitor(30, visitorWait);

	SEM_WAIT(parkCapacityGiftShop);           SWAP;
	enterGiftShop();
	createVisitor(30, visitorWait);
	SEM_SIGNAL(parkCapacityGiftShop);
}

void exitPark()
{
	SEM_WAIT(parkMutex);                SWAP;
	{
		myPark.numInPark--;             SWAP;
		myPark.numInGiftShop--;         SWAP;
		myPark.numExitedPark++;         SWAP;
	}
	SEM_SIGNAL(parkMutex);              SWAP;
}

















