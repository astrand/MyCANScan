/*
    CAN data visualizer for the Toyota Prius
    Main program

    Copyright 2004-2005 Attila Vass
    Copyright 2007 Peter Åstrand <astrand@lysator.liu.se>

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

/*

MPG -> L/100km         10 mpg = 16km/3.78l  =>  1.6km/G 1.6km/3.78l => 0.42328f

DEFAULT :
Working with ACR = 07202000   AMR = 004FDFEF
0x03B 0x120 0x348 0x34F 0x3C8 0x3C9 0x3CA 0x3CB 0x3CD 0x3CF 0x520 0x521 0x526 0x527 0x528 0x529
0x52C 0x540 0x553 0x554 0x56D 0x57F 0x591 0x5A4 0x5B2 0x5B6 0x5C8 0x5CC 0x5D4 0x5EC 0x5ED 0x5F8
0x7E8 0x7EA 0x7EB

Old Default :
Working with ACR = 07602000   AMR = 000FDFEF
0x03B 0x120 0x348 0x34F 0x3C8 0x3C9 0x3CA 0x3CB 0x3CD 0x3CF 0x520 0x521 0x526 0x527 0x528 0x529
0x52C 0x540 0x553 0x554 0x56D 0x57F 0x591 0x5A4 0x5B2 0x5B6 0x5C8 0x5CC 0x5D4 0x5EC 0x5ED 0x5F8
0x7E8 0x7EA 0x7EB

Working with ACR = 06002000   AMR = 016FDFEF
0x030 0x038 0x039 0x03A 0x03B 0x120 0x348 0x34F 0x3C8 0x3C9 0x3CA 0x3CB 0x3CD 0x3CF 0x520 0x521
0x526 0x527 0x528 0x529 0x52C 0x540 0x553 0x554 0x56D 0x57F 0x591 0x5A4 0x5B2 0x5B6 0x5C8 0x5CC
0x5D4 0x5EC 0x5ED 0x5F8

Working with ACR = 06002000   AMR = 416FDFEF
0x030 0x038 0x039 0x03A 0x03B 0x120 0x230 0x348 0x34F 0x3C8 0x3C9 0x3CA 0x3CB 0x3CD 0x3CF 0x520
0x521 0x526 0x527 0x528 0x529 0x52C 0x540 0x553 0x554 0x56D 0x57F 0x591 0x5A4 0x5B2 0x5B6 0x5C8
0x5CC 0x5D4 0x5EC 0x5ED 0x5F8

Working with ACR = 29000400   AMR = D6EFB3EF
0x020 0x022 0x023 0x025 0x030 0x038 0x039 0x03A 0x03B 0x03E 0x0B0 0x0B1 0x0B3 0x0B4 0x120 0x348
0x34F 0x3C8 0x3C9 0x3CA 0x3CB 0x3CD 0x3CF 0x423 0x520 0x521 0x526 0x527 0x528 0x529 0x52C 0x56D
0x57F 0x5A4 0x5B2 0x5B6 0x5C8 0x5CC 0x5EC 0x5ED 0x5F8

Working with ACR = 04000800   AMR = BBEFF1EF
0x020 0x022 0x023 0x025 0x030 0x038 0x039 0x03A 0x03B 0x03E 0x060 0x0B0 0x0B1 0x0B3 0x0B4 0x0C9
0x120 0x244 0x348 0x34F 0x3C8 0x3C9 0x3CA 0x3CB 0x3CD 0x3CF 0x423 0x4C1 0x4C3 0x4C6 0x4C7 0x4C8
0x4CE 0x520 0x521 0x526 0x527 0x528 0x529 0x52C 0x540 0x56D 0x57F 0x5A4 0x5B2 0x5B6 0x5C8 0x5CC
0x5EC 0x5ED 0x5F8


DIAG :
Working with ACR = 7960FC00   AMR = 000F016F	( M7960FC00 m000F016F )
 
0x3CB 0x7E0 0x7E2 0x7E3 0x7E8 0x7EA 0x7EB	( using 0x3CB - battery as heartbeat )

*/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <pthread.h>
#include "ui.h"


/**********************************/
/*   APPLICATION DEPENDENT DATA   */
/**********************************/

#define	STAT_FILE_NAME_SIM		"./PriusData_sim.txt"
#define FUEL_FILE_NAME_SIM		"./FuelData_sim.txt"
#define	STAT_FILE_NAME			"./PriusData.txt"
#define FUEL_FILE_NAME			"./FuelData.txt"

#ifndef NON_ZAURUS

#define	USE_VOICE_ANNOUNCEMENT		1
//#define       USE_KEYBOARD            1
#define	FORCE_PATH			"/home/zaurus/CAN"

#else // NON_ZAURUS

#define	USE_KEYBOARD		1
//      #define USE_VOICE_ANNOUNCEMENT          1
#define	FORCE_PATH			"./"

#endif // NON_ZAURUS

//#define       COMM_DEBUG              1
//#define       TRACE_IT                1
//#define       TRAFFIC_PROFILE         1
//#define       MORE_DATA               1

#define	USE_TOUCHSCREEN		1


#define	BAUD	B115200

#define	MISC_DATA_REFRESH_RATE		5
#define	FULLSCREEN_REFRESH_RATE		6       // every MISC_DATA_REFRESH_RATE refresh the whole screen...
#define	FIXED_FONT_COLOR		1
#define	SPDSCALER			((double)(16000.0))
#define	SPDSCALER_MILE			((double)(10000.0))

#define	VERSION_STRING	"v" PACKAGE_VERSION " by Attila Vass & Peter Astrand"

#define DEVICE "/dev/ttyCAN"

#ifndef	ABS
#define	ABS(a)	(((a)<0)?(-(a)):((a)))
#endif // ABS

#define	BKG_R	((unsigned char)0x00)
#define	BKG_G	((unsigned char)0x00)
#define	BKG_B	((unsigned char)0x00)

enum
{ TASK_INIT = 0, TASK_INFO, TASK_SCREENSAVER, TASK_RUNNING };


#include "images.h"
extern unsigned char *PriusPicture[];
extern unsigned char SideFrontDoor[];
extern unsigned char RearDoors[];
extern struct ImageBufferStructure Mode_EngineGreen_Color[];
extern unsigned char *Mode_EngineGreen_Picture[];
extern struct ImageBufferStructure Mode_EngineRed_Color[];
extern unsigned char *Mode_EngineRed_Picture[];
extern struct ImageBufferStructure Mode_Electric_Color[];
extern unsigned char *Mode_Electric_Picture[];
extern struct ImageBufferStructure Mode_ElectricCanc_Color[];
extern unsigned char *Mode_ElectricCanc_Picture[];

char *stat_file_name = STAT_FILE_NAME;
char *fuel_file_name = FUEL_FILE_NAME;
char *WorkData = NULL;
struct ImageBufferStructure *ImageBuffer = NULL;
char ProcessGo = 1, RealQuit = 0, ContSignal = 0, Sleeping = 0;
int Port = -1;
time_t StartTime = 0;
char *IB = NULL;
int IBS = 0;
int IBCP = 0;
int CRSignal = 0;
float DistanceTravelled = 0.0f, MeasurementKilometers = 0.0f, MeasuredKMPG =
    0.0f, MeasuredAverageSpeed = 0.0f;
float TripKilometers = 0.0f, TripGallonValue = 0.0f;
float TripMile = 0.0f, TripGal = 0.0f;
float MeasuredMinCurrent = 0.0f, MeasuredMaxCurrent = 0.0f, MeasuredMinW = 0.0f, MeasuredMaxW =
    0.0f;
float CurrentSpeed = 0, RefuelValue = 0.0f;
double AccSpd = 0.0f, AccRpm = 0.0f;
unsigned int StatusFlag = 0;
unsigned int AccSpdCntr = 0;
unsigned int SpeedAccumulated = 0, SpeedAccumulatedCntr = 0, ThrottleAccumulatedCntr = 0;
int Cat1Temp = 0, Cat2Temp = 0;
unsigned long ThrottleAccumulated = 0;
unsigned char NeedSynced = 1, SyncCntr = 0, RunningTask = 0;
unsigned char EVMode = 0, LEVMode = 0;  // Cause it to be normal at startup
unsigned char ICEPowered = 1, ICEPoweredCurrStage = 50;
unsigned char FSRCntr = 1;
unsigned char LatestRPM = 0;
unsigned char GasGauge = 0x7F;  // 11.9 Gal / 45 Liter ( 11.904762 Gal )
unsigned char GasGaugeExtraValue = 99, FirstGasReading = 1;
unsigned char BattMaxChargeCurrent = 0, BattMaxDisChargeCurrent = 0;
unsigned char DoorStatus = 0;
unsigned char PriusIsPowered = 1;
unsigned char ValueSwitch = 0;
unsigned char LightValue = 0;
unsigned char InstrumentsDimmed = 0;
unsigned char CollectCurrent = 0, TrafficCtr = 0, TrafficSubCtr = 0;
unsigned char MinMaxCurrentDisplay = 0, MinMaxkWDisplay = 0;
unsigned char DoorOpenCnt = 0, NoTrafficYet = 1, FirstTimeSOC = 1, FirstTimeGas = 1, VoiceMode =
    1, ForcePath = 0, SI_Measurements = 0, Simulation = 0;
unsigned char EngBlckTmpCnt = 1, EngBlckTmp = 0;
unsigned char TempValue = 0;    // Temperature
unsigned char CATReqCtr = 0;

unsigned int SOCValue = 0, SOCValueCurrStage = 0, LSOCValue = 0, PreGasValue = 0;       // 22  18  6   6  8
unsigned int BattTempValue = 0, BattVoltageValue = 0, FuelRead = 0;
unsigned int MaxCurrent = 0, MinCurrent = 0, MaxCurrentVoltage = 0, MinCurrentVoltage = 0;
#ifdef MORE_DATA
unsigned int Info_Brakes = 0, Info_Brakes_Cnt = 0;
#endif
int ScrSv_NumberOfTimes = 1;
int PreviousRPMValue = 0xFFFF;
unsigned int TimeElapsed = 0;
unsigned long CollCurr = 0, CollVolt = 0;
unsigned int CollCntr = 0, CollTime = 0;
int TouchedX = -1, TouchedY = -1;
unsigned int NofTrafficBytes = 0, SpeedCurrStage = 0, SpeedCurrStageCntr = 0;
float LastRegenkW = 0.0f;
unsigned char LastRegenTime = 0, Decel = 0, PrvSpd = 0, StrtSpd = 0, LastRegenSpdStart =
    0, LastRegenSpdEnd = 0, ScrTouched = 0;
char TouchedButton = -1;
#ifdef MORE_DATA
unsigned char LastRegenBrake = 0;
#endif
char LastRegenStopReason = 'U';
unsigned char NofTrafficBytesZero = 0;

#ifdef TRAFFIC_PROFILE
unsigned int NofInv = 0, NofNInv = 0;
#endif

char Version[4];
char Serial[5];
char Message[64];
unsigned long IC_MessageID;
unsigned char IC_Message[8];

#ifdef USE_KEYBOARD
struct termios oldT, newT;
#endif

#ifdef USE_TOUCHSCREEN
int touch_screen_fd = -1;
#endif

#ifdef TRACE_IT

#define	TRACE_BUFFER_LENGTH	1024
char TrBu[TRACE_BUFFER_LENGTH];
unsigned int TrBuPtr = 0;

#endif

#include "fonts.h"
extern char Font_Map[FONTMAP_HEIGHT][FONTMAP_WIDTH];
extern fontmetrics bignumbers[];
extern fontmetrics fonts[];

/*
SetUpCAN			A
UpdateSOC			B
CleanUp				C
UICopyDisplayBufferToScreen	D
UpdateCurrent			E
TransferFont			F
UpdateGG			G
UpdateSpeedComputations		H
SetUpMySignals			I
UpdateTemp			J
UpdateCatTemp			K
Poll				L
CreateMainWindow		M
GetMyStringLength		N
OpenAndConfigurePort		O
ProcessSigIo			P
MineChar			Q
UpdateRpm			R
PutMyString			S
ClearDisplayBuffer		T
MineUInt			U
UpdateDriveMode			V
WriteToPort			W
IntReXSigCatch			X
SetUpPicture			Y
FastPoll			<>
AnalyseHighCANMessages		-+
*/

unsigned char MaxCurrentValues[50][9];  // for every 2 km/h value record max generated current in Amps, also 40/45/50/55/60/65/70/75/80%

typedef struct ButtonStructure
{
    unsigned int X;
    unsigned int Y;
    unsigned char W;
    unsigned char H;
    char Title[32];
    unsigned char UserData;
} ButtonStructure;

void PutMyString(char *Text, int x, int y, int usebignums, int zoom);
int GetMyStringLength(char *Text, int usebignums, int zoom);
void IntReXSigCatch(int);
void PutExtraInfo(void);
void UpdateGG(void);
void ClearDisplayBuffer(void);
unsigned char MineChar(int);
void UpdateCatTemp(void);
unsigned int MineUInt(int Position);

#define	NUM_BUTTONS	16      // 320:5   430:125   540:245   540:365   ->  300:30   400:140  500:260  500:370
#define	BU_XST		320
#define	BU_YST		30
#define	BU_GP		10
#define	BU_W		100
#define	BU_H		100

ButtonStructure Buttons[NUM_BUTTONS];
char *KP = "1234567890.C";
char ValueChars[8];

void
DefineButtons(void)
{
    int a, x, y;

    a = 0;
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 3; x++) {
            Buttons[a].X = BU_XST + (x * BU_W) + (x * BU_GP);
            Buttons[a].Y = BU_YST + (y * BU_H) + (y * BU_GP);
            Buttons[a].W = BU_W;
            Buttons[a].H = BU_H;
            sprintf(Buttons[a].Title, "%c", KP[a]);
            Buttons[a].UserData = a;
            ++a;
        }
    }

    Buttons[a].X = 20;
    Buttons[a].Y = BU_YST + 20;
    Buttons[a].W = 200;
    Buttons[a].H = 60;
    sprintf(Buttons[a].Title, "REFUEL");
    Buttons[a].UserData = a;
    ++a;

    Buttons[a].X = 20;
    Buttons[a].Y = 380;
    Buttons[a].W = 200;
    Buttons[a].H = 80;
    sprintf(Buttons[a].Title, "RETURN");
    Buttons[a].UserData = a;
    ++a;

    Buttons[a].X = 20;
    Buttons[a].Y = 170;
    Buttons[a].W = 200;
    Buttons[a].H = 60;
    sprintf(Buttons[a].Title, "FULL TANK");
    Buttons[a].UserData = a;
    ++a;

    Buttons[a].X = 20;
    Buttons[a].Y = 290;
    Buttons[a].W = 200;
    Buttons[a].H = 70;
    sprintf(Buttons[a].Title, "Voice: %s", ((VoiceMode) ? ("Yes") : ("No")));
    Buttons[a].UserData = a;
    ++a;
}

void
DrawRectangle(int x, int y, int w, int h, int fill)
{
    int a, b, c;

    if (fill == 0) {
        for (b = y; b < (y + h); b++) {
            c = b * WIDTH + x;  // Left line
            ImageBuffer[c].R = 240;
            ImageBuffer[c].G = 240;
            ImageBuffer[c].B = 255;
            c = b * WIDTH + x + 1;      // Left line
            ImageBuffer[c].R = 220;
            ImageBuffer[c].G = 220;
            ImageBuffer[c].B = 245;
            c = b * WIDTH + x + w - 2;  // Right Line
            ImageBuffer[c].R = 200;
            ImageBuffer[c].G = 200;
            ImageBuffer[c].B = 230;
            c = b * WIDTH + x + w - 1;  // Right Line
            ImageBuffer[c].R = 180;
            ImageBuffer[c].G = 180;
            ImageBuffer[c].B = 220;
        }
        for (b = x; b < (x + w - 1); b++) {
            c = y * WIDTH + b;  // Top line
            ImageBuffer[c].R = 240;
            ImageBuffer[c].G = 240;
            ImageBuffer[c].B = 255;
            c = (y + 1) * WIDTH + b;    // Top line
            ImageBuffer[c].R = 220;
            ImageBuffer[c].G = 220;
            ImageBuffer[c].B = 245;
            c = (y + h) * WIDTH + b;    // Bottom line
            ImageBuffer[c].R = 200;
            ImageBuffer[c].G = 200;
            ImageBuffer[c].B = 230;
            c = (y + h + 1) * WIDTH + b;        // Bottom line
            ImageBuffer[c].R = 180;
            ImageBuffer[c].G = 180;
            ImageBuffer[c].B = 220;
        }
    }
    else {
        for (b = y; b < (y + h); b++) {
            c = b * WIDTH + x;
            for (a = x; a < (x + w); a++) {
                ImageBuffer[c].R = 240;
                ImageBuffer[c].G = 240;
                ImageBuffer[c].B = 255;
                ++c;
            }
        }
    }
}

void
DrawInfoScreen(void)
{
    int a, c, d, x, y, f;

    ClearDisplayBuffer();

    for (a = 0; a < NUM_BUTTONS; a++) {
        if (TouchedButton == a)
            f = 1;
        else
            f = 0;
        DrawRectangle(Buttons[a].X, Buttons[a].Y, Buttons[a].W, Buttons[a].H, f);
        sprintf(Message, "%s", Buttons[a].Title);
        c = GetMyStringLength(Message, 1, 3);
        d = 38;
        x = Buttons[a].X + (Buttons[a].W >> 1) - (c >> 1);
        y = Buttons[a].Y + (Buttons[a].H >> 1) - (d >> 1);
        PutMyString(Message, x, y, 1, 3);
    }

    if (SI_Measurements)
        sprintf(Message, "Ltr: %s", ValueChars);
    else
        sprintf(Message, "Gal: %s", ValueChars);
    PutMyString(Message, 20, 120, 1, 3);

    if (SI_Measurements)
        sprintf(Message, "km-tank: %3.1f", TripKilometers);
    else
        sprintf(Message, "mil-tank: %3.1f", (TripKilometers * 0.625f));
    PutMyString(Message, 30, 240, 1, 2);

    UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
}

void
EvaluateTouch(void)             // -20:-20 for touch-to-screen translation
{
    int a;

//printf("\n%d:%d",TouchedX,TouchedY);fflush(stdout);

    if ((TouchedX += 20) > 640)
        TouchedX = 640;
    if ((TouchedY += 10) > 480)
        TouchedY = 480;
    for (a = 0; a < NUM_BUTTONS; a++) {
        if ((TouchedX >= Buttons[a].X) && (TouchedX <= (Buttons[a].X + Buttons[a].W))
            && (TouchedY >= Buttons[a].Y) && (TouchedY <= (Buttons[a].Y + Buttons[a].H))) {
            TouchedButton = a;
            return;
        }
    }
}

void
ResetValues(void)
{
    RunningTask = TASK_INIT;
    TouchedX = -1;
    TouchedY = -1;
    ScrTouched = 0;
    RefuelValue = 0.0f;
    TouchedButton = -1;
    TripKilometers = 0.0f;
    TripGallonValue = 0.0f;
    TripMile = 0.0f;
    TripGal = 0.0f;
    DistanceTravelled = 0.0f;
    MeasurementKilometers = 0.0f;
    MeasuredKMPG = 0.0f;
    MeasuredAverageSpeed = 0.0f;
    MeasuredMinCurrent = 0.0f;
    MeasuredMaxCurrent = 0.0f;
    MeasuredMinW = 0.0f;
    MeasuredMaxW = 0.0f;
    StatusFlag = 0;
    AccSpd = 0.0f;
    AccRpm = 0.0f;
    AccSpdCntr = 0;
    SpeedAccumulated = 0;
    SpeedAccumulatedCntr = 0;
    ThrottleAccumulatedCntr = 0;
    ThrottleAccumulated = 0;
    SyncCntr = 0;
    EVMode = 0;
    LEVMode = 0;
    ICEPowered = 1;
    ICEPoweredCurrStage = 50;
    PriusIsPowered = 1;
    FSRCntr = 1;
    LatestRPM = 0;
    FirstGasReading = 1;
    MaxCurrent = 0;
    MinCurrent = 0;
    MinMaxCurrentDisplay = 0;
    MinMaxkWDisplay = 0;
    DoorStatus = 0;
    SOCValue = 0;
    LSOCValue = 0;
    SOCValueCurrStage = 0;
    PreGasValue = 0;
    BattTempValue = 0;
    BattVoltageValue = 0;
    FuelRead = 0;
    BattMaxChargeCurrent = 0;
    BattMaxDisChargeCurrent = 0;
    TempValue = 0;
    EngBlckTmpCnt = 1;
    EngBlckTmp = 0;
    Cat1Temp = 30;
    Cat2Temp = 30;
    CATReqCtr = 0;
    NeedSynced = 1;
    ScrSv_NumberOfTimes = 1;
    PreviousRPMValue = 0xFFFF;
    ValueSwitch = 0;
    LightValue = 0;
    InstrumentsDimmed = 0;
    CollCurr = 0;
    CollVolt = 0;
    CollCntr = 0;
    CollectCurrent = 0;
    TrafficCtr = 0;
    TrafficSubCtr = 0;
    Decel = 0;
    PrvSpd = 0;
    SpeedCurrStage = 0;
    SpeedCurrStageCntr = 0;
    StrtSpd = 0;
    LastRegenSpdStart = 0;
    LastRegenSpdEnd = 0;
    LastRegenkW = 0.0f;
    LastRegenTime = 0;
    LastRegenStopReason = 'U';
    NofTrafficBytesZero = 0;
#ifdef MORE_DATA
    LastRegenBrake = 0;
    Info_Brakes = 0;
    Info_Brakes_Cnt = 0;
#endif
    DoorOpenCnt = 0;
    FirstTimeSOC = 1;
    FirstTimeGas = 1;
    NoTrafficYet = 1;
}

int
Poll(void)
{
    register int a, b, c, d, ret, go = 1;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'L';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    if (IBCP >= IBS) {
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'l';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
        TrBu[TrBuPtr] = '1';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return (0);
    }
    if ((ret = read(Port, IB + IBCP, (IBS - IBCP))) < 1) {
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'l';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
        TrBu[TrBuPtr] = '2';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return (0);
    }
    IBCP += ret;

#ifdef COMM_DEBUG
    printf("STS");
    fflush(stdout);
#endif

    while (go) {
        a = IBCP;
        for (b = 0; b < a; b++) {
            if ((IB[b] == 13) || (IB[b] == 7))  // Terminator
            {

#ifdef COMM_DEBUG
                printf("\nT = %d/%d '%c': ", b, IBCP, IB[0]);
                fflush(stdout);
#endif

                switch (IB[0])  // 0 is OK, because we tidy up the buffer as we go...
                {
                case 'V':      // version
                    for (c = 0; c < 4; c++)
                        Version[c] = IB[c + 1];
#ifdef COMM_DEBUG
                    printf("Version Recognized");
                    fflush(stdout);
#endif
                    break;
                case 'N':
                    for (c = 0; c < 4; c++)
                        Serial[c] = IB[c + 1];
                    break;
                case 'F':      // Status
                    if (IB[1] >= 'A')
                        StatusFlag = (IB[1] - 'A') + 10;
                    else
                        StatusFlag = (IB[1] - '0');
                    StatusFlag <<= 4;
                    if (IB[2] >= 'A')
                        StatusFlag += (IB[2] - 'A') + 10;
                    else
                        StatusFlag += (IB[2] - '0');
                    CRSignal = 1;
                    break;
                case 'z':      // transmission ack
                case 'Z':
#ifdef COMM_DEBUG
                    printf("Transmission Success");
                    fflush(stdout);
#endif
                    CRSignal = 1;
                    break;
                case 't':      // 11 bit frame ( 3 byte ID )
                    IC_MessageID = 0;
                    for (c = 1; c < 4; c++) {
                        IC_MessageID <<= 4;
                        if (IB[c] >= 'A')
                            IC_MessageID += (IB[c] - 'A') + 10;
                        else
                            IC_MessageID += (IB[c] - '0');
                    }
                    if ((IB[4] >= '0') && (IB[4] <= '8')) {
//                                                      MessageLength=(IB[4]-'0');
//                                                      AnalyseFrame(5);
                    }
                    else {
#ifdef COMM_DEBUG
                        printf("\n###ERROR : Message length is %d:'%c'", IB[4], IB[4]);
                        fflush(stdout);
#endif
                    }
                    CRSignal = 1;
                    break;
#if 0
                case 'T':      // 29 bit frame ( 8 byte ID )
                    IC_MessageID = 0;
                    for (c = 1; c < 8; c++) {
                        IC_MessageID <<= 4;
                        if (IB[c] >= 'A')
                            IC_MessageID += (IB[c] - 'A') + 10;
                        else
                            IC_MessageID += (IB[c] - '0');
                    }

                    if ((IB[8] >= '0') && (IB[8] <= '8')) {
//                                                      MessageLength=(IB[8]-'0');
//                                                      AnalyseFrame(9);
                    }
                    else {
#ifdef COMM_DEBUG
                        printf("\n###ERROR : Message length is %d:'%c'", IB[8], IB[8]);
                        fflush(stdout);
#endif
                    }
                    CRSignal = 1;
                    break;
#endif
                case 13:
                    CRSignal = 1;
                    break;
                case 7:
#ifdef COMM_DEBUG
                    printf("\n*****  ERROR BELL RECEIVED...");
                    fflush(stdout);
#endif
                    CRSignal = 1;
                    break;
                default:
#ifdef COMM_DEBUG
                    printf("Header Byte : '%c' ", IB[0]);
                    fflush(stdout);
                    for (c = 0; c < b; c++)
                        printf("'%c'", IB[c]);
                    fflush(stdout);
#endif
                    break;
                }
                // Tidy up...
                if (IBCP == (b + 1))    // This is the only message, so no copy req.
                    IBCP = 0;
                else {
                    a = b + 1;  // from here
                    c = IBCP - b;       // this many
#ifdef COMM_DEBUG
                    printf(" => %d/%d : ", a, c);
                    fflush(stdout);
#endif
                    for (d = 0; d < c; d++) {
                        IB[d] = IB[d + a];
                    }
                    IBCP -= (b + 1);
#ifdef COMM_DEBUG
                    printf(" -> %d", IBCP);
                    fflush(stdout);
#endif
                }
                b = a + 2;      // quit from loop in 'C' fashion...
            }
        }
        if (b == a)
            go = 0;
    }

#ifdef COMM_DEBUG
    printf("RET");
    fflush(stdout);
#endif

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'l';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    return (ret);
}

void
RunTaskInfoMain(void)
{
    struct timespec rqst, resp;
    unsigned char Rc = 0, Gc = 0;
    char Val;
    int a, b, c, ppos;

    for (a = 0; a < 8; a++)
        ValueChars[a] = 0;
    ppos = 0;
    DrawInfoScreen();
    while (RunningTask == TASK_INFO) {
        while (TouchedX == -1) {
            rqst.tv_sec = 0;
            rqst.tv_nsec = 200000000;
            nanosleep(&rqst, &resp);
            if (ScrTouched) {
                switch (ScrTouched) {
                case 1:
                    Rc = 255;
                    Gc = 0;
                    ScrTouched = 7;
                    break;
                case 2:
                    Rc = 255;
                    Gc = 255;
                    ScrTouched = 7;
                    break;
                case 3:
                    Rc = 0;
                    Gc = 255;
                    ScrTouched = 7;
                    break;
                case 7:
                    Rc = 0;
                    Gc = 0;
                    ScrTouched = 0;
                    break;
                }
                for (b = 0; b < 20; b++) {
                    c = b * WIDTH;
                    for (a = 0; a < 20; a++) {
                        ImageBuffer[c].R = Rc;
                        ImageBuffer[c].G = Gc;
                        ImageBuffer[c].B = 0;
                        ++c;
                    }
                }
                UICopyDisplayBufferToScreen(0, 0, 20, 20);
            }
//printf(",");fflush(stdout);
        }
        EvaluateTouch();
        DrawInfoScreen();
        Val = 0;
        switch (TouchedButton) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            ValueChars[ppos] = KP[TouchedButton];       // 1234567890.C
            if (++ppos > 7)
                ppos = 7;
            break;
        case 11:               // C
            if (ppos)
                --ppos;
            ValueChars[ppos] = 0;
            break;
        case 12:               // Refuel
            ClearDisplayBuffer();
            if ((sscanf(ValueChars, "%f", &RefuelValue)) != 1) {
                RefuelValue = 0.0f;
                if (SI_Measurements)
                    sprintf(Message, "Ltr: %s", ValueChars);
                else
                    sprintf(Message, "Gal: %s", ValueChars);
                PutMyString(Message, 20, 140, 1, 3);
                sprintf(Message, "Invalid Number conversion!");
                PutMyString(Message, 20, 220, 1, 3);
            }
            else {
                if (SI_Measurements) {
                    sprintf(Message, "Refueled set with %2.3f Ltr.", RefuelValue);
                    RefuelValue *= 3.78f;
                }
                else {
                    sprintf(Message, "Refueled set with %2.3f Gal.", RefuelValue);
                }
                c = GetMyStringLength(Message, 1, 3);
                PutMyString(Message, (320 - (c >> 1)), 280, 1, 3);
            }
            UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
            sleep(3);
            break;
        case 13:               // Exit
            RunningTask = TASK_INIT;
            ClearDisplayBuffer();
            sprintf(Message, "Exit Task Menu.");
            c = GetMyStringLength(Message, 1, 3);
            PutMyString(Message, (320 - (c >> 1)), 180, 1, 3);
            if (RefuelValue > 0.1f) {
                if (SI_Measurements)
                    sprintf(Message, "Refueled set with %2.3f Ltr.", (RefuelValue * 3.78f));
                else
                    sprintf(Message, "Refueled set with %2.3f Gal.", RefuelValue);
                c = GetMyStringLength(Message, 1, 3);
                PutMyString(Message, (320 - (c >> 1)), 280, 1, 3);
            }
            if (TripKilometers < 0.1f) {
                sprintf(Message, "Full tank set.");
                c = GetMyStringLength(Message, 1, 3);
                PutMyString(Message, (320 - (c >> 1)), 340, 1, 3);
            }
            UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
            sleep(2);
            return;
            break;
        case 14:               // Full Tank
            TripKilometers = 0.0f;
            TripGallonValue = 0.0f;
            break;
        case 15:               // Voice: Yes/No
            if (++VoiceMode > 1)
                VoiceMode = 0;
            sprintf(Buttons[15].Title, "Voice: %s", ((VoiceMode) ? ("Yes") : ("No")));
            break;
        }
        TouchedX = -1;
        TouchedButton = -1;
        DrawInfoScreen();
    }
}

void
SS_Stats(void)
{
    int a, b, c;
    float MPG, ThrottleV, TKM, TGAL;

    if (AccSpdCntr < 1)
        return;

    for (b = 0; b < HEIGHT; b++) {
        c = b * WIDTH;
        for (a = 0; a < WIDTH; a++) {
            ImageBuffer[c].R = 0;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }

    if (AccRpm > 0.0f)
        MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
    else
        MPG = 0.0f;
    if (MPG > 0.0f) {
        if (AccSpdCntr > 0)
            ThrottleV = ((AccSpd * (float) TimeElapsed) / ((float) AccSpdCntr * 5760.0f * MPG));        // This many gallons were used on this trip
        else
            ThrottleV = 0.0;
    }
    else
        ThrottleV = 0.0f;

    if (AccSpdCntr > 0)
        DistanceTravelled = ((AccSpd * (float) TimeElapsed) / ((float) AccSpdCntr * 3600.0f));  // Since we computed /s updates -> /h
    else
        DistanceTravelled = 0;
    if (SI_Measurements)
        sprintf(Message, "Dist= %1.1f      Cons.= %2.1f km/l", (DistanceTravelled),
                (MPG * 0.42328f));
    else
        sprintf(Message, "Dist= %1.1f miles     MPG= %2.1f W", (DistanceTravelled * 0.625f), MPG);
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, (320 - (c >> 1)), 255, 1, 3);

    if (SI_Measurements)
        sprintf(Message, "Fuel Consumption= %1.3f Ltr", (ThrottleV * 3.78f));
    else
        sprintf(Message, "Fuel Consumption= %1.3f Gal", ThrottleV);
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, (320 - (c >> 1)), 310, 1, 3);

    TGAL = TripGallonValue;
    if (AccRpm > 0.0f) {
        if (AccSpd > 0.0f)
            MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
        else {
            if (AccSpdCntr > 0)
                MPG = (float) (((AccSpdCntr * SPDSCALER_MILE) / AccRpm));
            else
                MPG = (float) (((SPDSCALER_MILE) / AccRpm));
        }
        if (AccSpdCntr > 0)
            TGAL += (float) (((AccSpd * (float) TimeElapsed) / ((float) AccSpdCntr * MPG)));    // must be divided by 5760 for real number later ( preserving accuracy )
    }
    TKM = TripKilometers + DistanceTravelled;

    if (SI_Measurements)
        sprintf(Message, "Last Refill = %1.2f l : %1.1f km", ((TGAL * 3.78) / 5760.0f), TKM);
    else
        sprintf(Message, "Last Refill = %1.2f gal : %1.1f m", (TGAL / 5760.0f), (TKM * 0.625f));
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, (320 - (c >> 1)), 370, 1, 3);

    sprintf(Message, "- TRIP -");
    c = GetMyStringLength(Message, 0, 4);
    PutMyString(Message, (320 - (c >> 1)), 150, 0, 4);

    ThrottleV = ((float) AccSpd / (float) AccSpdCntr) * 0.625f; // Average Trip Speed
    a = (int) ((float) TimeElapsed / 60.0f);    // minute
    b = TimeElapsed - (a * 60); // seconds
    if (a >= 60) {
        c = (int) ((float) a / 60.0f);  // hour
        a -= (c * 60);
        if (SI_Measurements)
            sprintf(Message, "Time= %1d:%02d:%02d  A.Spd= %2.1f km/h", c, a, b, (ThrottleV * 1.6f));
        else
            sprintf(Message, "Time= %1d:%02d:%02d   A.Spd= %2.1f MPH", c, a, b, ThrottleV);
    }
    else {
        if (SI_Measurements)
            sprintf(Message, "Time= %02d:%02d  Avg.Spd= %2.1f km/h", a, b, (ThrottleV * 1.6f));
        else
            sprintf(Message, "Time= %02d:%02d   Avg.Spd= %2.1f MPH", a, b, ThrottleV);
    }
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, (320 - (c >> 1)), 200, 1, 3);

// Accumulative

    ThrottleV =
        ((MeasuredKMPG * 0.625f * MeasurementKilometers) +
         (MPG * DistanceTravelled)) / (MeasurementKilometers + DistanceTravelled);
    if (SI_Measurements)
        sprintf(Message, "%d km = %2.1f km/l",
                (int) ((MeasurementKilometers + DistanceTravelled) + 0.5f), (ThrottleV * 0.42328f));
    else
        sprintf(Message, "%d Miles = %2.1f W",
                (int) (((MeasurementKilometers + DistanceTravelled) * 0.625f) + 0.5f), ThrottleV);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (320 - (c >> 1)), 4, 0, 3);

    sprintf(Message, "Chrg=%3.1fA  DisC=%3.1fA", MeasuredMinCurrent, MeasuredMaxCurrent);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (320 - (c >> 1)), 45, 0, 3);

    sprintf(Message, "Chrg=%3.1fkw  DisC=%3.1fkw", MeasuredMinW, MeasuredMaxW);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (320 - (c >> 1)), 86, 0, 3);
}


#define	COPY_COMMAND	"su -c 'cp PriusData.txt FuelData.txt /mnt/card;umount /mnt/card'"
int
CopyDataToMemoryCard(void)
{
    struct stat st;

    if (!stat("/mnt/card", &st)) {
        if (st.st_ino == 1)     // card mounted
        {
            system(COPY_COMMAND);
            return (1);
        }
    }
    return (0);
}


void
ScreenSaver(void)
{
    int c;

    if (--ScrSv_NumberOfTimes < 1) {
        SS_Stats();
        ScrSv_NumberOfTimes = 300;
        c = GetMyStringLength(VERSION_STRING, 0, 2);
        PutMyString(VERSION_STRING, ((WIDTH >> 1) - c), 450, 0, 2);
        UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
    }
    if (TouchedX != -1) {
        if ((TouchedX == 17) && (TouchedY == 17)) {
            ClearDisplayBuffer();
            if ((CopyDataToMemoryCard()) == 1)
                sprintf(Message, "Save to card SUCCESS.");
            else
                sprintf(Message, "Save to card FAILED.");
            PutMyString(Message, 10, 100, 1, 3);
            PutMyString(Message, 20, 200, 1, 3);
            PutMyString(Message, 30, 300, 1, 3);
            PutMyString(Message, 40, 400, 1, 3);
            ScrSv_NumberOfTimes = 2;
            TouchedX = -1;
            TouchedY = -1;
        }
        UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
    }
    sleep(1);

#if 0
    tx = (int) (((float) (rand() & 0xFFF0) * 16) / (float) 0xFFFF);     // 0-15
    ty = (int) (((float) (rand() & 0xFFF0) * 10) / (float) 0xFFFF) + 2; // 0-11
    if ((tx *= 40) > 600)
        tx = 600;               // to further slow down processing...
    if ((ty *= 40) > 440)
        ty = 440;

    for (b = ty; b < (ty + 40); b++) {
        c = tx + b * WIDTH;
        for (a = 0; a < 40; a++) {
            ImageBuffer[c].R = (unsigned char) 0;
            ImageBuffer[c].G = (unsigned char) 0;
            ImageBuffer[c].B = (unsigned char) 0;
            ++c;
        }
    }

    UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);

#endif

}

void
LoadStat(void)
{
    FILE *fp = NULL;
    float MPG;
    int a, b, c, Go = 3, v1, v2, v3, v4, v5, v6, v7, v8, v9;

    while (Go > 0) {
        for (b = 0; b < 46; b++) {
            c = b * WIDTH;
            for (a = 0; a < (WIDTH - 10); a++) {
                ImageBuffer[c].R = 0;
                ImageBuffer[c].G = 0;
                ImageBuffer[c].B = 0;
                ++c;
            }
        }
        sprintf(Message, "Loading...");
        c = GetMyStringLength(Message, 1, 3);
        PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 3);
        UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);

        if ((fp = fopen(stat_file_name, "r")) == NULL) {
//                      printf("\n\nERROR : Can not open file '%s' for reading...",stat_file_name);fflush(stdout);
            for (b = 0; b < 46; b++) {
                c = b * WIDTH;
                for (a = 0; a < (WIDTH - 10); a++) {
                    ImageBuffer[c].R = 0;
                    ImageBuffer[c].G = 0;
                    ImageBuffer[c].B = 0;
                    ++c;
                }
            }
            sprintf(Message, "Loading Failed...");
            c = GetMyStringLength(Message, 1, 3);
            PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 3);
            UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);
            sleep(3);
            --Go;
        }
        else {
            fscanf(fp, "Measured Miles = %f\n", &MPG);
            fscanf(fp, "Accumulated MPG = %f\n", &MPG);
            fscanf(fp, "Accumulated Mean MPH = %f\n", &MPG);
            fscanf(fp, "Accumulated KMPG = %f\n", &MeasuredKMPG);
            fscanf(fp, "Accumulated MeanKMH = %f\n", &MeasuredAverageSpeed);
            fscanf(fp, "MeasuredKm = %f\n", &MeasurementKilometers);
            fscanf(fp, "LastFuelValue = %d\n", &a);
            GasGauge = (unsigned char) a;
            if ((fscanf(fp, "Max Charge Current = %f A\n", &MeasuredMinCurrent)) != 1)
                MeasuredMinCurrent = 0.0f;
            if ((fscanf(fp, "Max Charge Power = %f kW\n", &MeasuredMinW)) != 1)
                MeasuredMinW = 0.0f;
            if ((fscanf(fp, "Max Discharge Current = %f A\n", &MeasuredMaxCurrent)) != 1)
                MeasuredMaxCurrent = 0.0f;
            if ((fscanf(fp, "Max Discharge Power = %f kW\n", &MeasuredMaxW)) != 1)
                MeasuredMaxW = 0.0f;

            for (b = 0; b < 50; b++) {
                if ((fscanf
                     (fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", &v1, &v2, &v3, &v4, &v5, &v6, &v7,
                      &v8, &v9)) == 9) {
                    MaxCurrentValues[b][0] = (unsigned char) v1;
                    MaxCurrentValues[b][1] = (unsigned char) v2;
                    MaxCurrentValues[b][2] = (unsigned char) v3;
                    MaxCurrentValues[b][3] = (unsigned char) v4;
                    MaxCurrentValues[b][4] = (unsigned char) v5;
                    MaxCurrentValues[b][5] = (unsigned char) v6;
                    MaxCurrentValues[b][6] = (unsigned char) v7;
                    MaxCurrentValues[b][7] = (unsigned char) v8;
                    MaxCurrentValues[b][8] = (unsigned char) v9;
                }
                else
                    b = 50;
            }

            if ((fscanf(fp, "Trip Kilometers = %f\n", &MPG)) == 1)
                TripKilometers = MPG;
            if ((fscanf(fp, "Trip Gallon Usage Indicator = %f\n", &MPG)) == 1)
                TripGallonValue = MPG;
            fscanf(fp, "Trip Gallon Usage = %f\n", &MPG);
            if ((fscanf(fp, "Voice Announcements = %d\n", &a)) == 1) {
                VoiceMode = (unsigned char) a;
            }
            if ((fscanf(fp, "SI Measurements = %d\n", &a)) == 1) {
                SI_Measurements = (unsigned char) a;
            }

            fclose(fp);
            if ((MeasurementKilometers > 9999999.0f) || (MeasurementKilometers < 0.0f) ||
                (MeasuredKMPG < 0.0f) || (MeasuredKMPG > 999.0f) ||
                (MeasuredAverageSpeed < 0.0f) || (MeasuredAverageSpeed > 300.0f)) {
                for (b = 0; b < 46; b++) {
                    c = b * WIDTH;
                    for (a = 0; a < (WIDTH - 10); a++) {
                        ImageBuffer[c].R = 0;
                        ImageBuffer[c].G = 0;
                        ImageBuffer[c].B = 0;
                        ++c;
                    }
                }
                sprintf(Message, "INVALID DATA while loading...");
                c = GetMyStringLength(Message, 1, 2);
                PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 2);
                UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);
                sleep(3);
                --Go;
            }
            else
                Go = 0;
        }
    }

    for (b = 0; b < 46; b++) {
        c = b * WIDTH;
        for (a = 0; a < (WIDTH - 10); a++) {
            ImageBuffer[c].R = 0;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }
    sprintf(Message, "Loading... OK.");
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 3);
    UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);

#if 0
    printf("Accumulated KMPG = %1.6f\n", MeasuredKMPG);
    printf("Accumulated MeanKMH = %1.6f\n", MeasuredAverageSpeed);
    printf("MeasuredKm = %1.6f\n", MeasurementKilometers);
#endif

}


void
SaveStat(void)
{
    FILE *fp = NULL;
    float MPG, AMPG;
    int a, b, c;

    for (b = 0; b < 46; b++) {
        c = b * WIDTH;
        for (a = 0; a < (WIDTH - 10); a++) {
            ImageBuffer[c].R = 0;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }
    sprintf(Message, "Saving...");
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 3);
    UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);

    if ((DistanceTravelled < 0.001f) && (AccRpm < 0.001f)) {
        for (b = 0; b < 46; b++) {
            c = b * WIDTH;
            for (a = 0; a < (WIDTH - 10); a++) {
                ImageBuffer[c].R = 0;
                ImageBuffer[c].G = 0;
                ImageBuffer[c].B = 0;
                ++c;
            }
        }
        sprintf(Message, "Not Saved... Unnecessary...");
        c = GetMyStringLength(Message, 1, 2);
        PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 2);
        UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);
        sleep(1);
        return;
    }

    if ((MeasurementKilometers > 999999.0f) || (MeasurementKilometers < 0.0f) ||
        (DistanceTravelled > 9999.0f) || (DistanceTravelled < 0.0f) ||
        (MeasuredKMPG < 0.0f) || (MeasuredKMPG > 99999.0f) ||
        (MeasuredAverageSpeed < 0.0f) || (MeasuredAverageSpeed > 180.0f)) {
        for (b = 0; b < 46; b++) {
            c = b * WIDTH;
            for (a = 0; a < (WIDTH - 10); a++) {
                ImageBuffer[c].R = 0;
                ImageBuffer[c].G = 0;
                ImageBuffer[c].B = 0;
                ++c;
            }
        }
        sprintf(Message, "Not Saved... INVALID DATA...");
        c = GetMyStringLength(Message, 1, 2);
        PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 2);
        UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);
        sleep(3);
        return;
    }

    if ((fp = fopen(stat_file_name, "w")) == NULL) {
        printf("\n\nERROR : Can not open file '%s' for writing...", stat_file_name);
        fflush(stdout);
        for (b = 0; b < 46; b++) {
            c = b * WIDTH;
            for (a = 0; a < (WIDTH - 10); a++) {
                ImageBuffer[c].R = 0;
                ImageBuffer[c].G = 0;
                ImageBuffer[c].B = 0;
                ++c;
            }
        }
        sprintf(Message, "Saving... FAILED");
        c = GetMyStringLength(Message, 1, 3);
        PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 3);
        UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);
        sleep(3);
        return;
    }


    fprintf(fp, "Measured Miles = %1.2f\n", ((MeasurementKilometers + DistanceTravelled) * 0.625f));

    if (AccRpm > 0.0f)
        MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
    else
        MPG = 0.0f;
    if ((MeasurementKilometers + DistanceTravelled) > 0.0f)
        AMPG =
            ((MeasuredKMPG * 0.625f * MeasurementKilometers) +
             (MPG * DistanceTravelled)) / (MeasurementKilometers + DistanceTravelled);
    else
        AMPG = 0.0f;
    fprintf(fp, "Accumulated MPG = %2.1f\n", AMPG);

    if ((MeasurementKilometers + DistanceTravelled) > 0.0f)
        AMPG =
            ((MeasuredAverageSpeed * MeasurementKilometers) +
             ((AccSpd / AccSpdCntr) * DistanceTravelled)) / (MeasurementKilometers +
                                                             DistanceTravelled);
    else
        AMPG = 0.0f;
    fprintf(fp, "Accumulated Mean MPH = %1.1f\n", (AMPG * 0.625f));

    if (AccRpm > 0.0f)
        MPG = (float) (((AccSpd * SPDSCALER) / AccRpm));
    else
        MPG = 0.0f;
    if ((MeasurementKilometers + DistanceTravelled) > 0.0f)
        AMPG =
            ((MeasuredKMPG * MeasurementKilometers) +
             (MPG * DistanceTravelled)) / (MeasurementKilometers + DistanceTravelled);
    else
        AMPG = 0.0f;
    fprintf(fp, "Accumulated KMPG = %1.6f\n", AMPG);

    if ((MeasurementKilometers + DistanceTravelled) > 0.0f)
        AMPG =
            ((MeasuredAverageSpeed * MeasurementKilometers) +
             ((AccSpd / AccSpdCntr) * DistanceTravelled)) / (MeasurementKilometers +
                                                             DistanceTravelled);
    else
        AMPG = 0.0f;
    fprintf(fp, "Accumulated MeanKMH = %1.6f\n", AMPG);

    fprintf(fp, "MeasuredKm = %1.6f\n", (MeasurementKilometers + DistanceTravelled));

    fprintf(fp, "LastFuelValue = %d\n", (int) GasGauge);

    if (((float) MinCurrent * 0.1f) > 500.0f)   // Error!
    {

        printf("\nERROR in MinCurrent !");
        printf("\nMinCurrent = %d ( %f )", MinCurrent, ((float) MinCurrent * 0.1f));
        printf("\nMinCurrentVoltage = %d ( %f )", MinCurrentVoltage,
               ((float) MinCurrentVoltage * 0.001f));
        fflush(stdout);

    }
    else {
        if (MeasuredMinCurrent < ((float) MinCurrent * 0.1f)) {
            MeasuredMinCurrent = ((float) MinCurrent * 0.1f);
            MeasuredMinW = MeasuredMinCurrent * (float) MinCurrentVoltage *0.001f;
        }
    }

    if (MeasuredMaxCurrent < ((float) MaxCurrent * 0.1f)) {
        MeasuredMaxCurrent = ((float) MaxCurrent * 0.1f);
        MeasuredMaxW = MeasuredMaxCurrent * (float) MaxCurrentVoltage *0.001f;
    }

    fprintf(fp, "Max Charge Current = %3.1f A\n", MeasuredMinCurrent);
    fprintf(fp, "Max Charge Power = %2.2f kW\n", MeasuredMinW);
    fprintf(fp, "Max Discharge Current = %3.1f A\n", MeasuredMaxCurrent);
    fprintf(fp, "Max Discharge Power = %2.2f kW\n", MeasuredMaxW);
    for (b = 0; b < 50; b++) {
        fprintf(fp, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", MaxCurrentValues[b][0],
                MaxCurrentValues[b][1], MaxCurrentValues[b][2], MaxCurrentValues[b][3],
                MaxCurrentValues[b][4], MaxCurrentValues[b][5], MaxCurrentValues[b][6],
                MaxCurrentValues[b][7], MaxCurrentValues[b][8]);
    }

    AMPG = TripGallonValue;
    if (AccRpm > 0.0f) {
        if (AccSpd > 0.0f)
            MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
        else {
            if (AccSpdCntr > 0)
                MPG = (float) (((AccSpdCntr * SPDSCALER_MILE) / AccRpm));
            else
                MPG = (float) (((SPDSCALER_MILE) / AccRpm));
        }
        if ((AccSpdCntr > 0) && (MPG > 0.0f))
            AMPG += (float) (((AccSpd * (float) TimeElapsed) / ((float) AccSpdCntr * MPG)));    // must be divided by 5760 for real number later ( preserving accuracy )
    }

    fprintf(fp, "Trip Kilometers = %3.6f\n", (TripKilometers + DistanceTravelled));
    fprintf(fp, "Trip Gallon Usage Indicator = %3.6f\n", AMPG);
    fprintf(fp, "Trip Gallon Usage = %2.2f\n", (AMPG / 5760.0f));
    fprintf(fp, "Voice Announcements = %d\n", VoiceMode);
    fprintf(fp, "SI Measurements = %d\n", SI_Measurements);
    fprintf(fp, "\n");

    fflush(fp);
    fclose(fp);

    for (b = 0; b < 46; b++) {
        c = b * WIDTH;
        for (a = 0; a < (WIDTH - 10); a++) {
            ImageBuffer[c].R = 0;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }
    sprintf(Message, "Saving... Ok.");
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, ((WIDTH - 10 - c) >> 1), 9, 0, 3);
    UICopyDisplayBufferToScreen(0, 0, (WIDTH - 10), 46);
}

int
WriteToPort(char *ZTV)
{
    register int nofbytes, written, startpoz, looping;
    int reva = 0;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'W';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    if (Simulation) {
        return 1;
    }

    if (Port == -1)
        return 1;

    nofbytes = 0;
    startpoz = 0;
    while (*(ZTV + nofbytes) != '\0')
        ++nofbytes;

    looping = 256;              // Give up after 256 tries...
    while (looping) {
        written = write(Port, &ZTV[startpoz], nofbytes);
        if (written != nofbytes)        // did NOT write'em all
        {
            startpoz += written;
            nofbytes -= written;
            if ((--looping) == 0)
                reva = 1;       // error
        }
        else
            looping = 0;
    }

//      tcflush(Port,TCOFLUSH);

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'w';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    return (reva);
}


int
OpenAndConfigurePort(void)
{

    struct termios newio;
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'O';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

#ifdef COMM_DEBUG
    printf("\nOpening port on %s.", DEVICE);
    fflush(stdout);
#endif
    if (Port == -1) {
        if ((Port = open(DEVICE, O_RDWR | O_NOCTTY)) < 0) {
            printf("\nError Opening Serialport ( %s ) : '%s'", DEVICE, strerror(errno));
            fflush(stdout);
#ifdef TRACE_IT
            TrBu[TrBuPtr] = 'o';
            if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
                TrBuPtr = 0;
#endif
            return (1);
        }
    }
    memset(&newio, 0, sizeof(newio));   /* Clears termios struct  */
    newio.c_cflag = CS8 | CLOCAL | CREAD;
    newio.c_iflag = IGNPAR;
    newio.c_oflag = 0;
    newio.c_lflag = 0;
    newio.c_cc[VTIME] = 0;
    newio.c_cc[VMIN] = 0;       /* read min. one char at a time  */
    if (cfsetispeed(&newio, BAUD) == -1) {
        printf("Error setting serial input baud rate\n");
        close(Port);
        Port = -1;
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'o';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return (1);
    }
    if (cfsetospeed(&newio, BAUD) == -1) {
        printf("Error setting serial output baud rate\n");
        close(Port);
        Port = -1;
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'o';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return (1);
    }
    tcflush(Port, TCIFLUSH);
    if (tcsetattr(Port, TCSANOW, &newio) == -1) {
        printf("Error setting terminal attributes\n");
        close(Port);
        Port = -1;
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'o';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return (1);
    }

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'o';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    return (0);
}


int
SetUpCAN(void)
{
    int to;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'A';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    if (IB == NULL) {
        IBS = 16 * 1024;
        if ((IB = (char *) malloc((size_t) IBS)) == NULL) {
            printf("\n\nCan not allocate buffer with %d size...", IBS);
            fflush(stdout);
#ifdef TRACE_IT
            TrBu[TrBuPtr] = 'a';
            if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
                TrBuPtr = 0;
#endif
            return (0);
        }
    }
    Version[0] = '\0';
    Serial[0] = '\0';
    Serial[4] = '\0';
    IBCP = 0;

    if (Simulation) {
        Port = 0;
        return 1;
    }


//printf("\nCalling OACPort");fflush(stdout);
    if (OpenAndConfigurePort()) {
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'a';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        printf(" -> Zero");
        fflush(stdout);
        return (0);
    }

//printf("\nPort opened.");fflush(stdout);
//printf("\n... Clearing buffer");fflush(stdout);

    sprintf(Message, "\015\015\015");
    if (WriteToPort(Message)) {
        printf("\nError writing to port : Clearing Buffer");
        fflush(stdout);
    }

//printf("\n... Sending Version Request");fflush(stdout);

    sprintf(Message, "V\015");  // get version
    if (WriteToPort(Message)) {
        printf("\nError writing to port : Get Version");
        fflush(stdout);
    }

    to = 1000000;
    while (to) {
        if (Poll()) {
            if (Version[0] != '\0')
                to = 1;
        }
        --to;
    }

//printf("\nVersion = HW : %c.%c  SW : %c.%c",Version[0],Version[1],Version[2],Version[3]);fflush(stdout);

    if (Version[0] == '\0')
        return (0);             // Communication Error

//printf("\n... Sending Serial Request");fflush(stdout);

    sprintf(Message, "N\015");  // get Serial
    if (WriteToPort(Message)) {
        printf("\nError writing to port : Get Serial");
        fflush(stdout);
    }

    to = 1000000;
    while (to) {
        if (Poll()) {
            if (Serial[0] != '\0')
                to = 1;
        }
        --to;
    }

//printf("\nSerial ID = %s  ( %02x:%02x:%02x:%02x )",Serial,Serial[0],Serial[1],Serial[2],Serial[3]);fflush(stdout);

    CRSignal = 0;
    sprintf(Message, "S6\015"); // CAN with 500Kbps S0-10 S1-20 S2-50 S3-100 S4-125 S5-250 S7-800  S8-1M
    if (WriteToPort(Message)) {
        printf("\nError writing to port : Setting CAN speed");
        fflush(stdout);
    }

    to = 1000000;
    while (to) {
        if (Poll()) {
            if (CRSignal)
                to = 1;
        }
        --to;
    }

//printf("\nCAN speed setup.");fflush(stdout);


    CRSignal = 0;

#ifdef MORE_DATA
    sprintf(Message, "M29000400\015");  // Wide Special Prius profiled
#else
    sprintf(Message, "M07202000\015");  // Special Prius profiled
//      sprintf(Message,"M07602000\015");       // Old Default Special Prius profiled
//      sprintf(Message,"M06002000\015");       // Special Prius profiled

#endif

    if (WriteToPort(Message)) {
        printf("\nError writing to port : M register");
        fflush(stdout);
    }
    to = 1000000;
    while (to) {
        if (Poll()) {
            if (CRSignal)
                to = 1;
        }
        --to;
    }
    CRSignal = 0;

#ifdef MORE_DATA
    sprintf(Message, "mD6EFB3EF\015");  // Wide Special Prius profiled
#else
    sprintf(Message, "m004FDFEF\015");  // Special Prius profiled
//      sprintf(Message,"m000FDFEF\015");       // Old Default Special Prius profiled
//      sprintf(Message,"m416FDFEF\015");       // Special Prius profiled
#endif

    if (WriteToPort(Message)) {
        printf("\nError writing to port : m register");
        fflush(stdout);
    }
    to = 1000000;
    while (to) {
        if (Poll()) {
            if (CRSignal)
                to = 1;
        }
        --to;
    }

#if 0
    CRSignal = 0;
    sprintf(Message, "X1\015"); // Turn on out poll
    if (WriteToPort(Message)) {
        printf("\nError writing to port : Turn on poll");
        fflush(stdout);
    }
    to = 1000000;
    while (to) {
        if (Poll()) {
            if (CRSignal)
                to = 1;
        }
        --to;
    }

//printf("\nAuto Poll setup.");fflush(stdout);

    CRSignal = 0;
    sprintf(Message, "Z0\015"); // Turn off timestamp
    if (WriteToPort(Message)) {
        printf("\nError writing to port : Turn off timestamp");
        fflush(stdout);
    }
    to = 1000000;
    while (to) {
        if (Poll()) {
            if (CRSignal)
                to = 1;
        }
        --to;
    }

//printf("\nNo timestamp setup.");fflush(stdout);

#endif


    CRSignal = 0;
    sprintf(Message, "O\015");  // Open the CAN channel
    if (WriteToPort(Message)) {
        printf("\nError writing to port : Open CAN channel");
        fflush(stdout);
    }
    to = 1000000;
    while (to) {
        if (Poll()) {
            if (CRSignal)
                to = 1;
        }
        --to;
    }

//printf("\nChannel opened.");fflush(stdout);

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'a';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    return (1);
}


#ifdef TRACE_IT

int NoTD = 0;

void
DumpTraceBuffer(void)
{
    FILE *fp;
    int a;

    if (NoTD > 10)
        return;                 // No more dumping
    ++NoTD;
//      if((fp=fopen("Trace.txt","a"))==NULL)
    if (1) {
        printf("\nTB : ");
        for (a = (TrBuPtr + 1); a < TRACE_BUFFER_LENGTH; a++)
            printf("%c", TrBu[a]);
        for (a = 0; a < TrBuPtr; a++)
            printf("%c", TrBu[a]);
        fflush(stdout);
    }
    else {
        fprintf(fp, "\nTB : ");
        for (a = (TrBuPtr + 1); a < TRACE_BUFFER_LENGTH; a++)
            fprintf(fp, "%c", TrBu[a]);
        for (a = 0; a < TrBuPtr; a++)
            fprintf(fp, "%c", TrBu[a]);
        fflush(fp);
        fclose(fp);
    }
}
#endif

void
CleanUp(int vis)
{
    UICleanUp(vis);

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'C';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
    DumpTraceBuffer();
#endif

#ifdef TRACE_IT
    TrBu[TrBuPtr] = '1';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    if (!Simulation && Port >= 0) {
        sprintf(Message, "C\015");      // Close the CAN channel
        if (WriteToPort(Message)) {
            printf("\nError writing to port : Close CAN channel");
            fflush(stdout);
        }
        close(Port);
        Port = -1;
    }

#ifdef TRACE_IT
    TrBu[TrBuPtr] = '2';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    if (IB != NULL) {
        free((void *) IB);
        IB = NULL;
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = '3';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    if (ImageBuffer != NULL) {
        free((void *) ImageBuffer);
        ImageBuffer = NULL;
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = '4';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    if (WorkData != NULL) {
        free((void *) WorkData);
        WorkData = NULL;
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = '5';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

#ifdef USE_TOUCHSCREEN
    if (touch_screen_fd >= 0)
        close(touch_screen_fd);
#endif

#ifdef TRACE_IT
    TrBu[TrBuPtr] = '7';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

#ifdef USE_KEYBOARD
    ioctl(0, TCSETS, &oldT);
#endif

#ifdef TRACE_IT
    TrBu[TrBuPtr] = '8';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'c';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
    DumpTraceBuffer();
#endif
}


void
SendCATRequests(void)
{
    if (CATReqCtr == 1) {
        sprintf(Message, "t7E0802213C0000000000\015");  // CAT 1
        CATReqCtr = 0;
    }
    else {
        sprintf(Message, "t7E0802213E0000000000\015");  // CAT 2
        CATReqCtr = 1;
    }
    WriteToPort(Message);
}


#ifdef USE_VOICE_ANNOUNCEMENT

enum
{ V_EVACT =
        0, V_EVCANC, V_BATTERYAT, V_PERCENT, V_GREETINGS, V_DOOROPEN, V_THETIMEIS, V_1, V_2, V_3,
    V_4, V_5, V_6, V_7, V_8, V_9, V_10, V_11, V_12, V_13, V_14, V_15, V_16, V_17, V_18, V_19,
    V_20, V_30, V_40, V_50, V_60, V_70, V_80, V_90, V_100, V_0, V_OCLOCK, V_FUELAVAIL, V_MILES,
    V_LAST, V_FLUSH
};
char *VoiceFiles[] =
    { "EVAct.mp3", "EVCanc.mp3", "BatteryAt.mp3", "Percent.mp3", "Greetings.mp3", "DoorOpen.mp3",
    "TheTimeIs.mp3", "1.mp3", "2.mp3", "3.mp3", "4.mp3", "5.mp3", "6.mp3", "7.mp3", "8.mp3",
    "9.mp3", "10.mp3",
    "11.mp3", "12.mp3", "13.mp3", "14.mp3", "15.mp3", "16.mp3", "17.mp3", "18.mp3", "19.mp3",
    "20.mp3", "30.mp3",
    "40.mp3", "50.mp3", "60.mp3", "70.mp3", "80.mp3", "90.mp3", "100.mp3", "0.mp3", "OClock.mp3",
    "FuelAvailable.mp3", "Miles.mp3"
};

#define	    V_BATLAST	(V_BAT80+1)

#define		CODE_BUFFER_SIZE	16

pthread_t SubTh;
int VoiceCode[CODE_BUFFER_SIZE];
int ReadPtr = 0, WritePtr = 0;
unsigned char PrevH = 13, PrevM = 61, ThreadLive = 0;

void *
Thread_Start_Routine(void *tmp)
{
    char TmpBuf[511];
    char Go = 1;
    int a;

    ThreadLive = 1;
    while (Go) {
        a = 0;

        while ((VoiceCode[ReadPtr] != V_FLUSH) && (VoiceCode[ReadPtr] != -1)) {
//printf("\nTH : Code=%d  RPtr=%d  WPtr=%d",VoiceCode[ReadPtr],ReadPtr,WritePtr);fflush(stdout);
            if (a == 0) {
                if (ForcePath)
                    sprintf(TmpBuf, "%s/mp -Q %s/%s%c", FORCE_PATH, FORCE_PATH,
                            VoiceFiles[VoiceCode[ReadPtr]], 0);
                else
                    sprintf(TmpBuf, "mp -Q %s%c", VoiceFiles[VoiceCode[ReadPtr]], 0);
            }
            else {
                if (ForcePath)
                    sprintf(&TmpBuf[a], " %s/%s%c", FORCE_PATH, VoiceFiles[VoiceCode[ReadPtr]], 0);
                else
                    sprintf(&TmpBuf[a], " %s%c", VoiceFiles[VoiceCode[ReadPtr]], 0);
            }
            while (TmpBuf[a] != 0)
                ++a;
            if (++ReadPtr > (CODE_BUFFER_SIZE - 1))
                ReadPtr = 0;
        }
        if (a > 0) {
//printf("\nPLAY : %d : '%s'",a,TmpBuf);fflush(stdout);
            system(TmpBuf);
        }
        if (VoiceCode[ReadPtr] == V_FLUSH) {
            if (++ReadPtr > (CODE_BUFFER_SIZE - 1))
                ReadPtr = 0;
        }

        if (VoiceCode[ReadPtr] == -1) {
            Go = 0;             // Terminator
//                      printf("\nTH : END RPtr=%d  WPtr=%d",ReadPtr,WritePtr);fflush(stdout);
        }
    }
    ThreadLive = 0;
    pthread_exit(NULL);
}

void
InitVoice(void)
{
    int a;

    for (a = 0; a < CODE_BUFFER_SIZE; a++)
        VoiceCode[a] = -1;
    PrevH = 13;
    PrevM = 61;
}

void
CallVoice(int VCode, int Flush)
{
    if (VoiceMode == 0)
        return;
    if (VCode >= V_LAST)
        return;

//printf("\nCV : Code=%d  WPtr=%d RPtr=%d Flush=%d ThrLiv=%d",VCode,WritePtr,ReadPtr,Flush,ThreadLive);fflush(stdout);

    VoiceCode[WritePtr] = VCode;
    if (++WritePtr > (CODE_BUFFER_SIZE - 1))
        WritePtr = 0;
    VoiceCode[WritePtr] = -1;
    if (Flush) {
        VoiceCode[WritePtr] = V_FLUSH;
        if (++WritePtr > (CODE_BUFFER_SIZE - 1))
            WritePtr = 0;
        VoiceCode[WritePtr] = -1;
        if (ThreadLive == 0)
            pthread_create(&SubTh, NULL, Thread_Start_Routine, NULL);
    }
}

void
SayTime(void)
{
    int a, pm;
    time_t t0;
    struct tm *ct;

    t0 = time(0);
    ct = localtime(&t0);
    PrevH = (unsigned char) (ct->tm_hour);
    PrevM = (unsigned char) (ct->tm_min);
    CallVoice(V_THETIMEIS, 0);
    if (ct->tm_hour >= 12)
        pm = 1;
    if (ct->tm_hour == 0)
        ct->tm_hour = 12;
    if (ct->tm_hour > 12)
        ct->tm_hour -= 12;
    if (ct->tm_hour <= 10)
        CallVoice(V_1 + ct->tm_hour - 1, 0);
    else
        CallVoice(V_11 + ct->tm_hour - 11, 0);
    if ((a = ct->tm_min) == 0)
        CallVoice(V_OCLOCK, 1);
    else {
        a /= 10;
        if (a > 0) {
            if (a == 1) {
                ct->tm_min -= 10;
                CallVoice(V_10 + ct->tm_min, 1);
            }
            else {
                ct->tm_min -= (a * 10);
                if (ct->tm_min > 0) {
                    CallVoice(V_20 + a - 2, 0);
                    CallVoice(V_1 + ct->tm_min - 1, 1);
                }
                else
                    CallVoice(V_20 + a - 2, 1);
            }
        }
        else {
            CallVoice(V_0, 0);
            CallVoice(V_1 + ct->tm_min - 1, 1);
        }
    }
}

void
CheckTime(void)
{
    time_t t0;
    struct tm *ct;

    t0 = time(0);
    ct = localtime(&t0);
    if (((unsigned char) (ct->tm_hour) == PrevH) && ((unsigned char) (ct->tm_min) == PrevM))
        return;
    PrevM = (unsigned char) (ct->tm_min);
    if (PrevM % 10)
        return;
    SayTime();
}

#endif


void
UpdateFuelFile(void)
{
    FILE *fp = NULL;
    float MPG, AMPG;

    if ((fp = fopen(fuel_file_name, "a")) == NULL)
        return;

    AMPG = TripGallonValue;
    if (AccRpm != 0.0f) {
        if (AccSpd > 0.0f)
            MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
        else {
            if (AccSpdCntr > 0)
                MPG = (float) (((AccSpdCntr * SPDSCALER_MILE) / AccRpm));
            else
                MPG = (float) (((SPDSCALER_MILE) / AccRpm));
        }
        AMPG += (float) (((AccSpd * (float) TimeElapsed) / ((float) AccSpdCntr * MPG)));        // must be divided by 5760 for real number later ( preserving accuracy )
    }

    if (RefuelValue > 0.0f)
        fprintf(fp, "%d\t%1.4f\t%d\t%3.2f\t%3.2f\t%2.3f\n", GasGauge, (AMPG / 5760.0f),
                FirstGasReading, ((TripKilometers + DistanceTravelled) * 0.625f),
                ((MeasurementKilometers + DistanceTravelled) * 0.625f), RefuelValue);
    else
        fprintf(fp, "%d\t%1.4f\t%d\t%3.2f\t%3.2f\n", GasGauge, (AMPG / 5760.0f), FirstGasReading,
                ((TripKilometers + DistanceTravelled) * 0.625f),
                ((MeasurementKilometers + DistanceTravelled) * 0.625f));

    RefuelValue = 0.0f;
    fflush(fp);
    fclose(fp);
}




#define	RED_LUMA	1       // 2            // 0.212671f
#define	GREEN_LUMA	3       // 8            // 0.715160f
#define	BLUE_LUMA	0       // 1            // 0.072169f
#define	THRESHOLD	1800    // 2805

void
TransferFont(int fsx, int fsy, int tx, int ty, int fw, int fh, int zoom)
{
    register unsigned char *FRPtr;      // current row's string ptr
    register int ch;
    register int cr = fsy, tr = fsy + fh;       // current / target row
    register int cc, sc = fsx, tc = fsx + fw;   // current / start / target column
    register int sz1, sz2;      // colour pointer
    register int WBPtr, WBpy = ty;      // Pointer to workbuffer
    register unsigned char Rv, Gv, Bv;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'F';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif


#if 0
    if (WBpy < 0)               // don't copy if out of screen (y<0)
    {
        printf("\nty<0 for %d:%d fsx=%d fsy=%d wi=%d he=%d zoom=%d", tx, ty, fsx, fsy, fw, fh,
               zoom);
        fflush(stdout);
        cr -= WBpy;
        WBpy = 0;
    }
    if (tx < 0) {
        printf("\ntx<0 for %d:%d fsx=%d fsy=%d wi=%d he=%d zoom=%d", tx, ty, fsx, fsy, fw, fh,
               zoom);
        fflush(stdout);
        sc = fsx - tx;
        tx = 0;
    }
    if ((WBpy + (fh * zoom)) >= HEIGHT) // don't copy if out of screen (y>width)
    {
        printf("\nty+h>480 for %d:%d fsx=%d fsy=%d wi=%d he=%d zoom=%d", tx, ty, fsx, fsy, fw, fh,
               zoom);
        fflush(stdout);
        tr = fsy + (HEIGHT - WBpy);
    }
    if ((tx + (fw * zoom)) >= WIDTH) {
        printf("\ntx+w>640 for %d:%d fsx=%d fsy=%d wi=%d he=%d zoom=%d", tx, ty, fsx, fsy, fw, fh,
               zoom);
        fflush(stdout);
        tc -= (tx + (fw * zoom) - WIDTH);
    }
#endif

    while (cr < tr)             // raw...
    {
        FRPtr = Font_Map[cr];   // ptr to string within font map...
        for (cc = sc; cc < tc; cc++)    // column
        {
            ch = (int) (FRPtr[cc]);     // This now contains the intensity value
            for (sz2 = 0; sz2 < zoom; sz2++) {
                WBPtr = (tx + ((cc - sc) * zoom) + (WBpy + sz2) * WIDTH);
#ifdef TRACE_IT
                if (((tx + ((cc - sc) * zoom)) > 639) || (WBpy + sz2) > 479) {
                    printf("\nError in PutFont : %d:%d", (tx + ((cc - sc) * zoom)), (WBpy + sz2));
                    fflush(stdout);
                }
#endif
                for (sz1 = 0; sz1 < zoom; sz1++) {
                    if (WBPtr >= 0) {
#ifdef FIXED_FONT_COLOR
                        Rv = (unsigned char) ImageBuffer[WBPtr].R;
                        Gv = (unsigned char) ImageBuffer[WBPtr].G;
                        Bv = (unsigned char) ImageBuffer[WBPtr].B;
                        if (((Rv << 1) + (Gv << 3) + Bv) < THRESHOLD)   // DARK BACKGROUND
                        {
                            Rv = (unsigned char) ((((int) Rv * (255 - ch)) + ((int) 255 * ch)) >>
                                                  8);
                            Gv = (unsigned char) ((((int) Gv * (255 - ch)) + ((int) 255 * ch)) >>
                                                  8);
                            Bv = (unsigned char) ((((int) Bv * (255 - ch)) + ((int) 255 * ch)) >>
                                                  8);
                        }
                        else {
                            Rv = (unsigned char) (((int) Rv * (255 - ch)) >> 8);
                            Gv = (unsigned char) (((int) Gv * (255 - ch)) >> 8);
                            Bv = (unsigned char) (((int) Bv * (255 - ch)) >> 8);
                        }
#else
                        Rv = (unsigned char) ABS((int) ImageBuffer[WBPtr].R - ch);
                        Gv = (unsigned char) ABS((int) ImageBuffer[WBPtr].G - ch);
                        Bv = (unsigned char) ABS((int) ImageBuffer[WBPtr].B - ch);
#endif
                        ImageBuffer[WBPtr].R = Rv;
                        ImageBuffer[WBPtr].G = Gv;
                        ImageBuffer[WBPtr].B = Bv;
                    }
                    ++WBPtr;
                }
            }
        }
        WBpy += zoom;
        cr++;
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'f';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

}

void
PutMyString(char *Text, int x, int y, int usebignums, int zoom)
{
    int a = 0, b, px = x, notfound;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'S';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

//printf("\nPutString : '%s' to %dx%d ( %d %d)",Text,x,y,usebignums,zoom);fflush(stdout);

    while (Text[a] != '\0') {
        notfound = 1;
        if (usebignums) {
            b = 0;
            while ((bignumbers[b].letter != Text[a]) && (bignumbers[b].x >= 0))
                ++b;
            if (bignumbers[b].x >= 0) {
                if (zoom == 3)
                    TransferFont(bignumbers[b].x, bignumbers[b].y, px, y - 2, bignumbers[b].width,
                                 bignumbers[b].height, zoom);
                else
                    TransferFont(bignumbers[b].x, bignumbers[b].y, px, y + 1, bignumbers[b].width,
                                 bignumbers[b].height, zoom);
                px += bignumbers[b].width * zoom;
                notfound = 0;
            }
        }
        if (notfound) {
            b = 0;
            while ((fonts[b].letter != Text[a]) && (fonts[b].x >= 0))
                ++b;
            if (fonts[b].x >= 0) {
                if ((fonts[b].letter >= '0') && (fonts[b].letter <= '9'))
                    TransferFont(fonts[b].x, fonts[b].y, px, y - 1, fonts[b].width, fonts[b].height,
                                 zoom);
                else {
                    if (fonts[b].letter == '%') {
                        if (!usebignums)
                            TransferFont(fonts[b].x, fonts[b].y, px, y - 2, fonts[b].width,
                                         fonts[b].height, zoom);
                        else
                            TransferFont(fonts[b].x, fonts[b].y, px, y + 4, fonts[b].width,
                                         fonts[b].height, zoom);
                    }
                    else
                        TransferFont(fonts[b].x, fonts[b].y, px, y + (usebignums * 9),
                                     fonts[b].width, fonts[b].height, zoom);
                }
                px += fonts[b].width * zoom;
            }
        }
        ++a;
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 's';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}

int
GetMyStringLength(char *Text, int usebignums, int zoom)
{
    int a = 0, b, px = 0, notfound;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'N';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    while (Text[a] != '\0') {
        notfound = 1;
        if (usebignums) {
            b = 0;
            while ((bignumbers[b].letter != Text[a]) && (bignumbers[b].x >= 0))
                ++b;
            if (bignumbers[b].x >= 0) {
                px += bignumbers[b].width * zoom;
                notfound = 0;
            }
        }
        if (notfound) {
            b = 0;
            while ((fonts[b].letter != Text[a]) && (fonts[b].x >= 0))
                ++b;
            if (fonts[b].x >= 0)
                px += fonts[b].width * zoom;
        }
        ++a;
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'n';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    return (px);
}


void
MarkDoors(unsigned char Door)
{
    register int a, b, c, xofs, yofs, ptr, xs = 0, xe;
    unsigned char hp;
    unsigned char *RedBuffer;

    switch (Door) {
    case 1:                    // Driver
        RedBuffer = SideFrontDoor;
        xofs = 100;
        yofs = 0;
        hp = 1;
        break;
    case 2:                    // Passenger
        RedBuffer = SideFrontDoor;
        xofs = 100;
        yofs = 0;
        hp = 2;
        break;
    case 3:                    // Rear
        RedBuffer = RearDoors;
        xofs = 0;
        yofs = 22;
        hp = 3;
        break;
    default:
        return;
        break;
    }

    ptr = 0;
    xe = -1;
    for (b = 0; b < 108; b++)   // 480-216=264 => 132   
    {
        c = 101 + ((b + 132) * WIDTH);
        if ((hp & 1) && (ptr >= 0) && (b >= yofs)) {
            if (RedBuffer[ptr] == 0) {
                ptr = -1;
                xe = -1;
            }
            else {
                xs = xofs + (int) RedBuffer[ptr++];
                xe = xofs + (int) RedBuffer[ptr++];
            }
        }
        for (a = 0; a < 439; a++) {
            if ((a >= xs) && (a <= xe)) {
                ImageBuffer[c].G = 0;
                ImageBuffer[c].B = 0;
            }
            ++c;
        }
    }

    ptr = 0;
    xe = -1;
    for (b = 0; b < 108; b++)   // 480-216=264 => 132   
    {
        c = 101 + (((107 - b) + 240) * WIDTH);
        if ((hp & 2) && (ptr >= 0) && (b >= yofs)) {
            if (RedBuffer[ptr] == 0) {
                ptr = -1;
                xe = -1;
            }
            else {
                xs = xofs + (int) RedBuffer[ptr++];
                xe = xofs + (int) RedBuffer[ptr++];
            }
        }
        for (a = 0; a < 439; a++) {
            if ((a >= xs) && (a <= xe)) {
                ImageBuffer[c].G = 0;
                ImageBuffer[c].B = 0;
            }
            ++c;
        }
    }
}

void
ShowPrius(void)                 // DoorStatus = 0x00 - All Closed    0x04 - Rear    0x40 - Passenger   0x80 - Driver
{
    unsigned char cv;
    register int a, b, c;

    for (b = 0; b < HEIGHT; b++) {
        c = b * WIDTH;
        for (a = 0; a < WIDTH; a++) {
            ImageBuffer[c].R = 0;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }

    for (b = 0; b < 108; b++)   // 480-216=264 => 132   
    {
        c = 101 + ((b + 132) * WIDTH);
        for (a = 0; a < 439; a++) {
            cv = PriusPicture[b][a];
            ImageBuffer[c].R = cv;
            ImageBuffer[c].G = cv;
            ImageBuffer[c].B = cv;
            ++c;
        }
    }

    for (b = 0; b < 108; b++)   // 480-216=264 => 132   
    {
        c = 101 + (((107 - b) + 240) * WIDTH);
        for (a = 0; a < 439; a++) {
            cv = PriusPicture[b][a];
            ImageBuffer[c].R = cv;
            ImageBuffer[c].G = cv;
            ImageBuffer[c].B = cv;
            ++c;
        }
    }

    if (DoorStatus & 0x80)
        MarkDoors(1);
    if (DoorStatus & 0x40)
        MarkDoors(2);
    if (DoorStatus & 0x4)
        MarkDoors(3);

    UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
}

void
ClearDisplayBufferArea(int x, int y, int w, int h)
{
    register int a, b, c;

    for (b = y; b < (y + h); b++) {
        c = b * WIDTH + x;
        for (a = 0; a < w; a++) {
            ImageBuffer[c].R = BKG_R;
            ImageBuffer[c].G = BKG_G;
            ImageBuffer[c].B = BKG_B;
            ++c;
        }
    }
}

void
ClearDisplayBuffer(void)
{
    register int a, b, c;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'T';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    for (b = 0; b < HEIGHT; b++) {
        c = b * WIDTH;
        for (a = 0; a < WIDTH; a++) {
            ImageBuffer[c].R = BKG_R;
            ImageBuffer[c].G = BKG_G;
            ImageBuffer[c].B = BKG_B;
            ++c;
        }
    }

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 't';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

}

void
SetUpPicture(void)
{
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'Y';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    ClearDisplayBuffer();
    UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'y';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

}

void
SetUpMySignals()
{
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'I';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

/*
int	a;
	for(a=1;a<31;a++) signal(a,IntReXSigCatch);
return;
*/

    signal(SIGPIPE, IntReXSigCatch);
    signal(SIGSEGV, IntReXSigCatch);
    signal(SIGBUS, IntReXSigCatch);
    signal(SIGFPE, IntReXSigCatch);
    signal(SIGILL, IntReXSigCatch);
    signal(SIGPWR, IntReXSigCatch);
    signal(SIGHUP, IntReXSigCatch);
    signal(SIGINT, IntReXSigCatch);
    signal(SIGQUIT, IntReXSigCatch);
    signal(SIGTERM, IntReXSigCatch);
    signal(SIGSTOP, IntReXSigCatch);
    signal(SIGCONT, IntReXSigCatch);
    signal(SIGTTIN, IntReXSigCatch);
    signal(SIGIO, IntReXSigCatch);
    signal(SIGALRM, IntReXSigCatch);

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'i';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

}


void
ProcessSigIo()
{
    struct timeval to;
    fd_set fdset;
    int c, x, y;
    char BUFF[8];

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'P';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

#ifdef USE_KEYBOARD
    FD_ZERO(&fdset);
    FD_SET(0, &fdset);
    to.tv_sec = 0;
    to.tv_usec = 0;
    if (select(FD_SETSIZE, &fdset, 0, 0, &to) != -1) {
        if (FD_ISSET(0, &fdset)) {
            c = getchar();
            switch (c) {
            case 0xa:          // ReStart
                if (Sleeping) {
                    ProcessGo = 0;
                    ContSignal = 1;
                    Sleeping = 0;
                    PriusIsPowered = 1;
                }
                else {
                    alarm(0);
                    SaveStat();
                    Sleeping = 1;
                    if (Port >= 0) {
                        sprintf(Message, "C\015");      // Close the CAN channel
                        if (WriteToPort(Message)) {
                            printf("\nError writing to port : Close CAN Channel");
                            fflush(stdout);
                        }
                        close(Port);
                        Port = -1;
                    }

                }
                break;
            case 'q':
                ProcessGo = 0;
                RealQuit = 1;
                break;

            case 's':
                if (++SI_Measurements > 1)
                    SI_Measurements = 0;
                break;

            case 'p':
                PriusIsPowered = 0;
                Sleeping = 0;
                CurrentSpeed = 0;
                break;

            case 0x7F:         // BS
                if (++VoiceMode > 1)
                    VoiceMode = 0;
                break;

            case ' ':
                NeedSynced = 1;
                FSRCntr = 1;
                break;

            }
        }
    }
#endif

    return;


#ifdef USE_TOUCHSCREEN
    FD_ZERO(&fdset);
    FD_SET(touch_screen_fd, &fdset);
    to.tv_sec = 1;
    to.tv_usec = 0;
    if (!ScrTouched)
        ScrTouched = 1;
    if ((c = select((touch_screen_fd + 1), &fdset, NULL, NULL, &to))) {
        if (!ScrTouched)
            ScrTouched = 2;
        if (FD_ISSET(touch_screen_fd, &fdset)) {
            if ((c = read(touch_screen_fd, BUFF, 8)) == 8) {
                ScrTouched = 3;
//                              UICopyDisplayBufferToScreen(0,0,WIDTH,HEIGHT);

                x = ((int) (BUFF[5] * 256) + (int) (BUFF[4]));
                y = (480 - ((int) (BUFF[3] * 256) + (int) (BUFF[2])));  // 45:45  560:45  45:445   560:445
                TouchedX = x;
                TouchedY = y;

                if (RunningTask != TASK_INFO) {
                    if ((x -= 45) < 0)
                        x = 0;
                    if ((x >>= 7) > 3)
                        x = 3;  // Map to 4 quadrants   // x= 0 ~ 3
                    if ((y > 100) && (y < 400)) {
                        switch (x) {
                        case 0:
                            if (++VoiceMode > 1)
                                VoiceMode = 0;
                            break;
                        case 1:
                        case 2:
                            switch (RunningTask) {
                            case TASK_INIT:
                                RunningTask = TASK_INFO;
                                break;
                            case TASK_SCREENSAVER:     // Signal to save to memcard
                                TouchedX = 17;
                                TouchedY = 17;
                                break;
                            case TASK_RUNNING:
                                if (++SI_Measurements > 1)
                                    SI_Measurements = 0;
                                break;
                            }
                            break;
                        case 3:
                            if (RunningTask < TASK_RUNNING) {
                                ProcessGo = 0;
                                RealQuit = 1;
                            }
                            else {
                                PriusIsPowered = 0;
                                Sleeping = 0;
                                CurrentSpeed = 0;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
#endif

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'p';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

}

void
IntReXSigCatch(int sig)
{
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'X';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

//printf("\nSig:%d\n",sig);fflush(stdout);


    switch (sig) {
    case SIGIO:
        ProcessSigIo();
        break;
    case SIGALRM:
        NeedSynced = 1;
        signal(SIGALRM, IntReXSigCatch);
        if ((ProcessGo) && (RealQuit == 0) && (Sleeping == 0))
            alarm(1);
        break;
    case SIGQUIT:
        ProcessGo = 0;
        break;
    case SIGCONT:
        ProcessGo = 0;          // When Power was restored...
        ContSignal = 1;
        Sleeping = 0;
        PriusIsPowered = 1;
        break;
    case SIGSTOP:
    case SIGTTIN:
    case SIGPWR:
    case SIGILL:
    case SIGFPE:
    case SIGBUS:
    case SIGSEGV:
    case SIGPIPE:
    case SIGINT:
    case SIGHUP:
    case SIGTERM:
        printf("\nMisc Signal Cought, exiting (%d)", sig);
        fflush(stdout);
        CleanUp(0);
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'x';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        exit(-1);
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'x';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

}

#define	BAR_WIDTH	180
#define	BAR_SPACING	17
#define	CURRENT_XS	BAR_SPACING
#define	SOC_XS		((BAR_WIDTH+CURRENT_XS)+(BAR_SPACING<<1))       // ( 180 + 17 ) + 34 = 231
#define	RPM_XS		((BAR_WIDTH+SOC_XS)+(BAR_SPACING<<1))   // 180 + 231 + 34  = 445
#define	RPM_YS		260     // 220 180
#define	RPM_XE		(BAR_WIDTH+RPM_XS)
#define	RPM_YE		(HEIGHT-30)
#define	TEMP_YE		200     // 160

#define	I_BKG_R	((unsigned char)0x10)
#define	I_BKG_G	((unsigned char)0x20)
#define	I_BKG_B	((unsigned char)0x30)

#ifdef TRAFFIC_PROFILE
#define	SOC_YS	((HEIGHT>>1)+10)
#else
#define	SOC_YS	(HEIGHT>>1)     // 240
#endif
#define	SOC_XE	(BAR_WIDTH+SOC_XS)
#define	SOC_YE	(HEIGHT-30)
#define	SOC_UPPER_LIMIT	140
#define	SOC_LOWER_LIMIT	90

#define	GG_XS	SOC_XS
#define	GG_YS	50
#define	GG_XE	SOC_XE
#define	GG_YE	((HEIGHT>>1)-20)        // 220
#define	GG_UPPER_LIMIT	40

void
AnalyseHighCANMessages(int mid, int Position)
{
    register unsigned char CV;
    register unsigned int B;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = '-';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    CV = MineChar(Position + 0);
    if (CV != 0x04)
        return;
    CV = MineChar(Position + 2);
    if (CV != 0x61)
        return;
    CV = MineChar(Position + 4);
    if ((CV == 0x3C) || (CV == 0x3E))   //  CAT1 || CAT2
    {
        B = MineUInt(Position + 6);
        B = (unsigned int) ((float) B / 10.0f);
        if (CV == 0x3C)
            Cat1Temp = (int) B - 40;
        else
            Cat2Temp = (int) B - 40;
        UpdateCatTemp();
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = '+';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
    TrBu[TrBuPtr] = '1';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}

void
UpdateSpeedComputations(void)
{
    register int a, b, c;
    float MPG, ThrottleV, SpeedV;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'H';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    if (ContSignal)
        return;                 // time has no meaning...

    TimeElapsed = (unsigned int) difftime(time(NULL), StartTime);

//printf("\nTH : %x",V);fflush(stdout);

    if (ThrottleAccumulatedCntr == 0)
        ThrottleV = 0.0f;
    else
        ThrottleV = (float) ((float) ThrottleAccumulated / (float) ThrottleAccumulatedCntr);

    if (SpeedAccumulatedCntr == 0)
        SpeedV = 0.0f;
    else
        SpeedV = (float) ((float) SpeedAccumulated * 1.0075f / (float) SpeedAccumulatedCntr);

    CurrentSpeed = SpeedV;
    ThrottleAccumulatedCntr = 0;
    ThrottleAccumulated = 0;
    SpeedAccumulatedCntr = 0;
    SpeedAccumulated = 0;

#define	XOFS	105
    for (b = 0; b < 46; b++) {
        c = b * WIDTH;
        for (a = 0; a < (WIDTH - 6); a++) {
            ImageBuffer[c].R = 0;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }

    if (SI_Measurements)
        sprintf(Message, "%1.0f km/h", SpeedV);
    else
        sprintf(Message, "%1.1f MPH", (SpeedV * 0.625f));
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, ((XOFS + 208) - c), 9, 1, 3);

    if (ThrottleV != 0) {

//              MPG=(float)((SpeedV*0.625f*SpeedScalerV)/(float)FuelRead);
        MPG = (float) ((SpeedV * (float) SPDSCALER_MILE) / ThrottleV);
        if (MPG < 99.99f) {
            if (SI_Measurements)
                sprintf(Message, "%2.1f km/l", (MPG * 0.42328f));       // Current km/l
            else
                sprintf(Message, "%2.1f MPG", MPG);     // Current MPG
            c = GetMyStringLength(Message, 1, 3);
            PutMyString(Message, (XOFS + 406 - c), 9, 1, 3);
        }
        else {
            if (MPG > 99999.9f)
                MPG = 99999.9f;
            if (SI_Measurements)
                sprintf(Message, "%2.1f km/l", (MPG * 0.42328f));
            else
                sprintf(Message, "%2.1f MPG", MPG);
            c = GetMyStringLength(Message, 1, 2);
            PutMyString(Message, (XOFS + 400 - c), 17, 1, 2);
        }
        AccRpm += (double) ThrottleV;
    }

    sprintf(Message, "%3.1f Kbps", ((float) NofTrafficBytes / 128.0f)); // byte*8/1024
    c = GetMyStringLength(Message, 0, 2);
    ClearDisplayBufferArea(RPM_XS, 456, BAR_WIDTH, 25);
    PutMyString(Message, (((RPM_XS + (BAR_WIDTH >> 1)) - (c >> 1)) - 20), 457, 0, 2);
    UICopyDisplayBufferToScreen(RPM_XS, 456, BAR_WIDTH, 25);
    NofTrafficBytes = 0;

#ifdef MORE_DATA
    sprintf(Message, "%d", Info_Brakes);
    c = GetMyStringLength(Message, 0, 2);
    ClearDisplayBufferArea(RPM_XS, TEMP_YE + 1, BAR_WIDTH, 18);
    PutMyString(Message, ((RPM_XS + (BAR_WIDTH >> 1)) - (c >> 1)), (TEMP_YE + 2), 0, 2);
    UICopyDisplayBufferToScreen(RPM_XS, TEMP_YE, BAR_WIDTH, 20);
#endif

    if (Decel) {
        for (b = 10; b < 34; b++) {
            c = b * WIDTH + XOFS + 18;
            for (a = 0; a < 10; a++) {
                ImageBuffer[c].R = 0;
                ImageBuffer[c].G = 255;
                ImageBuffer[c].B = 0;
                ++c;
            }
        }
    }
    else {
        for (b = 10; b < 34; b++) {
            c = b * WIDTH + XOFS + 18;
            for (a = 0; a < 10; a++) {
                ImageBuffer[c].R = 255;
                ImageBuffer[c].G = 0;
                ImageBuffer[c].B = 0;
                ++c;
            }
        }
    }


    if (AccRpm > 0.0f)
        MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
    else
        MPG = 0.0f;

    if (++ValueSwitch > 1)
        ValueSwitch = 0;
    switch (ValueSwitch) {
    case 0:
        if (SI_Measurements)
            sprintf(Message, "%2.1f km/l", (MPG * 0.42328f));   // Accumulated km/l on this trip
        else
            sprintf(Message, "%2.1f MPG", MPG); // Accumulated MPG on this trip
        break;
    case 1:
        if ((MPG > 0.0f) && (AccSpdCntr > 0))
            ThrottleV = ((AccSpd * (float) TimeElapsed) / ((float) AccSpdCntr * 5760.0f * MPG));        // This many gallons were used on this trip
        else
            ThrottleV = 0.0f;
        if (SI_Measurements)
            sprintf(Message, "%1.3f Ltr", (ThrottleV * 3.78f));
        else
            sprintf(Message, "%1.3f Gal", ThrottleV);
        break;
    }
    c = GetMyStringLength(Message, 0, 2);
    PutMyString(Message, (WIDTH - 8 - c), 26, 0, 2);

    for (b = 456; b < 480; b++) {
        c = 231 + b * WIDTH;
        for (a = 0; a < 180; a++) {
            ImageBuffer[c].R = 0;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }

    ThrottleV =
        ((MeasuredKMPG * 0.625f * MeasurementKilometers) +
         (MPG * DistanceTravelled)) / (MeasurementKilometers + DistanceTravelled);
    if (SI_Measurements)
        sprintf(Message, "%d km=%2.1f km/l",
                (int) ((MeasurementKilometers + DistanceTravelled) + 0.5f), (ThrottleV * 0.42328f));
    else
        sprintf(Message, "%d M=%2.1f W",
                (int) (((MeasurementKilometers + DistanceTravelled) * 0.625f) + 0.5f), ThrottleV);
    c = GetMyStringLength(Message, 0, 2);       // 321 is the center
    PutMyString(Message, (321 - (c >> 1)), 457, 0, 2);  // All MPG and Miles

    ThrottleV = ((float) AccSpd / (float) AccSpdCntr) * 0.625f; // Average Trip Speed
    if (SI_Measurements)
        sprintf(Message, "%2.1f km/h", (ThrottleV * 1.6f));
    else
        sprintf(Message, "%2.1f MPH", ThrottleV);
    c = GetMyStringLength(Message, 0, 2);
    PutMyString(Message, (WIDTH - 10 - c), 2, 0, 2);

    a = (int) ((float) TimeElapsed / 60.0f);    // minute
    b = TimeElapsed - (a * 60); // seconds
    if (a >= 60) {
        c = (int) ((float) a / 60.0f);  // hour
        a -= (c * 60);
        sprintf(Message, "%1d:%02d:%02d", c, a, b);
    }
    else
        sprintf(Message, "%02d:%02d", a, b);

    if ((c = GetMyStringLength(Message, 0, 2)) > 118)
        c = 118;
    PutMyString(Message, (118 - c), 2, 0, 2);

    AccSpd += SpeedV;
    ++AccSpdCntr;

    MPG = (float) (AccSpd * (double) TimeElapsed) / (double) AccSpdCntr;
    DistanceTravelled = MPG / 3600.0f;  // Since we computed /s updates -> /h

    if (SI_Measurements)
        sprintf(Message, "%1.1f km", DistanceTravelled);
    else
        sprintf(Message, "%1.1f Miles", (DistanceTravelled * 0.625f));

    if ((c = GetMyStringLength(Message, 0, 2)) > 118)
        c = 118;
    PutMyString(Message, (118 - c), 26, 0, 2);

    UICopyDisplayBufferToScreen(0, 0, (WIDTH - 6), 46);
    UICopyDisplayBufferToScreen(231, 456, 180, 21);

    ThrottleV = TripGallonValue;
    if (AccRpm > 0.0f) {
        if (AccSpd > 0.0f)
            MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
        else {
            if (AccSpdCntr > 0)
                MPG = (float) (((AccSpdCntr * SPDSCALER_MILE) / AccRpm));
            else
                MPG = (float) (((SPDSCALER_MILE) / AccRpm));
        }
        if (AccSpdCntr > 0) {
            if (AccSpd > 0.0f)
                ThrottleV +=
                    (float) (((AccSpd * (float) TimeElapsed) / ((float) AccSpdCntr * MPG)));
            else
                ThrottleV += (float) ((((float) TimeElapsed) / ((float) AccSpdCntr * MPG)));
        }
    }
    TripGal = ThrottleV / 5760.0f;      // gallons consumed since last refill
    TripMile = (TripKilometers + DistanceTravelled) * 0.625f;   // miles from last refill

    UpdateGG();

    if (ProcessGo > 1)
        ProcessGo = 0;          // To guarantee accurate distance readings...

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'h';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}

void
StopCurrentCollection(char src)
{
    unsigned int EndTime;
    struct timeval tv;
    float fv;

    LastRegenStopReason = src;
    if (CollCntr != 0) {
        if (StrtSpd > PrvSpd) {
            gettimeofday(&tv, NULL);
            EndTime =
                (unsigned int) ((time_t) (tv.tv_sec) - StartTime) * (unsigned int) 100 +
                (unsigned int) ((float) (tv.tv_usec) / 10000.0f);
            EndTime -= CollTime;        // gives us 1/100 second accuracy
            if (EndTime > 100)  // less than a second should not be measured
            {
                fv = ((((float) CollCurr / (float) CollCntr) * 0.1f) * ((float) CollVolt / (float) CollCntr)) * 0.001f; // average kW
                fv *= ((float) EndTime * 0.01f);
                LastRegenkW = fv;
                MinMaxkWDisplay = 60;   // 60 seconds
                LastRegenTime = (unsigned char) ((float) EndTime * 0.01f);
                LastRegenSpdEnd = PrvSpd;
                LastRegenSpdStart = StrtSpd;
#ifdef MORE_DATA
                if (Info_Brakes_Cnt > 0) {
                    LastRegenBrake = (unsigned char) (Info_Brakes / Info_Brakes_Cnt);
                }
                else
                    LastRegenBrake = 0;
#endif
            }
        }
    }
    CollectCurrent = 0;
    CollCntr = 0;
    CollCurr = 0;
    CollVolt = 0;
#ifdef MORE_DATA
    Info_Brakes = 0;
    Info_Brakes_Cnt = 0;
#endif
}

//#define       BAR_WIDTH       180
//#define       BAR_SPACING     17

//#define       CURRENT_XS      BAR_SPACING
#define	CURRENT_YS	((HEIGHT>>1)-80)
#define	CURRENT_XE	(BAR_WIDTH+CURRENT_XS)
#define	CURRENT_YE	(HEIGHT-12)
#define	UPPER_LIMIT		0x535
#define	ONE_PER_UPPER_LIMIT	0.00075f
#define	LOWER_LIMIT		UPPER_LIMIT
#define	ONE_PER_LOWER_LIMIT	ONE_PER_UPPER_LIMIT
//#define       LOWER_LIMIT             0x380
//#define       ONE_PER_LOWER_LIMIT     0.001116f

#define		CURRENT_FREQ_UPDATE	20
#define		ONE_PER_QFU		0.05f
#define		ICE_POWERED_TOLERANCE	4
unsigned char CurrentFreqCntr = CURRENT_FREQ_UPDATE;
long AccumulatedCurrent = 0;
long AccumulatedHB_Level = 0;   // Voltage
int PreRegVal = 0;

void
UpdateCurrent(void)             // 31/s
{
    register int x, y, ty, c, ty1, Value, PrintValue, RVal, RegVal;
    float HB_Level, w1, w2;
    unsigned char cq;

//printf("\nUC : %x",V);fflush(stdout);

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'E';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    Value = (int) ((float) AccumulatedCurrent * ONE_PER_QFU);
    HB_Level = (float) AccumulatedHB_Level *ONE_PER_QFU;
    CurrentFreqCntr = CURRENT_FREQ_UPDATE;
    AccumulatedCurrent = 0;
    AccumulatedHB_Level = 0;

    if ((ICEPoweredCurrStage < ICE_POWERED_TOLERANCE) && (Value < 0) && (SpeedCurrStageCntr > 0))       // For bookkeeping purposes...
    {
        x = (int) ((float) SpeedCurrStage / ((float) SpeedCurrStageCntr * 2.0f));       // Speed for Every 2nd km/h
        PrintValue = -Value;    // 12bit Signum conversion   *** REGEN ***
        if (PrintValue < 1800)  // 180A average should be way enough
        {
            if (x < 50)         // Speed / 2
            {
                if (SOCValueCurrStage == 0)
                    SOCValueCurrStage = SOCValue;
                y = (int) ((float) (SOCValueCurrStage >> 1) / 5.0f);    // real % to 76% => 7   SOCValueCurrStage is *2
                if ((y -= 8) < 0)
                    y = 0;      // 40=0  80=8
                if (y > 8)
                    y = 8;      // -> 80%
                cq = (unsigned int) ((float) PrintValue * 0.1f);        // Real Amperage
                if (cq > (unsigned int) MaxCurrentValues[x][y])
                    MaxCurrentValues[x][y] = (unsigned char) cq;
            }
        }
    }

//printf(" => %x",V);fflush(stdout);

    if (DoorStatus)
        return;

    RegVal = 0;

    if (Value > 0)              // *** DISCHARGE ***
    {
        PrintValue = Value;

        if ((w1 = (float) BattMaxDisChargeCurrent * (float) BattVoltageValue) < 1.0f)
            w1 = 1.0f;
        w2 = ((float) PrintValue * 0.1f) * HB_Level;
        RVal = (int) ((w2 * 100.0f) / w1);

        if (Value > UPPER_LIMIT)
            Value = UPPER_LIMIT;
        ty = (int) (((float) Value * (float) (CURRENT_YE - CURRENT_YS)) * ONE_PER_UPPER_LIMIT); // How high the bar is
        ty1 = (CURRENT_YE - CURRENT_YS) - ty + CURRENT_YS;
        for (y = CURRENT_YS; y < ty1; y++) {
            c = CURRENT_XS + y * WIDTH;
            for (x = CURRENT_XS; x < CURRENT_XE; x++) {
                ImageBuffer[c].R = I_BKG_R;
                ImageBuffer[c].G = I_BKG_G;
                ImageBuffer[c].B = I_BKG_B;
                ++c;
            }
        }
        for (y = ty1; y < CURRENT_YE; y++) {
            c = CURRENT_XS + y * WIDTH;
            for (x = CURRENT_XS; x < CURRENT_XE; x++) {
                ImageBuffer[c].R = 255;
                ImageBuffer[c].G = 20;
                ImageBuffer[c].B = 20;
                ++c;
            }
        }
    }
    else {
        Value = -Value;         // 12bit Signum conversion   *** REGEN ***
        PrintValue = Value;

// Use empirical values for checking recharge capacity if speed is less than 100km/h
        ty1 = ((int) PrvSpd) >> 1;      // Every 2nd km/h
        if ((ty1 < 50) && (ICEPoweredCurrStage < ICE_POWERED_TOLERANCE)) {
            ty = (int) ((float) (SOCValue >> 1) / 5.0f);        // real % to 76% => 7
            if ((ty -= 8) < 0)
                ty = 0;
            if (ty > 8)
                ty = 8;
            w2 = (float) MaxCurrentValues[ty1][ty];
            if (w2 > 0.0f)
                RegVal = (int) (((float) PrintValue * 10.0f) / w2);     // For this is expressed in % and PrintValue is already *10
        }

        if ((w1 = (float) BattMaxChargeCurrent * (float) BattVoltageValue) < 1.0f)
            w1 = 1.0f;
        w2 = ((float) PrintValue * 0.1f) * HB_Level;
        RVal = (int) ((w2 * 100.0f) / w1);

        if (Value > LOWER_LIMIT)
            Value = LOWER_LIMIT;
        ty = (int) (((float) Value * (float) (CURRENT_YE - CURRENT_YS)) * ONE_PER_LOWER_LIMIT); // How high the bar is
        ty1 = (CURRENT_YE - CURRENT_YS) - ty + CURRENT_YS;
        for (y = CURRENT_YS; y < ty1; y++) {
            c = CURRENT_XS + y * WIDTH;
            for (x = CURRENT_XS; x < CURRENT_XE; x++) {
                ImageBuffer[c].R = I_BKG_R;
                ImageBuffer[c].G = I_BKG_G;
                ImageBuffer[c].B = I_BKG_B;
                ++c;
            }
        }
        for (y = ty1; y < CURRENT_YE; y++) {
            c = CURRENT_XS + y * WIDTH;
            for (x = CURRENT_XS; x < CURRENT_XE; x++) {
                ImageBuffer[c].R = 20;
                ImageBuffer[c].G = 255;
                ImageBuffer[c].B = 150;
                ++c;
            }
        }
    }

    if (RVal < 0)
        RVal = 0;
    else if (RVal > 1000)
        RVal = 1000;
    sprintf(Message, "%d%%", RVal);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (CURRENT_XS + ((BAR_WIDTH - c) >> 1)), (CURRENT_YS + 2), 0, 3);

    if (PreRegVal != RegVal) {
        PreRegVal = RegVal;
        PutExtraInfo();
    }

    sprintf(Message, "%1.1fA", ((float) PrintValue * 0.1f));
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (CURRENT_XS + ((BAR_WIDTH - c) >> 1)),
                (CURRENT_YS + ((CURRENT_YE - CURRENT_YS) >> 1)) - 15, 0, 3);

    sprintf(Message, "%3.1f V", HB_Level);
    c = GetMyStringLength(Message, 0, 2);
    PutMyString(Message, (CURRENT_XS + ((BAR_WIDTH - c) >> 1)),
                (CURRENT_YS + ((CURRENT_YE - CURRENT_YS) >> 1) + 30), 0, 2);

    if (MinMaxCurrentDisplay) {
        sprintf(Message, "Chg: %3.1f A", ((float) MinCurrent * 0.1f));
        PutMyString(Message, (CURRENT_XS + 31), (CURRENT_YS + 48), 0, 2);
        sprintf(Message, "Dischg: %3.1f A", ((float) MaxCurrent * 0.1f));
        PutMyString(Message, (CURRENT_XS + 5), (CURRENT_YS + 68), 0, 2);
    }
    if (MinMaxkWDisplay) {
        sprintf(Message, "%2ds: %3.1fkw %c", LastRegenTime, LastRegenkW, LastRegenStopReason);
        c = GetMyStringLength(Message, 0, 2);
        PutMyString(Message, (CURRENT_XS + ((BAR_WIDTH - c) >> 1)), (CURRENT_YS + 254), 0, 2);
#ifdef MORE_DATA
        sprintf(Message, "%2d - %2d : %3d",
                (unsigned char) (((float) LastRegenSpdStart * 0.625f) + 0.5f),
                (unsigned char) ((((float) LastRegenSpdEnd * 0.625f) + 0.5f)), LastRegenBrake);
#else
        sprintf(Message, "%2d - %2d", (unsigned char) (((float) LastRegenSpdStart * 0.625f) + 0.5f),
                (unsigned char) ((((float) LastRegenSpdEnd * 0.625f) + 0.5f)));
#endif
        c = GetMyStringLength(Message, 0, 2);
        PutMyString(Message, (CURRENT_XS + ((BAR_WIDTH - c) >> 1)), (CURRENT_YS + 280), 0, 2);
    }

    UICopyDisplayBufferToScreen(CURRENT_XS, CURRENT_YS, BAR_WIDTH, (CURRENT_YE - CURRENT_YS + 1));

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'e';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}


#ifdef USE_VOICE_ANNOUNCEMENT
void
SaySOC(void)
{
    register int c, ty1;

    c = (unsigned int) (SOCValue >> 1); // real %

    CallVoice(V_BATTERYAT, 0);
    ty1 = c;
    ty1 /= 10;
    if (ty1 > 0) {
        if (ty1 == 1) {
            c -= 10;
            CallVoice(V_10 + c, 0);
        }
        else {
            CallVoice(V_20 + ty1 - 2, 0);
            c -= (ty1 * 10);
            if (c > 0)
                CallVoice(V_1 + c - 1, 0);
        }
    }
    else {
//              c-=(ty1*10);
        if (c > 0)
            CallVoice(V_1 + c - 1, 0);
    }
    CallVoice(V_PERCENT, 1);
}
#endif

void
UpdateSOC(void)                 //   150-128-110-104-98-90   95-103-109-115-133
{
    register int x, y, ty, c, ty1, Value;
    unsigned char Rv, Gv;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'B';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

//printf("\n%d",Value);fflush(stdout);          140 - 80%   90 - 45%   ->  60 = 35%  (SOC-90)*0.7f + 45

    Value = SOCValue;

    if (Value > SOC_UPPER_LIMIT)
        Value = SOC_UPPER_LIMIT;
    else if (Value < SOC_LOWER_LIMIT)
        Value = SOC_LOWER_LIMIT;

    if ((Value <= 130) && (Value >= 94)) {
        c = Value - 94;
        Gv = (unsigned char) (((float) c * (float) 255) / (float) (130 - 94));
        Rv = 255 - Gv;
    }
    else {
        if (Value > 130) {
            Rv = 20;
            Gv = 255;
        }
        else {
            Rv = 255;
            Gv = 20;
        }
    }
    c = Value - SOC_LOWER_LIMIT;
    ty = (int) (((float) c * (float) (SOC_YE - SOC_YS)) / (float) (SOC_UPPER_LIMIT - SOC_LOWER_LIMIT)); // How high the bar is
    ty1 = (SOC_YE - SOC_YS) - ty + SOC_YS;
    for (y = SOC_YS; y < ty1; y++) {
        c = SOC_XS + y * WIDTH;
        for (x = SOC_XS; x < SOC_XE; x++) {
            ImageBuffer[c].R = I_BKG_R;
            ImageBuffer[c].G = I_BKG_G;
            ImageBuffer[c].B = I_BKG_B;
            ++c;
        }
    }
    for (y = ty1; y < SOC_YE; y++) {
        c = SOC_XS + y * WIDTH;
        for (x = SOC_XS; x < SOC_XE; x++) {
            ImageBuffer[c].R = Rv;
            ImageBuffer[c].G = Gv;
            ImageBuffer[c].B = 20;
            ++c;
        }
    }
//      if((c=(int)((((float)SOCValue-90.0f)*0.7)+45.0f))>100) c=100;
//      sprintf(Message,"%2d%%",c);

    sprintf(Message, "%2.1f%%", ((float) SOCValue * 0.5f));
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, (SOC_XS + ((BAR_WIDTH - c) >> 1)),
                (((SOC_YE - SOC_YS) >> 1) + SOC_YS - 20), 1, 3);

#ifdef USE_VOICE_ANNOUNCEMENT

    if (((SOCValue % 10) == 0) || (FirstTimeSOC)) {
        if ((FirstTimeSOC) || (LSOCValue != SOCValue))  // Every 10 units -> 5 percent
        {
            SaySOC();
            FirstTimeSOC = 0;
        }
        LSOCValue = SOCValue;
    }

#endif

    sprintf(Message, "%d^", BattTempValue);
    c = GetMyStringLength(Message, 0, 2);
    PutMyString(Message, (SOC_XS + ((BAR_WIDTH - c) >> 1)),
                (((SOC_YE - SOC_YS) >> 1) + SOC_YS + 30), 0, 2);

    sprintf(Message, "%d V", BattVoltageValue);
    c = GetMyStringLength(Message, 0, 2);
    PutMyString(Message, (SOC_XS + ((BAR_WIDTH - c) >> 1)),
                (((SOC_YE - SOC_YS) >> 1) + SOC_YS + 60), 0, 2);

    UICopyDisplayBufferToScreen(SOC_XS, SOC_YS, BAR_WIDTH, (SOC_YE - SOC_YS + 1));
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'b';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}



#ifdef USE_VOICE_ANNOUNCEMENT

void
SayMileage(void)
{
    register int c, hu, te, on;

    CallVoice(V_FUELAVAIL, 0);
    hu = PreGasValue / 100;
    te = (PreGasValue - (hu * 100)) / 10;
    on = PreGasValue - ((hu * 100) + (te * 10));

    if ((c = PreGasValue) > 20) // Approx...
    {
        if (on > 5)
            ++te;
        if (te > 9) {
            te = 0;
            ++hu;
        }
        on = 0;
    }

    if (hu > 0) {
        CallVoice(V_1 + hu - 1, 0);
        CallVoice(V_100, 0);
    }

    if (te > 0) {
        if (te == 1) {
            CallVoice(V_10 + on, 0);
        }
        else {
            CallVoice(V_20 + te - 2, 0);
            if (on > 0)
                CallVoice(V_1 + on - 1, 0);
        }
    }
    else {
        if (on > 0)
            CallVoice(V_1 + on - 1, 0);
    }
    CallVoice(V_MILES, 1);
}

#endif

#define GG_TINFO	((GG_YE)-16)

void
UpdateGG(void)                  // 11.9 Gal / 45 Liter ( 11.904762 Gal )
{
    register int x, y, ty, c, ty1;
    unsigned char PGG;
    float Value, MPG, Galls;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'G';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    PGG = GasGauge;
    if (GasGauge > GG_UPPER_LIMIT)
        GasGauge = GG_UPPER_LIMIT;
    c = GasGauge;
    ty = (int) (((float) c * (float) (GG_YE - GG_YS)) / (float) GG_UPPER_LIMIT);        // How high the bar is
    ty1 = (GG_YE - GG_YS) - ty + GG_YS;
    for (y = GG_YS; y < ty1; y++) {
        c = GG_XS + y * WIDTH;
        for (x = GG_XS; x < GG_XE; x++) {
            ImageBuffer[c].R = 100;
            ImageBuffer[c].G = 0;
            ImageBuffer[c].B = 0;
            ++c;
        }
    }
    for (y = ty1; y < GG_YE; y++) {
        c = GG_XS + y * WIDTH;
        for (x = GG_XS; x < GG_XE; x++) {
            ImageBuffer[c].R = 10;
            ImageBuffer[c].G = 100;
            ImageBuffer[c].B = 255;
            ++c;
        }
    }
    c = (int) (((float) GasGauge * 100.0f) / (float) GG_UPPER_LIMIT);
    sprintf(Message, "%2d%%", c);
    c = GetMyStringLength(Message, 1, 3);
    PutMyString(Message, ((GG_XS + ((BAR_WIDTH - c) >> 1)) + 5),
                ((GG_YS + ((GG_YE - GG_YS) >> 1))) - 20, 1, 3);

    if (GasGaugeExtraValue != 99) {
        sprintf(Message, "%2d", (int) GasGaugeExtraValue);
        c = GetMyStringLength(Message, 1, 2);
        PutMyString(Message, (GG_XS + (BAR_WIDTH - c)), (GG_YS + 2), 1, 2);
    }



    Galls = ((float) GasGauge * 45.0f) / ((float) GG_UPPER_LIMIT * 3.78f);      // In Gallons

    if (AccRpm > 0.0f)
        MPG = (float) (((AccSpd * SPDSCALER_MILE) / AccRpm));
    else
        MPG = 0.0f;

    if ((MeasurementKilometers + DistanceTravelled) > 0) {
        Value = ((MeasuredKMPG * 0.625f * MeasurementKilometers) + (MPG * DistanceTravelled)) / (MeasurementKilometers + DistanceTravelled);    // Accumulated MPG
        Value *= Galls;
        if (SI_Measurements)
            sprintf(Message, "%3.0f km", (Value * 1.6f));
        else
            sprintf(Message, "%3.0f mil", Value);

#ifdef USE_VOICE_ANNOUNCEMENT
        if ((FirstTimeGas))     // ||(Value<50))        // Don't say small numbers...
        {
            if (NoTrafficYet == 0) {
                FirstTimeGas = 0;
                if (PreGasValue != (int) Value) {
                    PreGasValue = (int) Value;
                    SayMileage();
                }
            }
        }
#endif
    }
    else
        sprintf(Message, "=====");
    PutMyString(Message, (GG_XS + 5), ((GG_YS + ((GG_YE - GG_YS) >> 1))) + 36, 0, 2);

    if (MPG != 0.0f) {
        MPG *= Galls;
        if (SI_Measurements)
            sprintf(Message, "%3.0f km", (MPG * 1.6f));
        else
            sprintf(Message, "%3.0f mil", MPG);
    }
    else
        sprintf(Message, "=====");
    c = GetMyStringLength(Message, 0, 2);
    PutMyString(Message, (GG_XE - c - 5), ((GG_YS + ((GG_YE - GG_YS) >> 1))) + 36, 0, 2);

    if ((Galls = 11.904762f - TripGal) < 0.01f)
        Galls = 0.0f;           // Assuming 11.9 Gallon bladder size
    if (SI_Measurements)
        sprintf(Message, "%1.2f Ltr", (Galls * 3.78f));
    else
        sprintf(Message, "%1.2f Gal", Galls);
    c = GetMyStringLength(Message, 0, 2);
    PutMyString(Message, (GG_XS + ((BAR_WIDTH - c) >> 1)), (GG_YS + 16), 0, 2);

    if (SI_Measurements)
        sprintf(Message, "%3.1f km = %1.3f l", (TripMile * 1.6f), (TripGal * 3.78f));
    else
        sprintf(Message, "%3.1f mil = %1.3f Gal", TripMile, TripGal);
    c = GetMyStringLength(Message, 0, 1);
    PutMyString(Message, ((GG_XS + ((BAR_WIDTH - c) >> 1)) + 2), (GG_TINFO + 2), 0, 1);

    UICopyDisplayBufferToScreen(GG_XS, GG_YS, BAR_WIDTH, (GG_YE - GG_YS + 1));
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'g';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

}


#define	RPM_UPPER_LIMIT		0x6F    // 0x77 is rare...
#define	ONE_PER_RPM_UPPER_LIMIT	0.009f
#define	IDDLE_RPM		0x28
#define	HIGH_RPM		0x50

#define	RPM_FREQ_UPDATE			2
#define	ONE_PER_RPM_FREQ_UPDATE		0.5f

int RPMFreqCntr = RPM_FREQ_UPDATE;
unsigned int AccumulatedRPM = 0;

void
UpdateRpm(void)
{
    register int x, y, ty, c, ty1, Value, ShowValue;
    unsigned char Rv, Gv;

//printf("\nRPM : %x",V);fflush(stdout);

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'R';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif



    Value = (int) ((float) AccumulatedRPM * ONE_PER_RPM_FREQ_UPDATE);
    ShowValue = Value;
    if (Value > RPM_UPPER_LIMIT)
        Value = RPM_UPPER_LIMIT;
    RPMFreqCntr = RPM_FREQ_UPDATE;
    AccumulatedRPM = 0;

    if (PreviousRPMValue == Value) {
#ifdef TRACE_IT
        TrBu[TrBuPtr] = 'r';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
        TrBu[TrBuPtr] = '1';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return;
    }
    PreviousRPMValue = Value;

    if (DoorStatus)
        return;

//printf(" => %x",V);fflush(stdout);

    ty = (int) (((float) Value * (float) (RPM_YE - RPM_YS)) * ONE_PER_RPM_UPPER_LIMIT); // How high the bar is
    ty1 = (RPM_YE - RPM_YS) - ty + RPM_YS;
    for (y = RPM_YS; y < ty1; y++) {
        c = RPM_XS + y * WIDTH;
        for (x = RPM_XS; x < RPM_XE; x++) {
            ImageBuffer[c].R = I_BKG_R;
            ImageBuffer[c].G = I_BKG_G;
            ImageBuffer[c].B = I_BKG_B;
            ++c;
        }
    }

    if ((Value <= HIGH_RPM) && (Value >= IDDLE_RPM)) {
        c = Value - IDDLE_RPM;
        Rv = (unsigned char) (((float) c * (float) 255) / (float) (HIGH_RPM - IDDLE_RPM));
        Gv = 255 - Rv;
    }
    else {
        if (Value > HIGH_RPM) {
            Rv = 255;
            Gv = 0;
        }
        else {
            Gv = 255;
            Rv = 0;
        }
    }

    for (y = ty1; y < RPM_YE; y++) {
        c = RPM_XS + y * WIDTH;
        for (x = RPM_XS; x < RPM_XE; x++) {
            ImageBuffer[c].R = Rv;
            ImageBuffer[c].G = Gv;
            ImageBuffer[c].B = 10;
            ++c;
        }
    }

    sprintf(Message, "%d", (ShowValue << 5));
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (RPM_XS + ((BAR_WIDTH - c) >> 1)), (((RPM_YE - RPM_YS) >> 1) + RPM_YS), 0,
                3);
    UICopyDisplayBufferToScreen(RPM_XS, RPM_YS, BAR_WIDTH, (RPM_YE - RPM_YS + 1));
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'r';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}


#define	TEMP_XS			RPM_XS
#define	TEMP_YS			50
#define	TEMP_YS_1		120
#define	TEMP_XE_1		((RPM_XE)-(BAR_WIDTH>>1))
#define	TEMP_XE_2		RPM_XE

#define	TEMP_UPPER_LIMIT		190     // 80 C
#define	TEMP_UPPER_LIMIT_CLR_H		96      // 48 C
#define	TEMP_UPPER_LIMIT_CLR_L		72      // 36 C
#define	TEMP_LOWER_LIMIT		48      // 24 C

#define	TEMP_UPPER_LIMIT_ENB		95      // 95 C
#define	TEMP_UPPER_LIMIT_ENB_CLR_H	44
#define	TEMP_UPPER_LIMIT_ENB_CLR_L	36
#define	TEMP_LOWER_LIMIT_ENB		24


void
GetColors(int Value, int LL, int CLL, int CHL, unsigned char *Rv, unsigned char *Gv,
          unsigned char *Bv)
{
    if (Value >= CHL) {
        *Rv = 255;
        *Bv = 8;
        *Gv = 8;
        return;
    }
    if (Value <= LL) {
        *Rv = 8;
        *Bv = 255;
        *Gv = 8;
        return;
    }
    if (Value <= CLL)           // Blue to white
    {
        *Rv =
            ((unsigned char) ((((float) (Value - LL) * (float) 239) / (float) (CLL - LL)) +
                              16) >> 1);
        *Bv = 255;
        *Gv = *Rv;
        return;
    }
    else                        // White to Red
    {
        *Bv =
            ((unsigned char) ((255 - (((float) (Value - CLL) * (float) 239) / (float) (CHL - CLL))))
             >> 1);
        *Rv = 255;
        *Gv = *Bv;
        return;
    }
}

void
UpdateTemp(void)
{
    register int x, y, ty, c, ty1, Value;
    unsigned char Rv, Gv, Bv;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'J';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    Value = (int) TempValue;

//printf("\n%d",Value);fflush(stdout);

    if (Value > TEMP_UPPER_LIMIT)
        Value = TEMP_UPPER_LIMIT;
    else if (Value < TEMP_LOWER_LIMIT)
        Value = TEMP_LOWER_LIMIT;

    c = Value - TEMP_LOWER_LIMIT;
    GetColors(Value, TEMP_LOWER_LIMIT, TEMP_UPPER_LIMIT_CLR_L, TEMP_UPPER_LIMIT_CLR_H, &Rv, &Gv,
              &Bv);
    ty = (int) (((float) c * (float) (TEMP_YE - TEMP_YS_1)) / (float) (TEMP_UPPER_LIMIT - TEMP_LOWER_LIMIT));   // How high the bar is
    ty1 = (TEMP_YE - TEMP_YS_1) - ty + TEMP_YS_1;
    for (y = TEMP_YS_1; y < ty1; y++) {
        c = TEMP_XS + y * WIDTH;
        for (x = TEMP_XS; x < TEMP_XE_1; x++) {
            ImageBuffer[c].R = I_BKG_R;
            ImageBuffer[c].G = I_BKG_G;
            ImageBuffer[c].B = I_BKG_B;
            ++c;
        }
    }
    for (y = ty1; y < TEMP_YE; y++) {
        c = TEMP_XS + y * WIDTH;
        for (x = TEMP_XS; x < TEMP_XE_1; x++) {
            ImageBuffer[c].R = Rv;
            ImageBuffer[c].G = Gv;
            ImageBuffer[c].B = Bv;
            ++c;
        }
    }




    Value = (int) EngBlckTmp;

//printf("\n%d",Value);fflush(stdout);

    if (Value > TEMP_UPPER_LIMIT_ENB)
        Value = TEMP_UPPER_LIMIT_ENB;
    else if (Value < TEMP_LOWER_LIMIT_ENB)
        Value = TEMP_LOWER_LIMIT_ENB;

    c = Value - TEMP_LOWER_LIMIT_ENB;
    GetColors(Value, TEMP_LOWER_LIMIT_ENB, TEMP_UPPER_LIMIT_ENB_CLR_L, TEMP_UPPER_LIMIT_ENB_CLR_H,
              &Rv, &Gv, &Bv);
    ty = (int) (((float) c * (float) (TEMP_YE - TEMP_YS_1)) / (float) (TEMP_UPPER_LIMIT_ENB - TEMP_LOWER_LIMIT_ENB));   // How high the bar is
    ty1 = (TEMP_YE - TEMP_YS_1) - ty + TEMP_YS_1;
    for (y = TEMP_YS_1; y < ty1; y++) {
        c = TEMP_XE_1 + y * WIDTH;
        for (x = TEMP_XE_1; x < TEMP_XE_2; x++) {
            ImageBuffer[c].R = I_BKG_R;
            ImageBuffer[c].G = I_BKG_G;
            ImageBuffer[c].B = I_BKG_B;
            ++c;
        }
    }
    for (y = ty1; y < TEMP_YE; y++) {
        c = TEMP_XE_1 + y * WIDTH;
        for (x = TEMP_XE_1; x < TEMP_XE_2; x++) {
            ImageBuffer[c].R = Rv;
            ImageBuffer[c].G = Gv;
            ImageBuffer[c].B = Bv;
            ++c;
        }
    }


    sprintf(Message, "%d", (TempValue >> 1));
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (TEMP_XS + ((BAR_WIDTH - c) >> 2)) - 10,
                (((TEMP_YE - TEMP_YS_1) >> 1) + TEMP_YS_1 - 10), 0, 3);

    sprintf(Message, "%d", EngBlckTmp);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (TEMP_XE_1 + ((BAR_WIDTH - c) >> 2)) - 10,
                (((TEMP_YE - TEMP_YS_1) >> 1) + TEMP_YS_1 - 10), 0, 3);

    sprintf(Message, "^");
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (TEMP_XS + ((BAR_WIDTH - c) >> 1)),
                (((TEMP_YE - TEMP_YS_1) >> 1) + TEMP_YS_1 + 10), 0, 3);
    UICopyDisplayBufferToScreen(TEMP_XS, TEMP_YS_1, BAR_WIDTH, (TEMP_YE - TEMP_YS_1 + 1));
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'j';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}

#define	TEMP_UPPER_LIMIT_CAT1		800
#define	TEMP_UPPER_LIMIT_CAT1_CLR_H	280
#define	TEMP_UPPER_LIMIT_CAT1_CLR_L	140
#define	TEMP_LOWER_LIMIT_CAT1		-50

#define	TEMP_UPPER_LIMIT_CAT2		600
#define	TEMP_UPPER_LIMIT_CAT2_CLR_H	90
#define	TEMP_UPPER_LIMIT_CAT2_CLR_L	45
#define	TEMP_LOWER_LIMIT_CAT2		-50

#define	TEMP_YE_1		(TEMP_YS_1-10)

void
UpdateCatTemp(void)
{
    register int x, y, ty, c, ty1, Value;
    unsigned char Rv, Gv, Bv;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'K';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    Value = (int) Cat1Temp;

//printf("\n%d",Value);fflush(stdout);

    if (Value > TEMP_UPPER_LIMIT_CAT1)
        Value = TEMP_UPPER_LIMIT_CAT1;
    else if (Value < TEMP_LOWER_LIMIT_CAT1)
        Value = TEMP_LOWER_LIMIT_CAT1;

    c = Value - TEMP_LOWER_LIMIT_CAT1;
    GetColors(Value, TEMP_LOWER_LIMIT_CAT1, TEMP_UPPER_LIMIT_CAT1_CLR_L,
              TEMP_UPPER_LIMIT_CAT1_CLR_H, &Rv, &Gv, &Bv);
    ty = (int) (((float) c * (float) (TEMP_YE_1 - TEMP_YS)) / (float) (TEMP_UPPER_LIMIT_CAT1 - TEMP_LOWER_LIMIT_CAT1)); // How high the bar is
    ty1 = (TEMP_YE_1 - TEMP_YS) - ty + TEMP_YS;

    if (LatestRPM)              // Valid only if ICE is ON.
    {
        for (y = TEMP_YS; y < ty1; y++) {
            c = TEMP_XS + y * WIDTH;
            for (x = TEMP_XS; x < TEMP_XE_1; x++) {
                ImageBuffer[c].R = I_BKG_R;
                ImageBuffer[c].G = I_BKG_G;
                ImageBuffer[c].B = I_BKG_B;
                ++c;
            }
        }
        for (y = ty1; y < TEMP_YE_1; y++) {
            c = TEMP_XS + y * WIDTH;
            for (x = TEMP_XS; x < TEMP_XE_1; x++) {
                ImageBuffer[c].R = Rv;
                ImageBuffer[c].G = Gv;
                ImageBuffer[c].B = Bv;
                ++c;
            }
        }
    }
    else {
        for (y = TEMP_YS; y < TEMP_YE_1; y++) {
            c = TEMP_XS + y * WIDTH;
            for (x = TEMP_XS; x < TEMP_XE_1; x++) {
                ImageBuffer[c].R = I_BKG_R;
                ImageBuffer[c].G = I_BKG_G;
                ImageBuffer[c].B = I_BKG_B;
                ++c;
            }
        }
        for (y = ty1; y < (ty1 + 7); y++) {
            c = TEMP_XS + y * WIDTH;
            for (x = TEMP_XS; x < TEMP_XE_1; x++) {
                ImageBuffer[c].R = Rv;
                ImageBuffer[c].G = Gv;
                ImageBuffer[c].B = Bv;
                ++c;
            }
            Rv >>= 1;
            Gv >>= 1;
            Bv >>= 1;
        }
    }


    Value = (int) Cat2Temp;

//printf("\n%d",Value);fflush(stdout);

    if (Value > TEMP_UPPER_LIMIT_CAT2)
        Value = TEMP_UPPER_LIMIT_CAT2;
    else if (Value < TEMP_LOWER_LIMIT_CAT2)
        Value = TEMP_LOWER_LIMIT_CAT2;

    c = Value - TEMP_LOWER_LIMIT_CAT2;
    GetColors(Value, TEMP_LOWER_LIMIT_CAT2, TEMP_UPPER_LIMIT_CAT2_CLR_L,
              TEMP_UPPER_LIMIT_CAT2_CLR_H, &Rv, &Gv, &Bv);
    ty = (int) (((float) c * (float) (TEMP_YE_1 - TEMP_YS)) / (float) (TEMP_UPPER_LIMIT_CAT2 - TEMP_LOWER_LIMIT_CAT2)); // How high the bar is
    ty1 = (TEMP_YE_1 - TEMP_YS) - ty + TEMP_YS;
    if (LatestRPM)              // Valid only if ICE is ON.
    {
        for (y = TEMP_YS; y < ty1; y++) {
            c = TEMP_XE_1 + y * WIDTH;
            for (x = TEMP_XE_1; x < TEMP_XE_2; x++) {
                ImageBuffer[c].R = I_BKG_R;
                ImageBuffer[c].G = I_BKG_G;
                ImageBuffer[c].B = I_BKG_B;
                ++c;
            }
        }
        for (y = ty1; y < TEMP_YE_1; y++) {
            c = TEMP_XE_1 + y * WIDTH;
            for (x = TEMP_XE_1; x < TEMP_XE_2; x++) {
                ImageBuffer[c].R = Rv;
                ImageBuffer[c].G = Gv;
                ImageBuffer[c].B = Bv;
                ++c;
            }
        }
    }
    else {
        for (y = TEMP_YS; y < TEMP_YE_1; y++) {
            c = TEMP_XE_1 + y * WIDTH;
            for (x = TEMP_XE_1; x < TEMP_XE_2; x++) {
                ImageBuffer[c].R = I_BKG_R;
                ImageBuffer[c].G = I_BKG_G;
                ImageBuffer[c].B = I_BKG_B;
                ++c;
            }
        }
        for (y = ty1; y < (ty1 + 7); y++) {
            c = TEMP_XE_1 + y * WIDTH;
            for (x = TEMP_XE_1; x < TEMP_XE_2; x++) {
                ImageBuffer[c].R = Rv;
                ImageBuffer[c].G = Gv;
                ImageBuffer[c].B = Bv;
                ++c;
            }
            Rv >>= 1;
            Gv >>= 1;
            Bv >>= 1;
        }
    }

    sprintf(Message, "%d", Cat1Temp);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (TEMP_XS + ((BAR_WIDTH - c) >> 2)) - 20,
                (((TEMP_YE_1 - TEMP_YS) >> 1) + TEMP_YS - 20), 0, 3);

    sprintf(Message, "%d", Cat2Temp);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (TEMP_XE_1 + ((BAR_WIDTH - c) >> 2)) - 10,
                (((TEMP_YE_1 - TEMP_YS) >> 1) + TEMP_YS - 20), 0, 3);

    sprintf(Message, "^");
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (TEMP_XS + ((BAR_WIDTH - c) >> 1)),
                (((TEMP_YE_1 - TEMP_YS) >> 1) + TEMP_YS), 0, 3);
    UICopyDisplayBufferToScreen(TEMP_XS, TEMP_YS, BAR_WIDTH, (TEMP_YE_1 - TEMP_YS + 1));
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'k';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}


#define	EV_XS		36
#define	EV_YS		48
#define	EV_YE		(EV_YS+102)
#define	EV_WIDTH	140
#define	EV_XE		(EV_XS+EV_WIDTH)

void
UpdateDriveMode(void)
{
    register int sr, c, x, y, d;
    struct ImageBufferStructure *ColorInfo = NULL;
    unsigned char *PictureInfo = NULL;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'V';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

// Mode_EngineRed   Mode_EngineGreen   Mode_Electric   Mode_ElectricCanc

    sr = 1;
    switch (EVMode) {
    case 0x40:                 // EV operation
        ColorInfo = Mode_Electric_Color;
        PictureInfo = (unsigned char *) Mode_Electric_Picture;
#ifdef USE_VOICE_ANNOUNCEMENT
        if (LEVMode != EVMode) {
            CallVoice(V_EVACT, 1);
            LEVMode = EVMode;
        }
#endif
        break;
    case 0x0:                  // Normal operation
        if (ICEPowered) {
            ColorInfo = Mode_EngineRed_Color;
            PictureInfo = (unsigned char *) Mode_EngineRed_Picture;
        }
        else {
            if (LatestRPM) {
                ColorInfo = Mode_EngineGreen_Color;
                PictureInfo = (unsigned char *) Mode_EngineGreen_Picture;
            }
            else {
                ColorInfo = Mode_Electric_Color;
                PictureInfo = (unsigned char *) Mode_Electric_Picture;
                sr = 0;
            }
        }
#ifdef USE_VOICE_ANNOUNCEMENT
        if (LEVMode != EVMode) {
            if (LEVMode == 0x40)
                CallVoice(V_EVCANC, 1);
            LEVMode = EVMode;
        }
#endif
        break;
    case 0x80:                 // Can not complete EV request
        ColorInfo = Mode_ElectricCanc_Color;
        PictureInfo = (unsigned char *) Mode_ElectricCanc_Picture;
        break;
    default:                   // EV cancelled by override
        ColorInfo = Mode_ElectricCanc_Color;
        PictureInfo = (unsigned char *) Mode_ElectricCanc_Picture;
#ifdef USE_VOICE_ANNOUNCEMENT
        if (LEVMode != EVMode) {
            if (LEVMode == 0x40)
                CallVoice(V_EVCANC, 1);
            LEVMode = EVMode;
        }
#endif
        break;
    }

    if (sr == 0) {
        for (y = EV_YS; y < EV_YE; y++) {
            c = EV_XS + y * WIDTH;
            for (x = 0; x < 140; x++) {
                ImageBuffer[c].R = 0;
                ++c;
            }
        }
    }

    for (y = EV_YS; y < EV_YE; y++) {
        c = EV_XS + y * WIDTH;
        d = (y - EV_YS) * 140;
        for (x = 0; x < 140; x++) {
            if (sr)
                ImageBuffer[c].R = ColorInfo[PictureInfo[d]].R;
            ImageBuffer[c].G = ColorInfo[PictureInfo[d]].G;
            ImageBuffer[c].B = ColorInfo[PictureInfo[d]].B;
            ++c;
            ++d;
        }
    }
    UICopyDisplayBufferToScreen(EV_XS, EV_YS, EV_WIDTH, (EV_YE - EV_YS + 1));

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'v';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
}

#define	EI_XS		RPM_XS
#define	EI_YS		(RPM_YS-50)     // (TEMP_YE+10)
#define	EI_XE		RPM_XE
#define	EI_YE		(EI_YS+40)      // (RPM_YS-10)
#define	EI_HG		((EI_YE)-(EI_YS))       // BARWIDTH = 180

void
PutExtraInfo(void)
{
    register int a, b, c, va;
    unsigned char Rc, Gc;

    ClearDisplayBufferArea(EI_XS, EI_YS, BAR_WIDTH, EI_HG);

    if (PreRegVal > 0) {
        if ((va = ((int) ((float) PreRegVal * 1.8f))) > 180)
            va = 180;
        Gc = (unsigned char) ((float) va * 1.412f);
        Rc = (unsigned char) (255 - Gc);
        for (b = EI_YS; b < EI_YE; b++) {
            c = b * WIDTH + EI_XS;
            for (a = 0; a < va; a++) {
                ImageBuffer[c].R = Rc;
                ImageBuffer[c].G = Gc;
                ImageBuffer[c].B = 16;
                ++c;
            }
        }
    }

    sprintf(Message, "%d%%", PreRegVal);
    c = GetMyStringLength(Message, 0, 3);
    PutMyString(Message, (EI_XS + ((BAR_WIDTH - c) >> 1)), (EI_YS + 5), 0, 3);

    UICopyDisplayBufferToScreen(EI_XS, EI_YS, BAR_WIDTH, (EI_HG + 1));
}


int SOCUpdater = 2;

unsigned char
MineChar(int Position)
{
    register int a = Position;
    unsigned char c;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'Q';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    if (IB[a] >= 'A')
        c = (IB[a] - 'A') + 10;
    else
        c = (IB[a] - '0');
    c <<= 4;
    ++a;
    if (IB[a] >= 'A')
        c += (IB[a] - 'A') + 10;
    else
        c += (IB[a] - '0');
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'q';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    return (c);
}

unsigned int
MineUInt(int Position)
{
    register int a = Position;
    unsigned char c, d;

#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'U';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif
    if (IB[a] >= 'A')
        c = (IB[a] - 'A') + 10;
    else
        c = (IB[a] - '0');
    c <<= 4;
    ++a;
    if (IB[a] >= 'A')
        c += (IB[a] - 'A') + 10;
    else
        c += (IB[a] - '0');
    ++a;
    if (IB[a] >= 'A')
        d = (IB[a] - 'A') + 10;
    else
        d = (IB[a] - '0');
    d <<= 4;
    ++a;
    if (IB[a] >= 'A')
        d += (IB[a] - 'A') + 10;
    else
        d += (IB[a] - '0');
#ifdef TRACE_IT
    TrBu[TrBuPtr] = 'u';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    return ((unsigned int) ((((int) c << 8) + (int) d)));
}


#ifdef MORE_DATA
void
AnalyseBrakes(int Position)
{
    register unsigned char CV;

    if (CollectCurrent) {
        CV = MineChar(Position + 8);
        Info_Brakes += (unsigned int) CV;
        ++Info_Brakes_Cnt;
    }
}
#endif


//#define       GG_UPPER_LIMIT          40
#define		REFILL_UPPER_TRIGGER	34
#define		REFILL_LOWER_TRIGGER	12

void
AnalyseGasGauge(int Position)
{
    register unsigned char CV, CV1;

    CV1 = MineChar(Position);
    CV = MineChar(Position + 2);
    if ((GasGauge != CV) || (CV1 != GasGaugeExtraValue)) {
        if ((CV > REFILL_UPPER_TRIGGER) && (GasGauge < REFILL_LOWER_TRIGGER)) {
            TripKilometers = 0.0f;
            TripGallonValue = 0.0f;
            FirstGasReading = 2;
        }
        GasGaugeExtraValue = CV1;
        GasGauge = CV;
        UpdateFuelFile();
        FirstGasReading = 0;
        if (!DoorStatus)
            UpdateGG();
    }
}

void
AnalyseLights(int Position)
{
    register unsigned char CV;

    CV = MineChar(Position + 4);        // Bit #3 is the Key one
    if (CV != LightValue) {
        LightValue = CV;
        CV >>= 3;
        CV &= 1;
        if (InstrumentsDimmed != CV) {
            InstrumentsDimmed = CV;
            UIAdjustBacklight(InstrumentsDimmed);
        }
    }
}

void
AnalyseDoors(int Position)
{
    register unsigned char CV;

    if (SpeedAccumulated)       // If moving, don't show open door...
    {
        DoorStatus = 0;
        CV = (unsigned char) MineChar(Position + 4);
#ifdef USE_VOICE_ANNOUNCEMENT
        if (CV != 0)            // Open
        {
            if (DoorOpenCnt == 0) {
                CallVoice(V_DOOROPEN, 1);
                DoorOpenCnt = 5;
            }
            else
                --DoorOpenCnt;
        }
#endif
    }
    else {
        CV = (unsigned char) MineChar(Position + 4);
        if ((CV != 0) || (DoorStatus != 0))     // open...
        {
            if (DoorStatus != CV) {
                DoorStatus = CV;
                DoorOpenCnt = 0;
                ShowPrius();
            }
            if (DoorStatus == 0) {
                ClearDisplayBuffer();
                UpdateDriveMode();
                UpdateGG();
                UpdateTemp();
                UpdateSOC();
                UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
            }
        }
    }
}

void
AnalyseSpeed(int Position)
{
    register unsigned char CV;
    struct timeval tv;

    CV = MineChar(Position + 4);
    SpeedCurrStage += (unsigned int) CV;
    SpeedCurrStageCntr++;
    if (CV <= (PrvSpd + 1))     // Slowing down...
    {
        if (CV < PrvSpd)        // Only trigger if no false positive
        {
            if (Decel == 0) {
                Decel = 1;
                CollectCurrent = 1;
                gettimeofday(&tv, NULL);
                CollTime =
                    (unsigned int) ((time_t) (tv.tv_sec) - StartTime) * (unsigned int) 100 +
                    (unsigned int) ((float) (tv.tv_usec) / 10000.0f);
                StrtSpd = CV;
            }
        }
        if (CV == 0) {
            if (Decel)
                StopCurrentCollection('S');
            Decel = 0;
        }
    }
    else {
        if (Decel) {
            StopCurrentCollection('A');
            Decel = 0;
        }
    }
    PrvSpd = CV;

    SpeedAccumulated += (unsigned int) CV;
    ++SpeedAccumulatedCntr;
    if (DoorStatus)             // Door is open
    {
        if (SpeedAccumulated)   // If moving
        {
            DoorStatus = 0;
            ClearDisplayBuffer();
            UpdateDriveMode();
            UpdateGG();
            UpdateTemp();
            UpdateSOC();
            UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
        }
    }
}

void
AnalyseSOC(int Position)
{
    register unsigned int V;
    int IV;

    if (--SOCUpdater == 0) {
        V = (unsigned int) (MineChar(Position + 8));
        V += (unsigned int) (MineChar(Position + 10));
        V >>= 1;                // Temp of Battery
        BattTempValue = V;
        BattMaxChargeCurrent = (unsigned int) (MineChar(Position + 0));
        BattMaxDisChargeCurrent = (unsigned int) (MineChar(Position + 2));
        V = (unsigned int) (MineChar(Position + 6));
        if (V != SOCValue) {
            if ((IV = (int) SOCValue - (int) V) < 0)
                IV = -IV;
            if ((IV < 4) || (SOCValue == 0))    // Filter Ocasional Error on receiving
            {
                SOCValue = V;
                if (!DoorStatus)
                    UpdateSOC();
            }
        }
        SOCUpdater = 5;
    }
}

void
AnalyseRPM(int Position)
{
    register unsigned char CV;

    CV = MineChar(Position + 4);
    AccumulatedRPM += (unsigned int) CV;
    if (--RPMFreqCntr == 0) {
        if ((CV == 0) && (LatestRPM != 0)) {
            LatestRPM = 0;
            UpdateDriveMode();
            UpdateCatTemp();
        }
        else
            LatestRPM = CV;
        UpdateRpm();
    }
}

void
AnalyseThrottle(int Position)
{
    register unsigned int V;
    register unsigned char CV;

    CV = MineChar(Position + 8);        // 0 when ICE is not excerting power
//      if(CV!=0) CV=1;
    if ((CV < 0xE0) && (CV != 0))
        CV = 1;
    else
        CV = 0;
    if (ICEPowered != CV) {
        ICEPowered = CV;
        if (!DoorStatus)
            UpdateDriveMode();
    }
    V = MineUInt(Position + 4);
    if (LatestRPM == 0) {
        V = 0;
    }
    ThrottleAccumulated += (unsigned long) V;
    ++ThrottleAccumulatedCntr;
}

void
AnalyseCurrent(int Position)
{
    register unsigned int V, B;

    B = MineUInt(Position + 4); // Voltage
    AccumulatedHB_Level += (unsigned long) B;
    V = MineUInt(Position);     // Current

    if (ICEPowered)
        ICEPoweredCurrStage++;

    if (V >= 0x800)             // Regen  0x0FF4  0x0E59
    {
        V = 0x0FFF - (V & 0xFFF);
        if (MinCurrent < V) {
            MinCurrent = V;
            MinCurrentVoltage = B;
            MinMaxCurrentDisplay = 30;  // 30 seconds
        }
        AccumulatedCurrent -= (long) V;
        if (CollectCurrent) {
            if (ICEPowered == 0) {
                CollCurr += (unsigned long) V;
                CollVolt += (unsigned long) B;
                ++CollCntr;
            }
            CollectCurrent = 1;
        }
    }
    else {
        if (MaxCurrent < V) {
            MaxCurrent = V;
            MaxCurrentVoltage = B;
            MinMaxCurrentDisplay = 30;  // 30 seconds
        }
        AccumulatedCurrent += (long) V;
        if (CollectCurrent) {
            if (++CollectCurrent > 30) {
                StopCurrentCollection('C');
                Decel = 0;
            }
        }
    }
    if (--CurrentFreqCntr == 0) {
        UpdateCurrent();
        ICEPoweredCurrStage = 0;
        SOCValueCurrStage = SOCValue;
        SpeedCurrStage = 0;
        SpeedCurrStageCntr = 0;
    }
}

void
FastPoll(void)
{
    register int cp, pp, a, go, ml, c, mid;
    register unsigned int V;
    unsigned char CV;


    if (Simulation) {
        unsigned char CV1;

        AccumulatedCurrent = (long) ((rand() & 0xFFFF) - 0x7FFF);
        UpdateCurrent();
        ICEPowered = (unsigned char) (rand() & 1);
        ThrottleAccumulated += (unsigned long) (rand() % 0xFFFF);
        ++ThrottleAccumulatedCntr;
        SpeedAccumulated += (unsigned int) (float) (rand() & 0x7F);
        ++SpeedAccumulatedCntr;
        AccumulatedRPM += (unsigned int) (rand() % 0x01FF);
        LatestRPM = (unsigned char) (rand() & 0x1F);
        UpdateRpm();
        SOCValue = (unsigned int) (rand() & 0x3F) + 90;
        UpdateSOC();
        TempValue = (unsigned char) (rand() & 0x7F) + 10;
        EngBlckTmp = (unsigned char) (rand() & 0x7F);
        UpdateTemp();
        Cat1Temp = (unsigned char) (rand() & 0xFF) + 10;
        Cat2Temp = (unsigned char) (rand() & 0xFF) + 10;
        UpdateCatTemp();

        GasGauge = (unsigned char) (rand() & 0x1F);
        UpdateGG();
        CV1 = 0x20;
        CV1 <<= (rand() & 0x3);
        EVMode = CV1;
        UpdateDriveMode();      // 0x40, 0x80, 0x00, 
        UICopyDisplayBufferToScreen((EV_XE - 20), EV_YS, BAR_WIDTH, (EV_YE - EV_YS + 1));
        return;
    }

#ifdef TRACE_IT
    TrBu[TrBuPtr] = '<';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

//      if(Port<0) return;              // This should never happen...

    if (IBCP >= IBS)
        IBCP = 0;               // Panic

#ifdef TRAFFIC_PROFILE
    ++NofNInv;
#endif

    if (Simulation) {
        sprintf(IB + IBCP,
                "t03B502E400BFE5\nt12080000000010130450\nt3486000100000052\nt3C8504280000FC\nt3CA5004D210040\nt3CB76664007E131141\nt3CD5000000BE93\nt52C2238A\nt57F768301000000000\nt5A426323\nt5296A70000854000\nt5B6364C100\n");
        cp = 231 - 26 - 12;
    }
    else {
        cp = read(Port, IB + IBCP, (IBS - IBCP));
    }


    if (cp <= 0) {
#ifdef TRACE_IT
        TrBu[TrBuPtr] = '>';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
        TrBu[TrBuPtr] = '1';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return;
    }

    if (NoTrafficYet) {
        NoTrafficYet = 0;
#ifdef USE_VOICE_ANNOUNCEMENT
        CallVoice(V_GREETINGS, 0);
        SayTime();
#endif
    }

    ml = SOC_XS + TrafficCtr + (TrafficSubCtr * 29) + ((GG_YE + 4) * WIDTH);
    for (go = (GG_YE + 4); go < (SOC_YS - 4); go++)     // TrafficSubCtr==0 going right
    {
        ImageBuffer[ml].R = 0;
        ImageBuffer[ml].G = 0;
        ImageBuffer[ml].B = 0;
        ml += WIDTH;
    }
    UICopyDisplayBufferToScreen((SOC_XS + TrafficCtr + (TrafficSubCtr * 29)), (GG_YE + 4), 1,
                                ((SOC_YS - 4) - (GG_YE + 4) + 1));

    if (TrafficSubCtr) {
        if (TrafficCtr == 0)
            TrafficSubCtr = 0;
        else
            --TrafficCtr;
    }
    else {
        if (TrafficCtr == 149)
            TrafficSubCtr = 1;
        else
            ++TrafficCtr;
    }

    ml = SOC_XS + TrafficCtr + ((1 - TrafficSubCtr) * 29) + (GG_YE + 4) * WIDTH;
    for (go = (GG_YE + 4); go < (SOC_YS - 4); go++) {
        ImageBuffer[ml].R = 255;
        ImageBuffer[ml].G = 150;
        ImageBuffer[ml].B = 0;
        ml += WIDTH;
    }
    UICopyDisplayBufferToScreen((SOC_XS + TrafficCtr + ((1 - TrafficSubCtr) * 29)), (GG_YE + 4), 1,
                                ((SOC_YS - 4) - (GG_YE + 4) + 1));

    NofTrafficBytes += (unsigned int) cp;


#ifdef TRAFFIC_PROFILE
    ++NofInv;
#endif

    IBCP += cp;
    go = 1;
    cp = 0;
    pp = 0;
    while (go) {
        while ((cp < IBCP) && (IB[cp] != 't'))
            ++cp;
        if (cp >= IBCP)
            go = 0;
        else {                  // 't' found, a points to MessageID;
            pp = cp;
            a = cp + 1;         // a points to first character in ID
            if ((a + 4) < IBCP) // The header can be mined
            {
#if 0
                if (IB[a] == '7') {
                    printf("\n");
                    for (ml = 0; ml < 20; ml++)
                        printf("%c", IB[a + ml]);
                    fflush(stdout);
                }
#endif
                ml = (int) (IB[a + 3] - '0');   // t3CDl0011223344556677
                if ((ml > 0) && (ml < 9))       // valid ?
                {
                    if ((a + 4 + (ml << 1)) < IBCP)     // enough bytes ?
                    {
                        mid = 0;
                        for (c = 0; c < 3; c++, a++) {
                            mid <<= 4;
                            if (IB[a] >= 'A')
                                mid += (IB[a] - 'A') + 10;
                            else
                                mid += (IB[a] - '0');
                        }

//  0x3B: Current               60/s
// 0x120: CarOn/Off
// 0x348: Throttle              11.3/s
// 0x3C8: RPM                   7.1/s
// 0x3CA: Speed                 4.6/s
// 0x3CB: SOC                   4.6/s
// 0x3CD: Battery Voltage       4.6/s
// 0x52C: Temp                  4.6/s
// 0x57F: Lights / Instrument Panel Dimming
// 0x5A4: Gas Gauge             0.1/s
// 0x5B6: Doors
// 0x529: EV mode
// 0x7Ex: Module response
                        ++a;
                        switch (mid) {
#ifdef MORE_DATA
                        case 0x30:     // Brakes
                            AnalyseBrakes(a);
                            break;
#endif

                        case 0x39:     // Temp         60/s
                            if (--EngBlckTmpCnt == 0) {
                                EngBlckTmpCnt = 30;
                                CV = MineChar(a);
                                if (CV != EngBlckTmp) {
                                    EngBlckTmp = CV;
                                    if (!DoorStatus)
                                        UpdateTemp();
                                }
                            }
                            break;

                        case 0x3B:     // Current      60/s
                            AnalyseCurrent(a);
                            break;

                        case 0x120:
                            if (IB[a + 13] == '0')      // If car is in standby
                                PriusIsPowered = 0;
                            else
                                PriusIsPowered = 1;
                            break;

                        case 0x348:    // Throttle     11.3/s          0 - 33280
                            AnalyseThrottle(a);
                            break;

                        case 0x3C8:    // RPM          7.1/s   0 - 26880
                            AnalyseRPM(a);
                            break;

                        case 0x3CA:    // Speed        4.6/s
                            AnalyseSpeed(a);
                            break;

                        case 0x3CB:    // SOC          4.6/s
                            AnalyseSOC(a);
                            break;

                        case 0x3CD:    // Battery Voltage              4.6/s
                            V = MineUInt(a + 4);        // Voltage
                            if (BattVoltageValue != V)
                                BattVoltageValue = V;
                            break;

                        case 0x52C:    // Temp         4.6/s
                            CV = MineChar(a + 2);
                            if (CV != TempValue) {
                                TempValue = CV;
                                if (!DoorStatus)
                                    UpdateTemp();
                            }
                            break;

                        case 0x57F:    // Lights / Instrument Panel Dimming
                            AnalyseLights(a);
                            break;

                        case 0x5A4:    // Gas Gauge  0.1/s
                            AnalyseGasGauge(a);
                            break;

                        case 0x5B6:    // Doors
                            AnalyseDoors(a);
                            break;

                        case 0x529:    // EV mode
                            CV = MineChar(a + 8);
                            if (EVMode != CV) {
                                EVMode = CV;
                                UpdateDriveMode();
                            }
                            break;

                        case 0x7E8:
                        case 0x7EA:
                        case 0x7EB:
                            AnalyseHighCANMessages(mid, a);
                            break;
                        }
                        cp = a + 2 + (ml << 1);
                    }
                    else
                        go = 0;
                }
                else
                    cp = a + 3;
            }
            else
                go = 0;
        }
    }

    if (pp == 0) {
        IBCP = 0;
#ifdef TRACE_IT
        TrBu[TrBuPtr] = '>';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
        TrBu[TrBuPtr] = '2';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif
        return;
    }
    memcpy((void *) (&IB[0]), (void *) (&IB[pp]), (size_t) (IBCP - pp));
    if ((IBCP -= pp) > 20)      // failsafe
    {
        IBCP = 0;
    }
#ifdef TRACE_IT
    TrBu[TrBuPtr] = '>';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    return;
}

int
main(int argc, char **argv)
{
    int a, b, c;
    unsigned char FVoiceMode = 0;
#ifdef TRAFFIC_PROFILE
    float fv;
#endif

#ifdef TRACE_IT
    TrBu[TrBuPtr] = '.';
    if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
        TrBuPtr = 0;
#endif

    b = 1;
    while (argc > b) {
        a = 0;
        while (argv[b][a] != '\0') {
            switch (argv[b][a]) {
            case 'v':
                FVoiceMode = 1;
                break;
            case 'f':
                ForcePath = 1;
                break;
            case 's':
                SI_Measurements = 1;
                break;
            case 'o':
                Simulation = 1;
                stat_file_name = STAT_FILE_NAME_SIM;
                fuel_file_name = FUEL_FILE_NAME_SIM;
                break;
            case '-':
            case 'h':
            case '?':
                printf("\n\nGraphCan %s", VERSION_STRING);
                printf("\n\nUsage : %s [vfs]", argv[0]);
                printf("\n\tv - Turn OFF voice mode");
                printf("\n\tf - Take sound samples and player from %s directory", FORCE_PATH);
                printf("\n\ts - Use SI measurements ( km/h etc.)");
                printf("\n\to - Offline Simulation mode");
                printf("\n\nTouching the 1st vertical quarter of the screen :");
                printf("\n    Switch Voice On/Off.");
                printf("\n 2nd and 3rd quarter of the screen :");
                printf("\n    While Initializing CAN : Go to Info / Setting mode.");
                printf("\n    While in ScreenSaver : Save data to card.");
                printf("\n    While in Running mode : Switch between SI and imperial.");
                printf("\n 4th quarter :");
                printf("\n    Quit.");
                printf
                    ("\n\nType 'su' and 'chmod a+w /dev/fl' for backlight control from GraphCan after reboot...");
                fflush(stdout);
                printf("\n\n");
                fflush(stdout);
                exit(0);
                break;
            }
            ++a;
        }
        ++b;
    }

    SetUpMySignals();
    if (UICreateWindow())
        return (0);

    DefineButtons();
    SetUpPicture();

#ifdef USE_KEYBOARD
    ioctl(0, TCGETS, &oldT);
    ioctl(0, TCGETS, &newT);
    newT.c_lflag &= ~ECHO;
    newT.c_lflag &= ~ICANON;
    ioctl(0, TCSETS, &newT);

    c = 0;
    if (fcntl(0, F_SETOWN, getpid()) >= 0) {
        c |= FASYNC;
        if (fcntl(0, F_SETFL, c) < 0) {
            printf("\nError with SIGIO for terminal...");
            fflush(stdout);
        }
    }
#endif

    while (RealQuit == 0) {
        RunningTask = TASK_INIT;
        UIAdjustBacklight(InstrumentsDimmed);
        NoTrafficYet = 1;
#ifdef USE_VOICE_ANNOUNCEMENT
        InitVoice();
#endif

        ResetValues();
        LoadStat();
        if (FVoiceMode)
            VoiceMode = 0;

      INIT_CAN:
        ClearDisplayBuffer();
        sprintf(Message, "Initializing CAN...");
        PutMyString(Message, 20, 150, 0, 6);
        c = GetMyStringLength(VERSION_STRING, 0, 2) + 10;
        PutMyString(VERSION_STRING, (WIDTH - c), 450, 0, 2);
        UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
        c = 1;

        while (c) {
            if (SetUpCAN() == 0) {
                ClearDisplayBuffer();
                sprintf(Message, "CAN is not ready!");
                PutMyString(Message, 20, 150, 0, 6);
                UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
                if ((ProcessGo == 0) || (RealQuit == 1)) {
                    ClearDisplayBuffer();
                    sprintf(Message, "Quit detected, exit.");
                    PutMyString(Message, 20, 20, 0, 6);
                    UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
                    CleanUp(0);
                    printf("\nExited...\n");
                    fflush(stdout);
                    return (1);
                }
                if (RunningTask == TASK_INFO)
                    RunTaskInfoMain();
                if (Port >= 0) {
                    close(Port);
                    Port = -1;
                }
                sleep(3);
                ClearDisplayBuffer();
                sprintf(Message, "Initializing CAN...");
                PutMyString(Message, 20, 150, 0, 6);
                UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
            }
            else
                c = 0;
        }
        if (Port < 0)
            goto INIT_CAN;      // Just to avoid any confusion... This should never happen...

        RunningTask = TASK_RUNNING;

        ClearDisplayBuffer();
        UpdateDriveMode();
        UpdateGG();
        UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
        alarm(2);

        StartTime = time(NULL);

        while (ProcessGo) {
            if (Sleeping == 0)
                FastPoll();
            else
                ScreenSaver();

            if (NeedSynced)     // This gets invoked every second...
            {

                if (MinMaxkWDisplay > 0)
                    --MinMaxkWDisplay;
                if (MinMaxCurrentDisplay > 0)
                    --MinMaxCurrentDisplay;

#ifdef TRACE_IT
                TrBu[TrBuPtr] = '!';
                if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
                    TrBuPtr = 0;
#endif

#ifdef TRAFFIC_PROFILE

                NofInv = 0;
                NofNInv = 0;
#endif

                if (NofTrafficBytes == 0) {
                    if (++NofTrafficBytesZero > 3)
                        PriusIsPowered = 0;
                }
                else {
                    NofTrafficBytesZero = 0;
                }

                UpdateSpeedComputations();

                SendCATRequests();

#ifdef USE_VOICE_ANNOUNCEMENT
                CheckTime();
#endif

                if (++SyncCntr > 4) {
                    if (--FSRCntr == 0) // 6*5 = 30 sec full screen refresh
                    {
                        if (!DoorStatus)
                            UpdateGG();
                        FSRCntr = FULLSCREEN_REFRESH_RATE;
                        UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
                    }
                }
                NeedSynced = 0;


                if ((PriusIsPowered == 0) && (Sleeping == 0) && ((unsigned char) CurrentSpeed == 0))    // Prius turned off
                {
                    if ((difftime(time(NULL), StartTime)) > 15) {
                        RunningTask = TASK_SCREENSAVER;
                        alarm(0);
                        SaveStat();
                        Sleeping = 1;
                        ScrSv_NumberOfTimes = 1;
                        if (Port >= 0) {
                            sprintf(Message, "C\015");  // Close the CAN channel
                            if (WriteToPort(Message)) {
                                printf("\nError writing to port : Close CAN Channel");
                                fflush(stdout);
                            }
                            close(Port);
                            Port = -1;
                        }
                    }
                    else
                        PriusIsPowered = 1;
                }

#ifdef TRACE_IT
                TrBu[TrBuPtr] = '@';
                if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
                    TrBuPtr = 0;
#endif
            }
#ifdef TRACE_IT
            TrBu[TrBuPtr] = ',';
            if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
                TrBuPtr = 0;
#endif

#ifdef NON_ZAURUS
            UIMainLoop();
#endif

        }
#ifdef TRACE_IT
        TrBu[TrBuPtr] = ':';
        if (++TrBuPtr >= TRACE_BUFFER_LENGTH)
            TrBuPtr = 0;
#endif

        if (RealQuit == 0)      // just temporary pause...
        {
            ContSignal = 0;
            ProcessGo = 1;
            ClearDisplayBuffer();
            sprintf(Message, "RESTARTING.");
            PutMyString(Message, 20, 150, 0, 6);
            sleep(1);
        }
    }
    CleanUp(1);
    return 0;
}
