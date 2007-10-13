/*
    CAN232 data logger
    Copyright (C) 2004 Attila Vass

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*										*/
/* This program records the incoming data from the CAN232 and stores in files.	*/
/* It uses a double buffer mechanism, while recording is done to one of the	*/
/* buffers, saving is done from the other. The 2 processes communicate via	*/
/* signals and exhange data from shared memory.					*/
/* The file name is generated based on seqnum/date/time.			*/
/* On top of logging the data from the CAN232, it inserts timestamps every	*/
/* second with 0xFFE id. ( tFFE0:%06d ) and characters pressed with id 0xFFF	*/
/* ( tFFF0:%02x ). These are useful when analyzing the traffic.			*/
/*										*/

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <termios.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>

//#define COMM_DEBUG    1
//#define COLL_DEBUG    1

#define	WORK_MEMORY_SIZE	(32*1024*1024)	// Up to 64 Meg each, since I have 512M available, 370M most of the time
#define	COMM_MEMORY_SIZE	256

#define	NEWLINE_CHAR	0x0D

#define	BAUD	B230400
#define	DEVICE	"/dev/ttyCAN"


long zeso = 0;
long curtime = 0;
unsigned char Maingo = 1, SamplingIsStopped = 0;

int SharedDataMemId[2];		// for Dual Buffer
char *SharedDataMemPtr[2];	// for Dual Buffer
int SharedCommMemId;
char *SharedCommMemPtr;
char Message[32];
unsigned int ActiveBank = 0, AB_Cntr = 0, ActiveFile = 0, DiskIO = 0;
struct shmid_ds SMMyInfo;
static pid_t PidMain = -1, PidCollector = -1, PidMine = -1;
int Port = -1;
time_t StartTime = 0;

void
gettime(void)
{
    struct timeval ltim;

    gettimeofday(&ltim, NULL);
    curtime = ((ltim.tv_sec - zeso) * 1000) + (ltim.tv_usec / 1000);
}

void
zerotime(void)			// This could take up to a second...
{
    struct timeval ltim;
    register int a;
    register long b = 0;


    while (1) {
	gettimeofday(&ltim, NULL);
	if (ltim.tv_usec < b) {
	    zeso = ltim.tv_sec;
	    return;
	}
	b = ltim.tv_usec;
    }
}

int
OpenAndConfigurePort(void)
{
    struct termios newio;

#ifdef COMM_DEBUG
    printf("\nOpening port on %s.", DEVICE);
    fflush(stdout);
#endif

    if ((Port = open(DEVICE, O_RDWR | O_NOCTTY)) < 0) {
	printf("\nError Opening Serialport ( %s ) : '%s'", DEVICE, strerror(errno));
	fflush(stdout);
	return (1);
    }
    memset(&newio, 0, sizeof(newio));	/* Clears termios struct  */
    newio.c_cflag = CS8 | CLOCAL | CREAD;
    newio.c_iflag = IGNPAR;
    newio.c_oflag = 0;
    newio.c_lflag = 0;
    newio.c_cc[VTIME] = 0;
    newio.c_cc[VMIN] = 0;	/* read min. one char at a time  */
    if (cfsetispeed(&newio, BAUD) == -1) {
	printf("Error setting serial input baud rate\n");
	close(Port);
	return (1);
    }
    if (cfsetospeed(&newio, BAUD) == -1) {
	printf("Error setting serial output baud rate\n");
	close(Port);
	return (1);
    }
    tcflush(Port, TCIFLUSH);
    if (tcsetattr(Port, TCSANOW, &newio) == -1) {
	printf("Error setting terminal attributes\n");
	close(Port);
	return (1);
    }

    return (0);
}


int
WriteToPort(char *ZTV)
{
    register a = 0, b = 0, str = 0;

    while (*(ZTV + a) != '\0')
	++a;

    printf("\nwriting to port %d. %d bytes.", Port, a);
    fflush(stdout);

    b = write(Port, ZTV, a);
//      tcflush(Port,TCOFLUSH);

#ifdef COMM_DEBUG
    printf("wrote %d.", b);
    fflush(stdout);
#endif

    return (b);

    while (*(ZTV + a) != '\0') {
	do {
	    b = 0;
	    b = write(Port, ZTV + a, 1);

#ifdef COMM_DEBUG
	    printf("wrote:%d", *(ZTV + a));
	    fflush(stdout);
#endif

	} while (!b);
	++a;
    }
    tcflush(Port, TCOFLUSH);
    return (a);
}

void
DumpBuffer(void)
{
    register int a = AB_Cntr, b;
    char *PB = SharedDataMemPtr[ActiveBank];

    printf("\n%d bytes in buffer :\n", a);
    fflush(stdout);

    for (b = 0; b < a; b++) {
	printf("%c", PB[b]);
	fflush(stdout);
    }

}

int
Poll(void)
{
    register int ret;
    char *PB = SharedDataMemPtr[ActiveBank];

#ifndef COLL_DEBUG
    if (SamplingIsStopped) {
	read(Port, PB + AB_Cntr, (WORK_MEMORY_SIZE - 1 - AB_Cntr));
	return (0);
    }

    if ((ret = read(Port, PB + AB_Cntr, (WORK_MEMORY_SIZE - 1 - AB_Cntr))) < 1)
	return (0);
    AB_Cntr += ret;

    if (AB_Cntr >= (WORK_MEMORY_SIZE - 10))	// Chg Bank, signal chg...
    {
	SharedCommMemPtr[1] = ActiveBank;
	SharedCommMemPtr[2] = 'N';
	if (++ActiveBank > 1)
	    ActiveBank = 0;
	*((unsigned int *) (SharedCommMemPtr + 16)) = AB_Cntr;
	kill(PidMain, SIGUSR1);
	AB_Cntr = 0;
    }
#else
    int a;

    if (SamplingIsStopped) {
	return (0);
    }

#if 1
    if ((rand() & 0x40) != 0x40)
	return;
    ret = rand() & 0x1F;
    if (ret = >(WORK_MEMORY_SIZE - AB_Cntr + 1))
	ret = (WORK_MEMORY_SIZE - AB_Cntr - 2);
    for (a = 0; a < ret; a++)
	PB[AB_Cntr + a] = (char) (rand() & 0x1F) + ' ';
    PB[AB_Cntr + a] = NEWLINE_CHAR;
    AB_Cntr += ret + 1;
#else
    ret = rand() & 0x3;
    for (a = 0; a < ret; a++)
	PB[AB_Cntr + a] = (char) (rand() & 0x1F) + ' ';
    AB_Cntr += ret;
#endif
    if (AB_Cntr >= (WORK_MEMORY_SIZE - 10))	// Chg Bank, signal chg...
    {
	SharedCommMemPtr[1] = ActiveBank;
	SharedCommMemPtr[2] = 'N';
	if (++ActiveBank > 1)
	    ActiveBank = 0;
	*((unsigned int *) (SharedCommMemPtr + 16)) = AB_Cntr;
	kill(PidMain, SIGUSR1);
	AB_Cntr = 0;
    }
#endif

    return (ret);
}



void
InsertTimeStamp(void)
{
    char *PB = SharedDataMemPtr[ActiveBank];
    char Message[32];
    int a, b, c, d;

    gettime();

#ifdef COMM_DEBUG
    printf("\ntimestamp : %06d", curtime);
    fflush(stdout);
#endif

    sprintf(Message, "tFFE0:%06d%c\0", curtime, NEWLINE_CHAR);	// My ID of 0xFFE with 0 byte indicator, but :%d is timestamp
    for (a = 0; Message[a] != '\0'; a++);
    for (d = 0, b = (AB_Cntr - 1); ((PB[b] != NEWLINE_CHAR) && (b > 0)); b--, d++);	// Let's insert it gracefully and not cut even 1 frame in half...

    if (AB_Cntr < (WORK_MEMORY_SIZE - a)) {
#if 1
	if ((b > 0) && (b < 32))	// Roll buffer to accomodate timestamp
	{
	    ++b;		// to move from 'NEWLINE_CHAR' to 't'
	    for (c = 0; c < d; c++)
		PB[b + c + d] = PB[b + c];
	}
	else
	    b = AB_Cntr;
	for (c = 0; c < a; c++)
	    PB[b + c] = Message[c];
	AB_Cntr += a;
#else
	for (a = 0; Message[a] != '\0'; a++)
	    PB[AB_Cntr++] = Message[a];
#endif
    }
    else {
	SharedCommMemPtr[1] = ActiveBank;	// sorry, not enough space, we need to cut...
	SharedCommMemPtr[2] = 'N';
	if (++ActiveBank > 1)
	    ActiveBank = 0;
	*((unsigned int *) (SharedCommMemPtr + 16)) = AB_Cntr;
	kill(PidMain, SIGUSR1);
	AB_Cntr = 0;
	PB = SharedDataMemPtr[ActiveBank];
	for (a = 0; Message[a] != '\0'; a++)
	    PB[AB_Cntr++] = Message[a];
    }
}


void
InsertCharacter(char Kar)
{
    char *PB = SharedDataMemPtr[ActiveBank];
    char Message[16];
    int a, b, c, d;

#ifdef COMM_DEBUG
    printf("\nkeystamp : %02x", Kar);
    fflush(stdout);
#endif

    sprintf(Message, "tFFF0:%02x%c\0", Kar, NEWLINE_CHAR);	// My ID of 0xFFF with 0 byte indicator, but :%c is info
    for (a = 0; Message[a] != '\0'; a++);
    for (d = 0, b = AB_Cntr; ((PB[b] != NEWLINE_CHAR) && (b > 0)); b--, d++);	// Let's insert it gracefully and not cut even 1 frame in half...

    if (AB_Cntr < (WORK_MEMORY_SIZE - a)) {
#if 1
	if ((b > 0) && (b < 32))	// Roll buffer to accomodate timestamp
	{
	    ++b;		// to move from 'NEWLINE_CHAR' to 't'
	    for (c = 0; c < d; c++)
		PB[b + c + d] = PB[b + c];
	}
	else
	    b = AB_Cntr;
	for (c = 0; c < a; c++)
	    PB[b + c] = Message[c];
	AB_Cntr += d;
#else
	for (a = 0; Message[a] != '\0'; a++)
	    PB[AB_Cntr++] = Message[a];
#endif
    }
    else {
	SharedCommMemPtr[1] = ActiveBank;
	SharedCommMemPtr[2] = 'N';
	if (++ActiveBank > 1)
	    ActiveBank = 0;
	*((unsigned int *) (SharedCommMemPtr + 16)) = AB_Cntr;
	kill(PidMain, SIGUSR1);
	AB_Cntr = 0;
	PB = SharedDataMemPtr[ActiveBank];
	for (a = 0; Message[a] != '\0'; a++)
	    PB[AB_Cntr++] = Message[a];
    }
}


void
GoodBye(void)
{
    // Anything else need to be done ?
}

void
SaveMemoryToDisk(int AB, unsigned int ThisMany)
{
    char *PB = SharedDataMemPtr[AB];
    int a;
    time_t t0;
    struct tm *ct;
    FILE *fp = NULL;
    char FileName[32];

    DiskIO = 1;

    t0 = time(0);
    ct = localtime(&t0);
    sprintf(FileName, "%03d_%02d.%02d_%02d.%02d.%02d", ActiveFile, (ct->tm_mon) + 1, ct->tm_mday,
	    ct->tm_hour, ct->tm_min, ct->tm_sec);
    ActiveFile++;

#ifndef COLL_DEBUG

    if ((fp = fopen(FileName, "w")) == NULL) {
	printf("\nCan not open '%s' for writing...", FileName);
	fflush(stdout);
	return;
    }

    if (fwrite(PB, sizeof(char), (size_t) ThisMany, fp) != ThisMany) {
	fseek(fp, 0L, SEEK_SET);
	a = 0;
	while (a < ThisMany) {
	    fputc((int) PB[a], fp);
	    ++a;
	}
    }
    fflush(fp);
    fclose(fp);

#else

    printf("\nSimulated writing %d bytes to '%s'", ThisMany, FileName);
    fflush(stdout);

#endif

    DiskIO = 0;

}

void
SigCatch(int sig)
{
    unsigned int a;

    switch (sig) {
    case SIGBUS:
    case SIGSEGV:
	printf("\nSYSTEM TERMINATION for %d...", PidMine);
	fflush(stdout);
	Maingo = 0;
	GoodBye();
	exit(1);
    case SIGQUIT:

	printf("\nSIGQUIT detected on %d. ( %d )", PidMine, PidCollector);
	fflush(stdout);

    case SIGINT:
    case SIGHUP:
    case SIGTERM:		// Ctrl+C  only on MAIN
#ifdef COMM_DEBUG
	printf("\nCTRL-C detected on %d.", PidMine);
	fflush(stdout);
#endif
	if (PidMine == PidMain) {

#ifdef COMM_DEBUG
	    printf("\nMain Handling it...", PidMine);
	    fflush(stdout);
#endif
	    SharedCommMemPtr[0] = (unsigned char) 254;	// Special Signal to quit
	    kill(PidCollector, SIGUSR1);
	}

//                      DumpBuffer();
	break;
    case SIGUSR1:
	if (PidMine == PidMain) {
#ifdef COMM_DEBUG
	    printf("\n Main received info, bank %d. is ready to be dumped ('%c')...",
		   SharedCommMemPtr[1], SharedCommMemPtr[2]);
	    fflush(stdout);
#endif
	    a = *((unsigned int *) (SharedCommMemPtr + 16));
	    if (DiskIO)
		sleep(4);	// Yes, yes...
	    SaveMemoryToDisk((int) (SharedCommMemPtr[1]), a);
	    if (SharedCommMemPtr[2] == 'Q')	// Quit operation...
	    {
		Maingo = 0;
		a = *((unsigned int *) (SharedCommMemPtr + 32));
		printf("\n\nSampling was for %d seconds... ( %2.2f minutes )", a,
		       ((float) a / 60.0f));
		fflush(stdout);
		printf("\nPlease press 'Enter'...");
		fflush(stdout);
	    }
	}
	else {
	    switch ((unsigned char) SharedCommMemPtr[0]) {
	    case 254:
#ifdef COMM_DEBUG
		printf("= > QUIT request");
		fflush(stdout);
#endif
		Maingo = 0;
		break;
	    case 253:
		if (++SamplingIsStopped > 1)
		    SamplingIsStopped = 0;
		if ((!SamplingIsStopped) && (StartTime == 0)) {
		    StartTime = time(NULL);
		    printf("\nCollector STARTED sampling...");
		    fflush(stdout);
		}
		else {
		    printf("\nCollector '%s' sampling...",
			   ((SamplingIsStopped) ? ("SUSPENDED") : ("RESUMED")));
		    fflush(stdout);
		}
		break;
	    case 252:

/*

Engine :	7E080213B00000000000

response came 55 message later as : 7E880253

Cruise Ctrl :	7E080213800000000000

response came 55 message later as : 7E880253

HV :		7E280213B00000000000

response came 54 message later as : 7EA80253

Batt :		7E380213800000000000

response came 59 message later as : 7EB80253

for 7E2 :

t7EA80253000000000000
after sending B0, this is
t7EA8045301A799000000

No fault response is 0253000000000000


no response for 7E1/7E4
*/


		printf("\nMessage transmit request...");
		fflush(stdout);

		InsertCharacter('d');
//                                              sprintf(Message,"t7E080213800000000000\015");
//                                              sprintf(Message,"t7E080213800000000000\015");
//                                              sprintf(Message,"t7E280213B00000000000\015");
		sprintf(Message, "t7E380213800000000000\015");
		WriteToPort(Message);
		break;
	    case 251:
		printf("\nMessage transmit more request...");
		fflush(stdout);
		InsertCharacter('m');
//                                              sprintf(Message,"t7E083000000000000000\015");
//                                              sprintf(Message,"t7E283000000000000000\015");
		sprintf(Message, "t7E383000000000000000\015");
		WriteToPort(Message);
		break;
	    default:
#ifdef COMM_DEBUG
		printf("\n Collector received info, character %c is to be inserted...",
		       SharedCommMemPtr[0]);
		fflush(stdout);
#endif
		InsertCharacter(SharedCommMemPtr[0]);
		break;
	    }
	}
	signal(SIGUSR1, SigCatch);
	break;
    case SIGALRM:
	signal(SIGALRM, SigCatch);
	alarm(1);
	if (!SamplingIsStopped)
	    InsertTimeStamp();
	break;
    }
}


int
GetSharedMemIds()
{
    register key_t SMKey;

    switch (sizeof(key_t)) {
    case 0:
	SMKey = (key_t) 0;
	break;
    case 1:
	SMKey = (key_t) 0x42;
	break;
    case 2:
	SMKey = (key_t) 0x4268;
	break;
    case 3:
	SMKey = (key_t) 0x426817;
	break;
    default:
	SMKey = (key_t) 0x42681712;
	break;
    }

    if ((SharedDataMemId[0] = shmget(SMKey, (size_t) WORK_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
	SMKey += 0xFF;
	if ((SharedDataMemId[0] = shmget(SMKey, (size_t) WORK_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
	    SMKey += 0xFF;
	    if ((SharedDataMemId[0] =
		 shmget(SMKey, (size_t) WORK_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
		return (-1);
	    }
	}
    }

    switch (sizeof(key_t)) {
    case 0:
	SMKey = (key_t) 0;
	break;
    case 1:
	SMKey = (key_t) 0x17;
	break;
    case 2:
	SMKey = (key_t) 0x1712;
	break;
    case 3:
	SMKey = (key_t) 0x171268;
	break;
    default:
	SMKey = (key_t) 0x17126842;
	break;
    }

    if ((SharedDataMemId[1] = shmget(SMKey, (size_t) WORK_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
	SMKey += 0xFF;
	if ((SharedDataMemId[1] = shmget(SMKey, (size_t) WORK_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
	    SMKey += 0xFF;
	    if ((SharedDataMemId[1] =
		 shmget(SMKey, (size_t) WORK_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
		return (-1);
	    }
	}
    }

    switch (sizeof(key_t)) {
    case 0:
	SMKey = (key_t) 0;
	break;
    case 1:
	SMKey = (key_t) 0x68;
	break;
    case 2:
	SMKey = (key_t) 0x6812;
	break;
    case 3:
	SMKey = (key_t) 0x681217;
	break;
    default:
	SMKey = (key_t) 0x68121742;
	break;
    }

    if ((SharedCommMemId = shmget(SMKey, (size_t) COMM_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
	SMKey += 0xFF;
	if ((SharedCommMemId = shmget(SMKey, (size_t) COMM_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
	    SMKey += 0xFF;
	    if ((SharedCommMemId =
		 shmget(SMKey, (size_t) COMM_MEMORY_SIZE, 0666 | IPC_CREAT)) == -1) {
		return (-1);
	    }
	}
    }

    return (0);
}


void
SetUpSignals()
{
    signal(SIGINT, SigCatch);
    signal(SIGHUP, SigCatch);
    signal(SIGTERM, SigCatch);
    signal(SIGBUS, SigCatch);
    signal(SIGSEGV, SigCatch);
    signal(SIGQUIT, SigCatch);
    signal(SIGUSR1, SigCatch);
}


int
RunMain(void)
{
    struct termios oldT, newT;
    int a, flag;

    PidMine = PidMain;

    printf("\n---Running Main Process with %d  (%d,%d)", PidMine, PidMain, PidCollector);
    fflush(stdout);

    if ((SharedDataMemPtr[0] = (char *) shmat(SharedDataMemId[0], 0, 0)) == NULL) {
	printf("\n%d : Can not attach Shared Memory Data_0...", PidMine);
	fflush(stdout);
	return (0);
    }
    if ((SharedDataMemPtr[1] = (char *) shmat(SharedDataMemId[1], 0, 0)) == NULL) {
	printf("\n%d : Can not attach Shared Memory Data_1...", PidMine);
	fflush(stdout);
	return (0);
    }
    if ((SharedCommMemPtr = (char *) shmat(SharedCommMemId, 0, 0)) == NULL) {
	printf("\n%d : Can not attach Shared Memory Comm...", PidMine);
	fflush(stdout);
	return (0);
    }

    SetUpSignals();

    ioctl(0, TCGETS, &oldT);
    ioctl(0, TCGETS, &newT);
    newT.c_lflag &= ~ECHO;
    newT.c_lflag &= ~ICANON;
    ioctl(0, TCSETS, &newT);

    printf("\n\t'z'   is to toggle sampling...");
    printf("\n\t'Esc' is to quit...");
    fflush(stdout);
    printf("\n\tOther keys will be inserted in datastream as 0xFFF message ID...\n");
    printf("\n### WAIT FOR COLLECTOR, PLEASE!!! ###");
    fflush(stdout);
    while (Maingo) {
	a = getchar();

//printf("\n%d = '%c' %d",PidMine,a,a);fflush(stdout);

	switch (a) {
	case 27:
	    SharedCommMemPtr[0] = (unsigned char) 254;	// Special Signal to quit
	    kill(PidCollector, SIGUSR1);
	    break;
	case 'z':
	case 'Z':
	    SharedCommMemPtr[0] = (unsigned char) 253;	// Special Signal to suspend/restart
	    kill(PidCollector, SIGUSR1);
	    break;
	case 'd':
	    SharedCommMemPtr[0] = (unsigned char) 252;	// Special Signal to send
	    kill(PidCollector, SIGUSR1);
	    break;
	case 'm':
	    SharedCommMemPtr[0] = (unsigned char) 251;	// Special Signal to send
	    kill(PidCollector, SIGUSR1);
	    break;
	default:
	    SharedCommMemPtr[0] = (unsigned char) a;
	    kill(PidCollector, SIGUSR1);
	    break;
	}
    }

    printf("\n---Main Quit Gracefully ...\n");
    fflush(stdout);

    ioctl(0, TCSETS, &oldT);
    shmdt(SharedDataMemPtr[0]);
    shmdt(SharedDataMemPtr[1]);
    shmdt(SharedCommMemPtr);

    return (1);
}

void
SignalDummyQuit()
{
    SharedCommMemPtr[1] = ActiveBank;
    SharedCommMemPtr[2] = 'Q';
    *((unsigned int *) (SharedCommMemPtr + 16)) = 0;
    *((unsigned int *) (SharedCommMemPtr + 32)) = 0;
    kill(PidMain, SIGUSR1);
}

int
RunCollector(void)
{
    int to, a;
    char *PB;
    time_t EndTime;
    unsigned int timdi = 0;


    if ((PidMine = getpid()) == -1) {
	printf("\nERROR : Can NOT obtain Collector ID on process...\n");
	fflush(stdout);
	SignalDummyQuit();
	return (1);
    }

//      setpriority(PRIO_PROCESS,PidMine,-9);

    if ((SharedDataMemPtr[0] = (char *) shmat(SharedDataMemId[0], 0, 0)) == NULL) {
	printf("\n%d : Can not attach Shared Memory Data_0...", PidMine);
	fflush(stdout);
	SignalDummyQuit();
	return (0);
    }
    if ((SharedDataMemPtr[1] = (char *) shmat(SharedDataMemId[1], 0, 0)) == NULL) {
	printf("\n%d : Can not attach Shared Memory Data_1...", PidMine);
	fflush(stdout);
	SignalDummyQuit();
	shmdt(SharedDataMemPtr[0]);
	return (0);
    }
    if ((SharedCommMemPtr = (char *) shmat(SharedCommMemId, 0, 0)) == NULL) {
	printf("\n%d : Can not attach Shared Memory Comm...", PidMine);
	fflush(stdout);
	SignalDummyQuit();
	shmdt(SharedDataMemPtr[0]);
	shmdt(SharedDataMemPtr[1]);
	return (0);
    }

    SetUpSignals();

    printf("\n---Running Collector with %d  (%d,%d)", PidMine, PidMain, PidCollector);
    fflush(stdout);

#ifndef COLL_DEBUG

    if (OpenAndConfigurePort()) {
	SignalDummyQuit();
	shmdt(SharedDataMemPtr[0]);
	shmdt(SharedDataMemPtr[1]);
	shmdt(SharedCommMemPtr);
	return (0);
    }

    sprintf(Message, "\015\015\015");
    WriteToPort(Message);

    for (a = 0; a < 10000; a++)
	to = a;

    sprintf(Message, "V\015");	// get version
    WriteToPort(Message);

    for (a = 0; a < 10000; a++)
	to = a;
    sprintf(Message, "N\015");	// get Serial
    WriteToPort(Message);

    for (a = 0; a < 10000; a++)
	to = a;
    sprintf(Message, "S6\015");	// CAN with 500Kbps S0-10 S1-20 S2-50 S3-100 S4-125 S5-250 S7-800  S8-1M
    WriteToPort(Message);

    sprintf(Message, "O\015");	// Open the CAN channel
    WriteToPort(Message);

#endif

    signal(SIGALRM, SigCatch);
    alarm(1);

    if (!SamplingIsStopped) {
	StartTime = time(NULL);
	printf("\nSampling is RUNNING...");
    }
    else {
	printf("\nSampling is SUSPENDED, activate with 'z' !");
    }
    fflush(stdout);

    SharedCommMemPtr[1] = ActiveBank;

    while (Maingo) {
	if (Poll()) {
//                      sleep(1);
//                      printf("\n%d : '%c'",PidMine,*(SharedMemoryPtr));fflush(stdout);
	}
    }

#ifndef COLL_DEBUG
    sprintf(Message, "C\015");	// Close the CAN channel
    WriteToPort(Message);
    if (Port >= 0)
	close(Port);
#endif

    printf("\n---Collector Quit Gracefully [%d:%d]...", ActiveBank, AB_Cntr);
    fflush(stdout);

    EndTime = time(NULL);
    timdi = (unsigned int) difftime(EndTime, StartTime);
    SharedCommMemPtr[1] = ActiveBank;
    SharedCommMemPtr[2] = 'Q';
    *((unsigned int *) (SharedCommMemPtr + 16)) = AB_Cntr;
    *((unsigned int *) (SharedCommMemPtr + 32)) = timdi;
    kill(PidMain, SIGUSR1);
    AB_Cntr = 0;

//printf("\n\nSampling was for %d seconds... ( %2.2f minutes )",timdi,((float)timdi/60.0f));fflush(stdout);

    shmdt(SharedDataMemPtr[0]);
    shmdt(SharedDataMemPtr[1]);
    shmdt(SharedCommMemPtr);

    return (1);
}


int
main(int argc, char **argv)
{

    if (argc > 1) {
	SamplingIsStopped = 1;
    }

    if (GetSharedMemIds() == -1) {
	printf("\nERROR for SharedMemoryKey...\n");
	fflush(stdout);
	return (1);
    }

#ifdef COMM_DEBUG
    printf("\nSharedMemoryIDs : %d %d %d", SharedDataMemId[0], SharedDataMemId[1], SharedCommMemId);
    fflush(stdout);
#endif

    if (shmctl(SharedDataMemId[0], IPC_STAT, &SMMyInfo) == -1) {
	printf("\nError For SM-Info for DataBank0.");
	fflush(stdout);
	shmctl(SharedDataMemId[0], IPC_RMID, 0);
	shmctl(SharedDataMemId[1], IPC_RMID, 0);
	shmctl(SharedCommMemId, IPC_RMID, 0);
	return (1);
    }

    if (shmctl(SharedDataMemId[1], IPC_STAT, &SMMyInfo) == -1) {
	printf("\nError For SM-Info for DataBank1.");
	fflush(stdout);
	shmctl(SharedDataMemId[0], IPC_RMID, 0);
	shmctl(SharedDataMemId[1], IPC_RMID, 0);
	shmctl(SharedCommMemId, IPC_RMID, 0);
	return (1);
    }

    if (shmctl(SharedCommMemId, IPC_STAT, &SMMyInfo) == -1) {
	printf("\nError For SM-Info for CommBank");
	fflush(stdout);
	shmctl(SharedDataMemId[0], IPC_RMID, 0);
	shmctl(SharedDataMemId[1], IPC_RMID, 0);
	shmctl(SharedCommMemId, IPC_RMID, 0);
	return (1);
    }

    if ((PidMain = getpid()) == -1) {
	printf("\nERROR : Can NOT obtain information on process...\n");
	fflush(stdout);
	return (1);
    }
    printf("\nMainProcess: %d", PidMain);
    fflush(stdout);

#if COMM_DEBUG
    printf("\nCId : %d (%d) <%d>", SMMyInfo.shm_cpid, PidMain, SMMyInfo.shm_segsz);
    fflush(stdout);
#endif

    zerotime();
    switch ((PidCollector = fork())) {
    case -1:
	printf("\nERROR : Can NOT fork process...\n");
	fflush(stdout);
	shmctl(SharedDataMemId[0], IPC_RMID, 0);
	shmctl(SharedDataMemId[1], IPC_RMID, 0);
	shmctl(SharedCommMemId, IPC_RMID, 0);
	return (1);
    default:
	RunMain();
	shmctl(SharedDataMemId[0], IPC_RMID, 0);
	shmctl(SharedDataMemId[1], IPC_RMID, 0);
	shmctl(SharedCommMemId, IPC_RMID, 0);
	break;
    case 0:
	sleep(1);
	RunCollector();
	break;
    }
    GoodBye();
//printf("\nEND. (%d)\n",PidMine);fflush(stdout);
}
