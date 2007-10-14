/*
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <math.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>


#define	GRAPHICS_C	1	// for main.h

//#define       DEBUG_BUFFERS   1
//#define       DEBUG_MESSAGES  1
//#define       DEBUG_GRAPHOPS  1

#define	SWAP_RGB	1

#include "main.h"


#define	WIDTH	820
#define	HEIGHT	520

#define		WINDOWNAME	"Attila's CAN data visualizer"
#define		ICONNAME	"CANVIS"

char Verbose = 0, Main_Info = 0;


Window WorkWindow = 0;
Display *WorkDisplay;
int WorkScreen, WorkDepth, WorkBitsPerRGB, WorkScanLinePad;
unsigned long WorkRedMask, WorkGreenMask, WorkBlueMask;
unsigned int WorkRedNumberOfShifts, WorkGreenNumberOfShifts, WorkBlueNumberOfShifts;
unsigned int WorkRedNumberOfUpShifts, WorkGreenNumberOfUpShifts, WorkBlueNumberOfUpShifts;
Visual *WorkVisual = NULL;
XImage *WorkImage = NULL;
char *WorkData = NULL;
struct ImageBufferStructure
{
    unsigned char R, G, B;
} ImageBufferStructure;
struct ImageBufferStructure *ImageBuffer = NULL;
int WorkWidth = WIDTH, WorkHeight = HEIGHT;
int DisplayWidth = WIDTH, DisplayHeight = HEIGHT + 30;
int WorkPosX = 0, WorkPosY = 0;
char WorkWindowInitialized = 0;
GC WorkWindowGC = 0, WorkPixmapGC = 0;
Pixmap WorkPixmap = 0;
Cursor WorkCursor = 0;
char ProcessGo = 1;

#if 0
void (*LFunc) ();
if ((LFunc = (void (*)()) LClientData) != NULL)
    LFunc();
#endif

typedef struct ButtonStructure
{
    void (*PushFunction) (void *);
    int (*ButtonFunction) (int, int, char, unsigned char);
    void *PushClientData;
    int (*KeyFunction) (int, char);
    unsigned char PushChar, ImageManipulate;
    int x, y, w, h;
    char name[128];
} ButtonStructure;

ButtonStructure *Buttons = NULL;
int NumberOfButtons = 0;
int ActiveButton = -1;
int SelectedImage = -1, PreviousSelectedImage = -1;
unsigned char ButtonPressed = 0;
int DrawTarget = 0;		// 0=WorkBuffer, 1=DisplayBuffer;

unsigned int InternalKeyModifiers = 0;
unsigned char KeyModifiers = 0;
int ButtonPressedX = -1, ButtonPressedY = -1;
int ButtonPressedImageX = -1, ButtonPressedImageY = -1;
int ZPressX = -1, ZPressY = -1, ZPressSel = -1;

MyImagesStructure *MyImages = NULL;
int NumberOfImages;

char TmpBuffer[512];
char *TmpChrPtrBuffer[2];

#define	FONTMAP_WIDTH	175
#define	FONTMAP_HEIGHT	31
#define	FONTMAP_COLORS	10

struct
{
    char letter;
    unsigned char Color;
} Font_Color[FONTMAP_COLORS] = {
    {
    '.', 0x00}, {
    '@', 0xFF}, {
    '#', 0xF0}, {
    '&', 0xE8}, {
    '%', 0xE0}, {
    '$', 0xA0}, {
    '-', 0x80}, {
    '+', 0x60}, {
    '=', 0x40}, {
'*', 0x20},};

char *Font_Map[FONTMAP_HEIGHT] = {
    "..+@@#+....$%%%.....+#@#$....+#@&*.......+%...=%%%%%=.....$%+...%%%%%%%...-#@#-....+@@#$........................................................................................",
    ".%@#%#@%...%@@@....%@#%@@+..$@#&@#......$@@...-@@@@@-....=@@=...@@@@@@@..=@@%@@=..&@#%@@+.........$#@&=..%@@-..*&@#-..-#@+.....&@..-@@@%...#@*.%@@@@%..%@#*..-#@&*..............",
    "*@@*.*@@*....@@...=@@*.*@@*.&@$.&@-....*@@@...%@-........#@+........$@&..+@$.+@%.-@#..*@@*.......=@%-##..=+@-..##-%@=.@&$@$...-@@..+#--=..$@$..=--$@+.$@$##.=@%-##=.............",
    "=@%...%@-....@@...-@%...%@-.....&@-....&@@@...%@-.......+@#.........#@-..%@-.-@%.%@-...%@-.......%@..-@-..-@-.=@$..@%.-**@%...#@@..%%....*@&......+@*.%@.%@.%@..$@%.............",
    "=@%...%@-....@@...*-=...&@-...$#@@*...$@$@@...@@+-+=...=@@&%$......$@#...=@#-#@$.%@-...%@-..$%=..%@..-@-..-@-.*-*.=@%..=@@-..+#&@..@#@#-.+@&%*....#&..=@&@&.%@..-@-.............",
    "-@%...%@-....@@........%@#....%@@#*..*##.@@..*@@@@@@-..#@@@@@+.....&@$....&@@@#..$@#..*@@*..%@-..%@..-@-..-@-....*#@=..=&@%.*@$%@..@#-&@=@#%@#...$@-..%@%##*$@%-#@..............",
    "-@%...%@-....@@.......%@@-....=-&@#..%@=.@@..*+&*.$@@.-@#=.-@@*...=@@*...%@%-&@%.*#@#%@@&........%@..-@-..-@-...*#@-.....+@=&&.%@.....*@%@*.$@-..##..=@+.=@+.+@@@$........$%=...",
    "=@%...%@-....@@......-@@-........#@=-@%..@@........&@=%@$...&@=...&@+...=@#...#@=.*&@#@@*........%@..-@-..-@-..*#@-..$+..-@+@&%#@%-%*.*@%@..-@-.=@$..-@-..@%..*#&.........%@-...",
    "=@%...%@-....@@.....-@@-...$%=...%@=#@&%%@@%=$%=...%@=%@-...%@=..*@@=...-@%...%@-....#@$....$%=..=@%-##*..-@-.*#@%--=+@+-##=%%%#@%*@&-%@=@%-##..&@*..*##-%@-..%@*...............",
    "*@#..*@@=....@@....-@@-....+@&..*@@*@@@@@@@@-+@&..*@@*$@&..*@@*..+@&....=@#..*#@*...%@&.....%@-...$#@&=...-@-.-@@@@@%.+@@&*....%@..=&@#-.$#@&=.*@+....=&@#$..-@+................",
    ".%@&$#@&.....@@...=@@#%%%%=*#@&$#@%......@@..*#@&$#@+..#@&$#@%..*@@-.....%@&$#@%...-@@=.........................................................................................",
    ".*%@@@&*.....@@...-@@@@@@@-.=&@@@-.......@@...*&@@@+...*&@@@%...+@#......*%@@@+...*#@+..........................................................................................",
    "....*.........................**................**........*.................*........................................................................................@@@...@@*..",
    "...-*...---*....--*..=--=...=---**---*..*--...*-..*-.==...-**-..=-.-*..=-*...--.=-...-*...--*...---*....--*...---=..*-*.=---===..-*-=...=--=..-..*-==...---*..=-=---=@$$=..*@$..",
    "..=@+...@@@#*.-@&%@+.%#%@#*.%#%%=-@%%=.+@%&@-.-@..-@.%%...@--@.-@-.@-..%@+..*@@.%@$..@-.-@&%@+..@&#@=.-#&%@%..@&&@-=@%@-$&@%$%%..@-+&...&%%%.=@=.%&-@=.$@=#+.*#+$%%@+@$.....@$..",
    "..+@@...@-*@-=@-..-%=%%.*##.%%...-@...$#*..-%*-@..-@.%%...@--@*#%..@-..%@#..$@@.%@#..@-=@-..*#+.@-.&%=@-..*#+.@-.%@%%.$$.-@..%%..@-=@=.=@=-@.$@$.#$.%#*@+.$@.+#...$@*@$.....@$..",
    ".*@-@$..@+$@*+&......%%..*@=%&--*-@--*#$......-@--+@.%%...@--@%&...@-..%%@=.#%@.%%#+.@-+&....$#.@-*#++&....$@.@-.$@$@%=..-@..%%..@-.&+.+#.*@=%&%=@*.*#@&...#+@-...#+.@$.....@$..",
    ".$@*&&..@&%&*%%......%%...@-%#%%=-@%%=@-.%%%%$-@%%&@.%%...@--@@$...@-..%%+#=#-@.%%=@*@-%%....-@.@@@&*%&--..-@.@@@@-.$&@+.-@..%%..@-.$#.#$..&$@-@$#...%@$...$@&...$@*.@$.....@$..",
    ".#@@@@=.@-.&+$#......%%..-@*%%...-@...#+.--$@%-@..-@.%%...@--@$@*..@-..%%=#+#-@.%%.%&@-$#....+&.@-...$@&&@++#.@&@=....$@.-@..%%..@-..@$@*..+#@.##+..=@&#*...@-...#+..@$.....@$..",
    "=@=..&%.@-*#+.#%*.%@=%%.$@%.%%...-@...-@$.*##*-@..-@.%%-%*@--@.&#..@-..%%.#@=-@.%%.*@@-*#%*.$@-.@-....#&*=@@$.@-&#.%&.%#.-@..$#*-@*..%#%...=@%.%@=..#%.#%...@-..$@*..@@@...@@$..",
    "&&...$@*@@@%*.*+@@&-.%@@#+..%@@@--@....=&@@%*.-@..-@.%%*&@%.-@.*#%.@@@%%%.+#.-@.%%..+@-.*+@@&=..@-....*+@@&%@.@-*#%*&@#*.-@...+@#-...=@=....#$.-@..%#*.=@-..@-..%@@@%*$$=..*==..",
    ".......==...............==........=*......*-....*-.-**-....=*...........................................==......................................................................",
    ".......%%...............%%.......-@=......-@....-@.%=-@....@=...........................................%%....................................-#@-.=@*.............-=....*+++#..",
    ".=%%=%=%%+%$...$%+*..$%%&%..$%+**&&**+%$%=-@+%=.=*.%=-@.-%*@==%+%=$%$..%$%+...$%+*.$++%$...+%+%==%+.-%$=##$$$.=%*%=.=%-%.=%.=%*%=*%-+$..%$%%%%@%%@=++...-@-........@%....++&+#..",
    "*@%-#@-%@$$@+.%#-##.$@$$@%.#&-%#=##=#&-&@--@%+@*-@.@--@*#$.@=-@$%@#$#$.@#$@$.%#-%#*%@$$@$.&#-%@--@&=@+@+&&=%%.-@.#+.%&*@=+@*+&.%&+#.$@.=@=--#&@$$@-@*...*-*........@%....*+&+#..",
    "+&..-@-%%..$@=@*.*-*#$..%%-@$*-@+%%-@*.*@--@..@--@.@--@%%..@=-@..@-.%%.@-.%%=@*..#+%&..$#=@*.*@--@.=@+=.%%.%%.-@.$#.@-.&$&@-#$.*@@=.*@-%&..$@*%@@%$%........%@@%.@@@@@@..*+&+#..",
    "%%..*@-%%..-@-@.....@-..%%-@%%%%=%%-@...@--@..@--@.@--@#+..@=-@..@-.%%.@-.%%-@...%+%%..-@-@...@--@..-&@$%%.%%.-@.*@+#..$&@-&@*.*#@*..%%@$.*@$..=-.#.....*-*.=--=.--@&--..*+&+#..",
    "$@=.%@-%#**&%*#+.-%*&#**#%.#+.*=*%%*@$.-@--@..@--@.@--@=@-.@=-@..@-.%%.@-.%%*#+.-@*%#**&&*@+.-@--@.=%.&%%%.+&.+@..%@+..*@%*@%..%#%&..-@#..&&.....=&-#@-.-@-........@%....*+&+#..",
    ".+@@+@-%##@&*.=&@#-.*&@#&%.*&@#$.%%.-#@%@--@..@--@.@--@.+@*@=-@..@-.%%.@-.%%.=&@#-.%#@@&*.-#@%@--@.*&@&*%%.*&@&@..-@*...&-.&-.-@=*@+..@+..@@@@...&=@%%@............%$..**++&++##",
    "....................................*%--@*........=@=..............................%%.........@-.....................................-@*........*#.@$$@..................*+&+#..",
    ".....................................-##-.........-+...............................%%.........@-.....................................&%.........-$.%@@%...................*+#..."
};

struct
{
    char letter;
    int x;
    int y;
    int width;
    int height;
} bignumbers[] = {
    {
    '0', 0, 0, 10, 13}, {
    '1', 10, 0, 7, 13}, {
    '2', 17, 0, 10, 13}, {
    '3', 27, 0, 9, 13}, {
    '4', 36, 0, 9, 13}, {
    '5', 45, 0, 9, 13}, {
    '6', 54, 0, 9, 13}, {
    '7', 63, 0, 9, 13}, {
    '8', 72, 0, 9, 13}, {
    '9', 81, 0, 9, 13}, {
    ':', 91, 0, 5, 13}, {
    '`', -1, -1, -1, -1}
};


struct
{
    char letter;
    int x;
    int y;
    int width;
    int height;
} fonts[] = {
    {
    'A', 0, 13, 8, 8}, {
    'B', 8, 13, 5, 8}, {
    'C', 13, 13, 8, 8}, {
    'D', 21, 13, 7, 8}, {
    'E', 28, 13, 5, 8}, {
    'F', 33, 13, 5, 8}, {
    'G', 38, 13, 8, 8}, {
    'H', 46, 13, 6, 8}, {
    'I', 52, 13, 3, 8}, {
    'J', 55, 13, 5, 8}, {
    'K', 60, 13, 6, 8}, {
    'L', 66, 13, 5, 8}, {
    'M', 71, 13, 8, 8}, {
    'N', 79, 13, 8, 8}, {
    'O', 87, 13, 8, 8}, {
    'P', 95, 13, 6, 8}, {
    'Q', 101, 13, 8, 8}, {
    'R', 109, 13, 6, 8}, {
    'S', 115, 13, 5, 8}, {
    'T', 120, 13, 5, 8}, {
    'U', 125, 13, 6, 8}, {
    'V', 131, 13, 7, 8}, {
    'W', 138, 13, 9, 8}, {
    'X', 147, 13, 7, 8}, {
    'Y', 154, 13, 6, 8}, {
    'Z', 160, 13, 5, 8}, {
    'a', 0, 21, 7, 10}, {
    'b', 7, 21, 6, 10}, {
    'c', 13, 21, 7, 10}, {
    'd', 20, 21, 6, 10}, {
    'e', 26, 21, 6, 10}, {
    'f', 32, 21, 4, 10}, {
    'g', 36, 21, 6, 10}, {
    'h', 42, 21, 6, 10}, {
    'i', 48, 21, 2, 10}, {
    'j', 50, 21, 3, 10}, {
    'k', 53, 21, 6, 10}, {
    'l', 59, 21, 2, 10}, {
    'm', 61, 21, 9, 10}, {
    'n', 70, 21, 6, 10}, {
    'o', 76, 21, 7, 10}, {
    'p', 83, 21, 6, 10}, {
    'q', 89, 21, 7, 10}, {
    'r', 96, 21, 3, 10}, {
    's', 99, 21, 5, 10}, {
    't', 104, 21, 3, 10}, {
    'u', 107, 21, 5, 10}, {
    'v', 112, 21, 6, 10}, {
    'w', 118, 21, 8, 10}, {
    'x', 126, 21, 6, 10}, {
    'y', 132, 21, 6, 10}, {
    'z', 138, 21, 4, 10}, {
    '%', 142, 21, 9, 10}, {
    ':', 151, 21, 5, 10}, {
    '.', 169, 0, 4, 10}, {
    '-', 156, 21, 5, 10}, {
    '+', 160, 21, 6, 10}, {
    '0', 96, 1, 8, 10}, {
    '1', 104, 1, 5, 10}, {
    '2', 109, 1, 8, 10}, {
    '3', 117, 1, 6, 10}, {
    '4', 123, 1, 7, 10}, {
    '5', 130, 1, 7, 10}, {
    '6', 136, 1, 7, 10}, {
    '7', 143, 1, 6, 10}, {
    '8', 149, 1, 7, 10}, {
    '9', 156, 1, 7, 10}, {
    ' ', 163, 0, 5, 10}, {
    '^', 168, 21, 9, 10}, {
    ']', 169, 12, 5, 10}, {
    '[', 165, 12, 5, 10}, {
    '`', -1, -1, -1, -1}
};

void
_xstat(void)
{
}

void
_Xsetlocale(void)
{
}

extern void ShowColorBalance(char *, int, int, int);
char *CreatePicture(unsigned int *ret_width, unsigned int *ret_height, int *SrcBufferSize);
int InitializeNextImageStructure(void);

extern int DRed, DGreen, DBlue;


void
CleanUp(void)
{
//      if(WorkImage!=NULL) XDestroyImage(WorkImage);
    if (WorkPixmap != (Pixmap) 0)
	XFreePixmap(WorkDisplay, WorkPixmap);
    if (WorkCursor != 0)
	XFreeCursor(WorkDisplay, WorkCursor);
    if (WorkPixmapGC != NULL)
	XFreeGC(WorkDisplay, WorkPixmapGC);
    if (WorkWindowGC != NULL)
	XFreeGC(WorkDisplay, WorkWindowGC);
    if ((WorkWindow != 0) && (WorkDisplay != NULL))
	XDestroyWindow(WorkDisplay, WorkWindow);
    if (WorkDisplay != NULL)
	XCloseDisplay(WorkDisplay);
    if (ImageBuffer != NULL)
	free((void *) ImageBuffer);
    if (WorkData != NULL)
	free((void *) WorkData);
}


void
TransferFont(int fsx, int fsy, int tx, int ty, int fw, int fh, int zoom)
{
    register char *FRPtr, ch;	// current row's string ptr
    register int cr = fsy, tr = fsy + fh;	// current / target row
    register int cc, sc = fsx, tc = fsx + fw;	// current / start / target column
    register int cp, sz1, sz2;	// colour pointer
    register int WBPtr, WBpy = ty;	// Pointer to workbuffer
    register unsigned char Rv, Gv, Bv;

    if (WBpy < 0) {
	cr -= WBpy;
	WBpy = 0;
    }				// don't copy if out of screen (y<0)
    if (tx < 0) {
	sc = fsx - tx;
	tx = 0;
    }

    if (DrawTarget) {
	if ((WBpy + (fh * zoom)) >= DisplayHeight)
	    tr = fsy + (DisplayHeight - WBpy);	// don't copy if out of screen (y>width)
	if ((tx + (fw * zoom)) >= DisplayWidth)
	    tc -= (tx + (fw * zoom) - DisplayWidth);
    }
    else {
	if ((WBpy + (fh * zoom)) >= WorkHeight)
	    tr = fsy + (WorkHeight - WBpy);	// don't copy if out of screen (y>width)
	if ((tx + (fw * zoom)) >= WorkWidth)
	    tc -= (tx + (fw * zoom) - WorkWidth);
    }

    while (cr < tr)		// raw...
    {
	FRPtr = Font_Map[cr];	// ptr to string within font map...
	for (sz2 = 0; sz2 < zoom; sz2++) {
	    for (cc = sc; cc < tc; cc++)	// column
	    {
		ch = FRPtr[cc];
		for (sz1 = 0; sz1 < zoom; sz1++) {
		    WBPtr = (tx + sz1 + ((cc - sc) * zoom) + (WBpy + sz2) * DisplayWidth);
		    if (WBPtr >= 0) {
			for (cp = 0; cp < FONTMAP_COLORS; cp++) {
			    if (Font_Color[cp].letter == ch) {
				Rv = (unsigned char) ABS((int) ImageBuffer[WBPtr].R -
							 (int) Font_Color[cp].Color);
				Gv = (unsigned char) ABS((int) ImageBuffer[WBPtr].G -
							 (int) Font_Color[cp].Color);
				Bv = (unsigned char) ABS((int) ImageBuffer[WBPtr].B -
							 (int) Font_Color[cp].Color);
				ImageBuffer[WBPtr].R = Rv;
				ImageBuffer[WBPtr].G = Gv;
				ImageBuffer[WBPtr].B = Bv;
			    }
			}
		    }
		}
	    }
	}
	WBpy += zoom;
	cr++;
    }
}

void
PutMyString(char *Text, int x, int y, int usebignums, int zoom)
{
    int a = 0, b, px = x, notfound;

    while (Text[a] != '\0') {
	notfound = 1;
	if (usebignums) {
	    b = 0;
	    while ((bignumbers[b].letter != Text[a]) && (bignumbers[b].x >= 0))
		++b;
	    if (bignumbers[b].x >= 0) {
		TransferFont(bignumbers[b].x, bignumbers[b].y, px, y - 2, bignumbers[b].width,
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
		    if ((fonts[b].letter == '%') && (!usebignums))
			TransferFont(fonts[b].x, fonts[b].y, px, y - 2, fonts[b].width,
				     fonts[b].height, zoom);
		    else
			TransferFont(fonts[b].x, fonts[b].y, px, y, fonts[b].width, fonts[b].height,
				     zoom);
		}
		px += fonts[b].width * zoom;
	    }
	}
	++a;
    }
}

void
TransferFontB(char *TB, int xsz, int ysz, int fsx, int fsy, int tx, int ty, int fw, int fh,
	      int zoom)
{
    register char *FRPtr, ch;	// current row's string ptr
    register int cr = fsy, tr = fsy + fh;	// current / target row
    register int cc, sc = fsx, tc = fsx + fw;	// current / start / target column
    register int cp, sz1, sz2;	// colour pointer
    register int WBPtr, WBpy = ty;	// Pointer to workbuffer
    register unsigned char Rv, Gv, Bv;
    register struct ImageBufferStructure *IB;

    IB = (struct ImageBufferStructure *) TB;
    if (WBpy < 0) {
	cr -= WBpy;
	WBpy = 0;
    }				// don't copy if out of screen (y<0)
    if (tx < 0) {
	sc = fsx - tx;
	tx = 0;
    }

    if ((WBpy + (fh * zoom)) >= ysz)
	tr = fsy + (ysz - WBpy);	// don't copy if out of screen (y>width)
    if ((tx + (fw * zoom)) >= xsz)
	tc -= (tx + (fw * zoom) - xsz);

    while (cr < tr)		// raw...
    {
	FRPtr = Font_Map[cr];	// ptr to string within font map...
	for (sz2 = 0; sz2 < zoom; sz2++) {
	    for (cc = sc; cc < tc; cc++)	// column
	    {
		ch = FRPtr[cc];
		for (sz1 = 0; sz1 < zoom; sz1++) {
		    WBPtr = (tx + sz1 + ((cc - sc) * zoom) + (WBpy + sz2) * xsz);
		    if (WBPtr >= 0) {
			for (cp = 0; cp < FONTMAP_COLORS; cp++) {
			    if (Font_Color[cp].letter == ch) {
				Rv = (unsigned char) ABS((int) IB[WBPtr].R -
							 (int) Font_Color[cp].Color);
				Gv = (unsigned char) ABS((int) IB[WBPtr].G -
							 (int) Font_Color[cp].Color);
				Bv = (unsigned char) ABS((int) IB[WBPtr].B -
							 (int) Font_Color[cp].Color);
				IB[WBPtr].R = Rv;
				IB[WBPtr].G = Gv;
				IB[WBPtr].B = Bv;
			    }
			}
		    }
		}
	    }
	}
	WBpy += zoom;
	cr++;
    }
}

void
PutMyStringB(char *TB, int xsz, int ysz, char *Text, int x, int y, int usebignums, int zoom)
{
    int a = 0, b, px = x, notfound;

    while (Text[a] != '\0') {
	notfound = 1;
	if (usebignums) {
	    b = 0;
	    while ((bignumbers[b].letter != Text[a]) && (bignumbers[b].x >= 0))
		++b;
	    if (bignumbers[b].x >= 0) {
		TransferFontB(TB, xsz, ysz, bignumbers[b].x, bignumbers[b].y, px, y - 2,
			      bignumbers[b].width, bignumbers[b].height, zoom);
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
		    TransferFontB(TB, xsz, ysz, fonts[b].x, fonts[b].y, px, y - 1, fonts[b].width,
				  fonts[b].height, zoom);
		else {
		    if ((fonts[b].letter == '%') && (!usebignums))
			TransferFontB(TB, xsz, ysz, fonts[b].x, fonts[b].y, px, y - 2,
				      fonts[b].width, fonts[b].height, zoom);
		    else
			TransferFontB(TB, xsz, ysz, fonts[b].x, fonts[b].y, px, y, fonts[b].width,
				      fonts[b].height, zoom);
		}
		px += fonts[b].width * zoom;
	    }
	}
	++a;
    }
}

int
GetMyStringLength(char *Text, int usebignums, int zoom)
{
    int a = 0, b, px = 0, notfound;

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
    return (px);
}

void
GR_Refresh(void)
{
    XSync(WorkDisplay, False);
}

void
GR_GetPixel(int x, int y, unsigned char *GRv_BckR, unsigned char *GRv_BckG, unsigned char *GRv_BckB)
{
    register int c;

    if (DrawTarget) {
	if ((x < 0) || (x >= DisplayWidth) || (y < 0) || (y >= DisplayHeight)) {
	    *GRv_BckR = 0;
	    *GRv_BckG = 0;
	    *GRv_BckB = 0;
	    return;
	}
    }
    else {
	if ((x < 0) || (x >= WorkWidth) || (y < 0) || (y >= WorkHeight)) {
	    *GRv_BckR = 0;
	    *GRv_BckG = 0;
	    *GRv_BckB = 0;
	    return;
	}
    }
    c = x + y * DisplayWidth;
    *GRv_BckR = ImageBuffer[c].R;
    *GRv_BckG = ImageBuffer[c].G;
    *GRv_BckB = ImageBuffer[c].B;
}

void
GR_PutPixel(int x, int y, unsigned char GRv_BckR, unsigned char GRv_BckG, unsigned char GRv_BckB)
{
    register int c;

    if (DrawTarget) {
	if ((x < 0) || (x >= DisplayWidth) || (y < 0) || (y >= DisplayHeight))
	    return;
    }
    else {
	if ((x < 0) || (x >= WorkWidth) || (y < 0) || (y >= WorkHeight))
	    return;
    }
    c = x + y * DisplayWidth;
    ImageBuffer[c].R = GRv_BckR;
    ImageBuffer[c].G = GRv_BckG;
    ImageBuffer[c].B = GRv_BckB;
}

void
GR_DrawLine(int gx1, int gy1, int gx2, int gy2, int mode, unsigned char GRv_FwdR, unsigned char GRv_FwdG, unsigned char GRv_FwdB)	// mode : 0-put 1-antialias 2-negate
{
    register int x1 = gx1, y1 = gy1, x2 = gx2, y2 = gy2;
    register unsigned int a = 0, nofs;
    register float x, y, xs, ys;
    register unsigned char mr, mg, mb, hnt, ybgr;
    unsigned char GRv_BckR, GRv_BckG, GRv_BckB;


#ifdef DEBUG_GRAPHOPS
    printf("\nGR_DrawLine(%d,%d,%d,%d, %d, %03d,%03d,%03d)", gx1, gy1, gx2, gy2, mode, GRv_FwdR,
	   GRv_FwdG, GRv_FwdB);
    fflush(stdout);
#endif

    if (x2 < x1) {
	a = x2;
	x2 = x1;
	x1 = (int) a;
	a = y2;
	y2 = y1;
	y1 = (int) a;
    }
    if (y1 > y2)
	ybgr = 0;
    else
	ybgr = 1;

    if (ABS((x2 - x1)) > ABS((y2 - y1))) {
	xs = ((x2 >= x1) ? (1.0) : (-1.0));
	ys = (float) (y2 - y1) / (float) (ABS((x2 - x1)));
	nofs = ABS((x2 - x1));
	hnt = 0;
    }
    else {
	xs = (float) (x2 - x1) / (float) (ABS((y2 - y1)));
	ys = ((y2 >= y1) ? (1.0) : (-1.0));
	nofs = ABS((y2 - y1));
	hnt = 1;
    }
    x = (float) x1, y = (float) y1;
    a = 0;

    if ((mode == 1) && ((x1 == x2) || (y1 == y2)))	// no antialiasing necessarry
	mode = 0;
    switch (mode) {
    case 0:
	do {
	    GR_PutPixel((int) x, (int) y, GRv_FwdR, GRv_FwdG, GRv_FwdB);
	    x += xs;
	    y += ys;
	    ++a;
	} while (a <= nofs);
	break;
    case 1:			// antialias
	do {
	    if (!hnt)		// step in x
	    {
		if (CHNK(y) >= (float) 0.5) {
		    GR_GetPixel((int) x, (int) (y + 1), &GRv_BckR, &GRv_BckG, &GRv_BckB);
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * (1 - CHNK(y))) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * (1 - CHNK(y))) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * (1 - CHNK(y))) +
					  GRv_FwdB);
		    GR_PutPixel((int) x, (int) (y + 1), mr, mg, mb);
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * CHNK(y)) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * CHNK(y)) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * CHNK(y)) +
					  GRv_FwdB);
		    if (ybgr)
			GR_PutPixel((int) x, (int) (y + ((ys < 0) ? (2) : (0))), mr, mg, mb);
		    else
			GR_PutPixel((int) x, (int) (y + ((ys < 0) ? (0) : (1))), mr, mg, mb);
		}
		else {
		    GR_GetPixel((int) x, (int) (y + 1), &GRv_BckR, &GRv_BckG, &GRv_BckB);
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * (1 - CHNK(y))) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * (1 - CHNK(y))) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * (1 - CHNK(y))) +
					  GRv_FwdB);
		    GR_PutPixel((int) x, (int) (y + 1), mr, mg, mb);
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * CHNK(y)) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * CHNK(y)) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * CHNK(y)) +
					  GRv_FwdB);
		    GR_PutPixel((int) x, (int) y, mr, mg, mb);
		}
	    }
	    else {
		if (CHNK(x) >= (float) 0.5) {
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * (1 - CHNK(x))) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * (1 - CHNK(x))) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * (1 - CHNK(x))) +
					  GRv_FwdB);
		    GR_PutPixel((int) (x + ((xs < 0) ? (-1) : (1))), (int) y, mr, mg, mb);
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * CHNK(x)) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * CHNK(x)) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * CHNK(x)) +
					  GRv_FwdB);
		    GR_PutPixel((int) x, (int) y, mr, mg, mb);
		}
		else {
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * (1 - CHNK(x))) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * (1 - CHNK(x))) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * (1 - CHNK(x))) +
					  GRv_FwdB);
		    GR_PutPixel((int) (x + ((xs < 0) ? (-1) : (1))), (int) y, mr, mg, mb);
		    mr = (unsigned char) ((((float) GRv_BckR - (float) GRv_FwdR) * CHNK(x)) +
					  GRv_FwdR);
		    mg = (unsigned char) ((((float) GRv_BckG - (float) GRv_FwdG) * CHNK(x)) +
					  GRv_FwdG);
		    mb = (unsigned char) ((((float) GRv_BckB - (float) GRv_FwdB) * CHNK(x)) +
					  GRv_FwdB);
		    GR_PutPixel((int) x, (int) y, mr, mg, mb);
		}
	    }
	    x += xs;
	    y += ys;
	    ++a;
	} while (a <= nofs);
	break;
    case 2:
	do {
	    GR_GetPixel((int) x, (int) y, &GRv_BckR, &GRv_BckG, &GRv_BckB);
	    GR_PutPixel((int) x, (int) y, (unsigned char) (255 - GRv_BckR),
			(unsigned char) (255 - GRv_BckG), (unsigned char) (255 - GRv_BckB));
	    x += xs;
	    y += ys;
	    ++a;
	} while (a <= nofs);
	break;
    }
}

void
GR_DrawRectangle(int gx1, int gy1, int gx2, int gy2, int mode, unsigned char GRv_FwdR, unsigned char GRv_FwdG, unsigned char GRv_FwdB)	// mode : 0-put 1-antialias 2-negate
{

#ifdef DEBUG_GRAPHOPS
    printf("\nGR_DrawRectangle(%d,%d,%d,%d, %d, %03d,%03d,%03d)", gx1, gy1, gx2, gy2, mode,
	   GRv_FwdR, GRv_FwdG, GRv_FwdB);
    fflush(stdout);
#endif

    GR_DrawLine(gx1, gy1, gx2, gy1, mode, GRv_FwdR, GRv_FwdG, GRv_FwdB);
    GR_DrawLine(gx2, gy1 + 1, gx2, gy2, mode, GRv_FwdR, GRv_FwdG, GRv_FwdB);
    GR_DrawLine(gx2 - 1, gy2, gx1, gy2, mode, GRv_FwdR, GRv_FwdG, GRv_FwdB);
    GR_DrawLine(gx1, gy2 - 1, gx1, gy1 + 1, mode, GRv_FwdR, GRv_FwdG, GRv_FwdB);
}

void
GR_DrawFillRectangle(int gx1, int gy1, int gx2, int gy2, int mode, unsigned char GRv_FwdR, unsigned char GRv_FwdG, unsigned char GRv_FwdB)	// mode : 0-put 1-antialias 2-negate
{
    register int ty;

    for (ty = gy1; ty < gy2; ty++)
	GR_DrawLine(gx1, ty, gx2, ty, mode, GRv_FwdR, GRv_FwdG, GRv_FwdB);
}

void
ClearWorkBuffer(void)
{
    register int a, b, c;

#ifdef DEBUG_BUFFERS
    printf("\nClearWorkBuffer");
    fflush(stdout);
#endif

    for (b = 0; b < WorkHeight; b++) {
	c = a + b * DisplayWidth;
	for (a = 0; a < WorkWidth; a++) {
	    c = a + b * DisplayWidth;
	    ImageBuffer[c].R = BKG_R;
	    ImageBuffer[c].G = BKG_G;
	    ImageBuffer[c].B = BKG_B;
	}
    }

#ifdef DEBUG_BUFFERS
    printf(" ok.");
    fflush(stdout);
#endif

}

void
ReDrawWorkBuffer(int x, int y, int w, int h)
{
    register int a, b, c, wx = x, wy = y, wh = h, ww = w;

    if (wx < 0) {
	if ((ww -= (0 - wx)) <= 0)
	    return;		// nothing to draw...
	wx = 0;
    }
    if (wx >= DisplayWidth)
	return;
    if (wy < 0) {
	if ((wh -= (0 - wy)) <= 0)
	    return;
	wy = 0;
    }

    wy += wh;
    wx += ww;
    for (b = wy; b < wh; b++)	// clear
    {
	c = a + b * DisplayWidth;
	for (a = wx; a < ww; a++) {
	    ImageBuffer[c].R = BKG_R;
	    ImageBuffer[c].G = BKG_G;
	    ImageBuffer[c++].B = BKG_B;
	}
    }
}

void
ClearDisplayBuffer(void)
{
    register int a, b, c;

#ifdef DEBUG_BUFFERS
    printf("\nClearDisplayBuffer (%x)", ImageBuffer);
    fflush(stdout);
#endif

    for (b = 0; b < DisplayHeight; b++) {
	for (a = 0; a < DisplayWidth; a++) {
	    c = a + b * DisplayWidth;
	    ImageBuffer[c].R = BKG_R;
	    ImageBuffer[c].G = BKG_G;
	    ImageBuffer[c].B = BKG_B;
	}
    }

#ifdef DEBUG_BUFFERS
    printf(" ok.");
    fflush(stdout);
#endif

}

void
CopyWorkBufferToScreen(void)
{
    register int a, b, c, d;
    register unsigned short *TmpWrk = (unsigned short *) WorkData;
    register unsigned char *TmpChrWrk = (unsigned char *) WorkData;

#ifdef DEBUG_BUFFERS
    printf("\nCopyWorkBufferToScreen");
    fflush(stdout);
#endif

    switch (WorkBitsPerRGB) {
    case 8:
	for (b = 0; b < WorkHeight; b++)
	    for (a = 0; a < WorkWidth; a++) {
		c = a + b * DisplayWidth;
		TmpChrWrk[c] =
		    (char) ((ImageBuffer[c].
			     R >> WorkRedNumberOfShifts) << WorkRedNumberOfUpShifts) |
		    ((ImageBuffer[c].
		      G >> WorkGreenNumberOfShifts) << WorkGreenNumberOfUpShifts) |
		    ((ImageBuffer[c].B >> WorkBlueNumberOfShifts) << WorkBlueNumberOfUpShifts);
	    }
	break;
    case 16:
	for (b = 0; b < WorkHeight; b++)
	    for (a = 0; a < WorkWidth; a++) {
		c = a + b * DisplayWidth;
		TmpWrk[c] =
		    ((ImageBuffer[c].
		      R >> WorkRedNumberOfShifts) << WorkRedNumberOfUpShifts) | ((ImageBuffer[c].
										  G >>
										  WorkGreenNumberOfShifts)
										 <<
										 WorkGreenNumberOfUpShifts)
		    | ((ImageBuffer[c].B >> WorkBlueNumberOfShifts) << WorkBlueNumberOfUpShifts);

#if 0
		if (b == 0)
		    printf("%d=[%04x:%04x:%04x = %04x]  ", a,
			   ((ImageBuffer[c].R >> WorkRedNumberOfShifts) << WorkRedNumberOfUpShifts),
			   ((ImageBuffer[c].
			     G >> WorkGreenNumberOfShifts) << WorkGreenNumberOfUpShifts),
			   ((ImageBuffer[c].
			     B >> WorkBlueNumberOfShifts) << WorkBlueNumberOfUpShifts), TmpWrk[c]);
		fflush(stdout);
#endif

	    }
	break;
    case 24:
// I did this based on the VMware Linux station running under Windows2000
// Format is 2 pixels on 3 shorts : ggbb|BBrr|RRGG (rgb is the first pixel, RGB is the second pixel)  
	for (b = 0; b < WorkHeight; b++)
	    for (a = 0; a < WorkWidth; a++) {
		c = a + b * DisplayWidth;
		if (a % 2) {
		    d = (c * 3) >> 1;
		    TmpWrk[d++] |= ((ImageBuffer[c].B << 8) & 0xFF00);
		    TmpWrk[d] = ImageBuffer[c].G | (ImageBuffer[c].R << 8);
		}
		else {
		    d = (c * 3) >> 1;
		    TmpWrk[d++] = ImageBuffer[c].B | (ImageBuffer[c].G << 8);
		    TmpWrk[d++] = ImageBuffer[c].R;
		}

	    }
	break;
    case 32:
	for (b = 0; b < WorkHeight; b++)
	    for (a = 0; a < WorkWidth; a++) {
		c = a + b * DisplayWidth;
		d = c << 1;
#ifdef SWAP_RGB
		TmpWrk[d++] = ImageBuffer[c].B | (ImageBuffer[c].G << 8);
		TmpWrk[d] = ImageBuffer[c].R;
#else
		TmpWrk[d++] = ImageBuffer[c].R | (ImageBuffer[c].G << 8);
		TmpWrk[d] = ImageBuffer[c].B;
#endif
	    }
	break;
    }
    XPutImage(WorkDisplay, WorkPixmap, WorkPixmapGC, WorkImage, 0, 0, 0, 0, WorkWidth, WorkHeight);
    XCopyArea(WorkDisplay, WorkPixmap, WorkWindow, WorkWindowGC, 0, 0, WorkWidth, WorkHeight, 0, 0);

#ifdef DEBUG_BUFFERS
    printf(" ok.");
    fflush(stdout);
#endif

}

void
CopyDisplayBufferToScreen(int x, int y, int w, int h)
{
    register unsigned short *TmpWrk = (unsigned short *) WorkData;
    register unsigned char *TmpChrWrk = (unsigned char *) WorkData;
    register int a, b, c, d, wx = x, wy = y, wh = h, ww = w;


#ifdef DEBUG_BUFFERS
    printf("\nCopyDisplayBufferToScreen (%d,%d,%d,%d %d)", x, y, w, h, WorkBitsPerRGB);
    fflush(stdout);
#endif

    wx = 0, wy = 0;
    ww = DisplayWidth - 1;
    wh = DisplayHeight;

    if (wx < 0) {
	if ((ww -= (0 - wx)) <= 0)
	    return;		// nothing to draw...
	wx = 0;
    }
    if (wx >= DisplayWidth)
	return;
    if (wy < 0) {
	if ((wh -= (0 - wy)) <= 0)
	    return;
	wy = 0;
    }

    if (wh >= DisplayHeight)
	wh = DisplayHeight - 1;
    if (ww >= DisplayWidth)
	ww = DisplayWidth - 1;

    switch (WorkBitsPerRGB) {
    case 8:
	for (b = wy; b < wh; b++)
	    for (a = wx; a < ww; a++) {
		c = a + b * DisplayWidth;
		TmpChrWrk[c] =
		    (char) ((ImageBuffer[c].
			     R >> WorkRedNumberOfShifts) << WorkRedNumberOfUpShifts) |
		    ((ImageBuffer[c].
		      G >> WorkGreenNumberOfShifts) << WorkGreenNumberOfUpShifts) |
		    ((ImageBuffer[c].B >> WorkBlueNumberOfShifts) << WorkBlueNumberOfUpShifts);
	    }
	break;
    case 16:
	for (b = wy; b < wh; b++)
	    for (a = wx; a < ww; a++) {
		c = a + b * DisplayWidth;
		TmpWrk[c] =
		    ((ImageBuffer[c].
		      R >> WorkRedNumberOfShifts) << WorkRedNumberOfUpShifts) | ((ImageBuffer[c].
										  G >>
										  WorkGreenNumberOfShifts)
										 <<
										 WorkGreenNumberOfUpShifts)
		    | ((ImageBuffer[c].B >> WorkBlueNumberOfShifts) << WorkBlueNumberOfUpShifts);
	    }
	break;
    case 24:
	for (b = wy; b < wh; b++)
	    for (a = wx; a < ww; a++) {
		c = a + b * DisplayWidth;
		if (a % 2) {
		    d = (c * 3) >> 1;
		    TmpWrk[d++] |= ((ImageBuffer[c].B << 8) & 0xFF00);
		    TmpWrk[d] = ImageBuffer[c].G | (ImageBuffer[c].R << 8);
		}
		else {
		    d = (c * 3) >> 1;
		    TmpWrk[d++] = ImageBuffer[c].B | (ImageBuffer[c].G << 8);
		    TmpWrk[d++] = ImageBuffer[c].R;
		}

	    }
	break;
    case 32:
	for (b = wy; b < wh; b++)
	    for (a = wx; a < ww; a++) {
		c = a + b * DisplayWidth;
		d = c << 1;
#ifdef SWAP_RGB
		TmpWrk[d++] = ImageBuffer[c].B | (ImageBuffer[c].G << 8);
		TmpWrk[d] = ImageBuffer[c].R;
#else
		TmpWrk[d++] = ImageBuffer[c].R | (ImageBuffer[c].G << 8);
		TmpWrk[d] = ImageBuffer[c].B;
#endif
	    }
	break;
    }

//      for(b=0;b<DisplayWidth*6;b++) {TmpWrk[b*3]=0x0000;TmpWrk[b*3+1]=0x0000;TmpWrk[b*3+2]=0x0000;}

    XPutImage(WorkDisplay, WorkPixmap, WorkPixmapGC, WorkImage, wx, wy, wx, wy, ww, wh);
    XCopyArea(WorkDisplay, WorkPixmap, WorkWindow, WorkWindowGC, wx, wy, ww, wh, wx, wy);

#ifdef DEBUG_BUFFERS
    printf(" -> %d:%d %dx%d ok.", wx, wy, ww, wh);
    fflush(stdout);
#endif

}


void
CopyDisplayBufferRectangleToScreen(int x1, int y1, int x2, int y2)
{
    register int a, b, c, d, tx = x2, ty = y2;
    register unsigned short *TmpWrk = (unsigned short *) WorkData;
    register unsigned char *TmpChrWrk = (unsigned char *) WorkData;

#ifdef DEBUG_BUFFERS
    printf("\nCopyDisplayBufferRectangleToScreen (%d,%d,%d,%d)", x1, y1, x2, y2);
    fflush(stdout);
#endif

    switch (WorkBitsPerRGB) {
    case 8:
	for (b = y1; b < ty; b++)
	    for (a = x1; a < tx; a++) {
		c = a + b * DisplayWidth;
		TmpChrWrk[c] =
		    (char) ((ImageBuffer[c].
			     R >> WorkRedNumberOfShifts) << WorkRedNumberOfUpShifts) |
		    ((ImageBuffer[c].
		      G >> WorkGreenNumberOfShifts) << WorkGreenNumberOfUpShifts) |
		    ((ImageBuffer[c].B >> WorkBlueNumberOfShifts) << WorkBlueNumberOfUpShifts);
	    }
	break;
    case 16:
	for (b = y1; b < ty; b++)
	    for (a = x1; a < tx; a++) {
		c = a + b * DisplayWidth;
		TmpWrk[c] =
		    ((ImageBuffer[c].
		      R >> WorkRedNumberOfShifts) << WorkRedNumberOfUpShifts) | ((ImageBuffer[c].
										  G >>
										  WorkGreenNumberOfShifts)
										 <<
										 WorkGreenNumberOfUpShifts)
		    | ((ImageBuffer[c].B >> WorkBlueNumberOfShifts) << WorkBlueNumberOfUpShifts);
	    }
	break;
    case 24:
	for (b = y1; b < ty; b++)
	    for (a = x1; a < tx; a++) {
		c = a + b * DisplayWidth;
		if (a % 2) {
		    d = (c * 3) >> 1;
		    TmpWrk[d++] |= ((ImageBuffer[c].B << 8) & 0xFF00);
		    TmpWrk[d] = ImageBuffer[c].G | (ImageBuffer[c].R << 8);
		}
		else {
		    d = (c * 3) >> 1;
		    TmpWrk[d++] = ImageBuffer[c].B | (ImageBuffer[c].G << 8);
		    TmpWrk[d++] = ImageBuffer[c].R;
		}

	    }
	break;
    case 32:
	for (b = y1; b < ty; b++)
	    for (a = x1; a < tx; a++) {
		c = a + b * DisplayWidth;
		d = c << 1;
#ifdef SWAP_RGB
		TmpWrk[d++] = ImageBuffer[c].B | (ImageBuffer[c].G << 8);
		TmpWrk[d] = ImageBuffer[c].R;
#else
		TmpWrk[d++] = ImageBuffer[c].R | (ImageBuffer[c].G << 8);
		TmpWrk[d] = ImageBuffer[c].B;
#endif
	    }
	break;
    }
    XPutImage(WorkDisplay, WorkPixmap, WorkPixmapGC, WorkImage, x1, y1, x1, y1, x2 - x1, y2 - y1);
    XCopyArea(WorkDisplay, WorkPixmap, WorkWindow, WorkWindowGC, x1, y1, x2 - x1, y2 - y1, x1, y1);
}


void
CopyImageToWorkBuffer(int ctr)
{
    register int ssx, ssy, wpx, wpy, cw, cc, Sbp, Tbp;	// (s)creen(s)tartx/y (w)ork(p)icturexy (c)urrent(w)idth(c)omponent
    register float wsx, wsy;	// (w)ork(s)creenx/y  
    register int iwsx, iwsy, ww, wh, fx, fy;	// (i)ntegerpartof(w)ork(s)creenx/y  (w)ork(w)idth (w)ork(h)eight (f)irst block xy
    register int zct, zctx, zcty, wx, wy;	// zct = counter on zoom (pixel-size) (w)orkx/y for zooming
    register int ppsx, ppsy, ppex, ppey;	// Picture's Physical (s)tart/(e)nd x/y
    register int poslsx, poslsy, poslex, posley;	// Picture's OnScreen Logical (s)tart/(e)nd x/y (zoom included)
    register char *TmpCPtr, frx, fry, nofade = 0, masko;
    register float ZoomFactor, FadeFactor, tr, tg, tb;

#ifdef DEBUG_BUFFERS
    printf("\nCopyImageToWorkBuffer(%d) = 0x%0x", ctr, MyImages[ctr].Buffer);
    fflush(stdout);
#endif

    if (MyImages[ctr].Buffer == NULL)
	return;

    masko = (char) MyImages[SelectedImage].Mask;
    ww = WorkWidth;
    wh = WorkHeight;
    cw = MyImages[ctr].Width;
    cc = MyImages[ctr].NumOfComponent;
    ZoomFactor = MyImages[ctr].ZoomFactor;
    if ((FadeFactor = MyImages[ctr].FadeFactor) > (float) 0.95)
	nofade = 1;
    zct = (int) (ZoomFactor + (float) 0.9999);
    fx = zct;
    fy = zct;
    ppsx = MyImages[ctr].X;
    ppsy = MyImages[ctr].Y;
    ppex = MyImages[ctr].X + (int) ((float) MyImages[ctr].Width * ZoomFactor);
    ppey = MyImages[ctr].Y + (int) ((float) MyImages[ctr].Height * ZoomFactor);

    if ((ppsx > ww) || (ppex < 0) || (ppey < 0) || (ppsy > wh))	// not on logical screen
	return;
    if ((ssx = MyImages[ctr].X) < 0) {
	fx = zct + ((ssx + (int) ((float) ((int) ((float) ssx / ZoomFactor)) * ZoomFactor)) % zct);	// first cell x size on zoom for smooth zoom
	ssx = 0;
    }
    if ((ssy = MyImages[ctr].Y) < 0) {
	fy = zct + ((ssy + (int) ((float) ((int) ((float) ssy / ZoomFactor)) * ZoomFactor)) % zct);	// first cell y size on zoom for smooth zoom
	ssy = 0;
    }

    if ((poslsx = (int) ((float) (0 - MyImages[ctr].X) / ZoomFactor)) < 0)
	poslsx = 0;
    if ((poslsy = (int) ((float) (0 - MyImages[ctr].Y) / ZoomFactor)) < 0)
	poslsy = 0;

    if ((poslex = (int) ((float) (ww - MyImages[ctr].X) / ZoomFactor) + 1) > MyImages[ctr].Width)
	poslex = MyImages[ctr].Width;
    if ((posley = (int) ((float) (wh - MyImages[ctr].Y) / ZoomFactor) + 1) > MyImages[ctr].Height)
	posley = MyImages[ctr].Height;

    TmpCPtr = MyImages[ctr].Buffer;

    if (Verbose) {
	printf("\n%d. image = %d:%d %dx%d [%d] Zoom=%1.4f [%d]  ssx=%d ssy=%d", ctr,
	       MyImages[ctr].X, MyImages[ctr].Y, MyImages[ctr].Width, MyImages[ctr].Height,
	       MyImages[ctr].NumOfComponent, ZoomFactor, zct, ssx, ssy);
	fflush(stdout);
	printf("\n\t ppos = %d:%d - %d:%d spos = %d:%d - %d:%d  [%08x -> %08x]\n", ppsx, ppsy, ppex,
	       ppey, poslsx, poslsy, poslex, posley, TmpCPtr, ImageBuffer);
	fflush(stdout);
    }

    for (wsy = (float) ssy, wpy = poslsy, fry = 1; wpy < posley; wpy++, wsy += ZoomFactor) {
	Sbp = (poslsx + wpy * cw) * cc;
	if (!fry) {
	    iwsy = (int) wsy + fy - zct;
	    zcty = zct;
	}
	else {
	    iwsy = (int) wsy;
	    zcty = fy;
	}
	for (wsx = (float) ssx, wpx = poslsx, frx = 1; wpx < poslex; wpx++, wsx += ZoomFactor) {
	    if (!frx) {
		iwsx = (int) wsx + fx - zct;
		zctx = zct;
	    }
	    else {
		iwsx = (int) wsx;
		zctx = fx;
	    }
	    for (wy = 0; wy < zcty; wy++) {
		for (wx = 0; wx < zct; wx++) {
		    if (((iwsx + wx) < ww) && ((iwsy + wy) < wh)) {
			Tbp = iwsx + wx + ((iwsy + wy) * WorkWidth);
//printf("\n %d:%d [%d:%d] => [%d:%d]",Sbp,Tbp,wpx,wpy,iwsx+wx,(int)wsy+wy);fflush(stdout);
			if (nofade) {
			    if (masko & M_RED)
				ImageBuffer[Tbp].R = TmpCPtr[Sbp];
			    else
				ImageBuffer[Tbp].R = 0;
			    if (masko & M_GREEN)
				ImageBuffer[Tbp].G = TmpCPtr[Sbp + 1];
			    else
				ImageBuffer[Tbp].G = 0;
			    if (masko & M_BLUE)
				ImageBuffer[Tbp].B = TmpCPtr[Sbp + 2];
			    else
				ImageBuffer[Tbp].B = 0;
			}
			else {
			    if (masko & M_RED)
				tr = (float) ((unsigned char) ImageBuffer[Tbp].R);
			    else
				tr = 0.0f;
			    if (masko & M_GREEN)
				tg = (float) ((unsigned char) ImageBuffer[Tbp].G);
			    else
				tg = 0.0f;
			    if (masko & M_BLUE)
				tb = (float) ((unsigned char) ImageBuffer[Tbp].B);
			    else
				tb = 0.0f;
			    ImageBuffer[Tbp].R =
				(unsigned char) ((tr * (1.0f - FadeFactor)) +
						 ((float) ((unsigned char) TmpCPtr[Sbp]) *
						  FadeFactor));
			    ImageBuffer[Tbp].G =
				(unsigned char) ((tg * (1.0f - FadeFactor)) +
						 ((float) ((unsigned char) TmpCPtr[Sbp + 1]) *
						  FadeFactor));
			    ImageBuffer[Tbp].B =
				(unsigned char) ((tb * (1.0f - FadeFactor)) +
						 ((float) ((unsigned char) TmpCPtr[Sbp + 2]) *
						  FadeFactor));

//printf("\n%d:%d:%d %d:%d:%d %1.2f => %d:%d:%d  [%1.2f]",(unsigned char)tr,(unsigned char)tg,(unsigned char)tb,(unsigned char)TmpCPtr[Sbp],(unsigned char)TmpCPtr[Sbp+1],(unsigned char)TmpCPtr[Sbp+2],FadeFactor,(unsigned char)ImageBuffer[Tbp].R,(unsigned char)ImageBuffer[Tbp].G,(unsigned char)ImageBuffer[Tbp].B,((float)TmpCPtr[Sbp]*FadeFactor));fflush(stdout);

			}
		    }
		}
	    }
	    Sbp += cc;
	    frx = 0;
	}
	fry = 0;
    }

#ifdef DEBUG_BUFFERS
    printf(" ok.");
    fflush(stdout);
#endif

}

void
CopyImagesToWorkBuffer(void)
{
    int ctr;

    DrawTarget = 0;

    if (Verbose) {
	printf("\nWorkArea = %dx%d, Number of Images = %d", WorkWidth, WorkHeight, NumberOfImages);
	fflush(stdout);
    }

    for (ctr = 0; ctr < NumberOfImages; ctr++) {
	if (ctr != SelectedImage)
	    CopyImageToWorkBuffer(ctr);
    }

    if (SelectedImage != -1) {
	CopyImageToWorkBuffer(SelectedImage);
	GR_DrawRectangle(MyImages[SelectedImage].X - 1, MyImages[SelectedImage].Y - 1,
			 (int) ((float) MyImages[SelectedImage].Width *
				MyImages[SelectedImage].ZoomFactor) + MyImages[SelectedImage].X,
			 (int) ((float) MyImages[SelectedImage].Height *
				MyImages[SelectedImage].ZoomFactor) + MyImages[SelectedImage].Y, 0,
			 255, 0, 0);
    }
}

#define	DEF_BUT_HOR_PAD		4
#define	DEF_BUT_VER_PAD		3

int
CreateButton(char *name, void (*ClientFunction) (void *), void *ClientData, unsigned char AccChr,
	     int (*ClientKeyFunction) (int, char), unsigned char ImageManipulate, int x, int y,
	     int w, int h, int (*ButtonFunction) (int, int, char, unsigned char))
{
    ButtonStructure *TmpButton = NULL;

    if ((TmpButton =
	 (ButtonStructure *) realloc((void *) Buttons,
				     (size_t) (sizeof(struct ButtonStructure) *
					       (NumberOfButtons + 1)))) == NULL) {
	printf("\nError with Adding button!!!");
	fflush(stdout);
	return (1);
    }
    Buttons = TmpButton;
    sprintf(Buttons[NumberOfButtons].name, "%s", name);
    Buttons[NumberOfButtons].x = x;
    Buttons[NumberOfButtons].y = y;
    if (w != 0)
	Buttons[NumberOfButtons].w = w;
    else
	Buttons[NumberOfButtons].w =
	    GetMyStringLength(Buttons[NumberOfButtons].name, 0, 1) + (DEF_BUT_HOR_PAD << 1);
    if (h != 0)
	Buttons[NumberOfButtons].h = h;
    else
	Buttons[NumberOfButtons].h = 10 + (DEF_BUT_VER_PAD << 1);
    Buttons[NumberOfButtons].PushFunction = ClientFunction;
    Buttons[NumberOfButtons].ButtonFunction = ButtonFunction;
    Buttons[NumberOfButtons].PushClientData = ClientData;
    Buttons[NumberOfButtons].KeyFunction = ClientKeyFunction;
    Buttons[NumberOfButtons].PushChar = AccChr;
    Buttons[NumberOfButtons].ImageManipulate = ImageManipulate;
    ++NumberOfButtons;
    return (0);
}

#define	Bt_R	((unsigned char)0xFF)
#define	Bt_G	((unsigned char)0xFF)
#define	Bt_B	((unsigned char)0xFF)

#define	Bt_BgR	((unsigned char)0x00)
#define	Bt_BgG	((unsigned char)0x20)
#define	Bt_BgB	((unsigned char)0x10)

#define	Bts_R	((unsigned char)0xFF)
#define	Bts_G	((unsigned char)0x10)
#define	Bts_B	((unsigned char)0x20)
#define	Bts2_R	((unsigned char)0xFF)
#define	Bts2_G	((unsigned char)0xA0)
#define	Bts2_B	((unsigned char)0x20)

int
DrawButtons(void)
{
    register int a, b;

    DrawTarget = 1;

    GR_DrawRectangle(-2, -2, WorkWidth + 2, WorkHeight + 2, 1, 0x20, 0xA0, 0x80);
    GR_DrawRectangle(-1, -1, WorkWidth + 1, WorkHeight + 1, 1, 0x50, 0xFF, 0xA0);

    for (a = 0; a < NumberOfButtons; a++) {
	if (ActiveButton == a) {
	    GR_DrawRectangle(Buttons[a].x, Buttons[a].y, Buttons[a].x + Buttons[a].w,
			     Buttons[a].y + Buttons[a].h, 1, Bts_R, Bts_G, Bts_B);
	    GR_DrawRectangle(Buttons[a].x + 1, Buttons[a].y + 1, Buttons[a].x + Buttons[a].w - 1,
			     Buttons[a].y + Buttons[a].h - 1, 1, Bts2_R, Bts2_G, Bts2_B);
	}
	else {
	    GR_DrawRectangle(Buttons[a].x, Buttons[a].y, Buttons[a].x + Buttons[a].w,
			     Buttons[a].y + Buttons[a].h, 1, Bt_R, Bt_G, Bt_B);
	    GR_DrawRectangle(Buttons[a].x + 1, Buttons[a].y + 1, Buttons[a].x + Buttons[a].w - 1,
			     Buttons[a].y + Buttons[a].h - 1, 1, Bt_R, Bt_G, Bt_B);
	}
	GR_DrawFillRectangle(Buttons[a].x + 2, Buttons[a].y + 2, Buttons[a].x + Buttons[a].w - 2,
			     Buttons[a].y + Buttons[a].h - 2, 0, Bt_BgR, Bt_BgG, Bt_BgB);
	b = GetMyStringLength(Buttons[a].name, 0, 1);
	PutMyString(Buttons[a].name, Buttons[a].x + DEF_BUT_HOR_PAD,
		    Buttons[a].y + DEF_BUT_VER_PAD + 1, 0, 1);
    }
}

int
HandleButtonPush(int x, int y)
{
    register int a, w, h;

    DrawTarget = 1;

    for (a = 0; a < NumberOfButtons; a++) {
	if ((x >= Buttons[a].x) && (y >= Buttons[a].y) && ((Buttons[a].x + Buttons[a].w) >= x)
	    && ((Buttons[a].y + Buttons[a].h) >= y)) {
	    if (ActiveButton == a)	// deactivate
	    {
		GR_DrawRectangle(Buttons[a].x, Buttons[a].y, Buttons[a].x + Buttons[a].w,
				 Buttons[a].y + Buttons[a].h, 1, Bt_R, Bt_G, Bt_B);
		GR_DrawRectangle(Buttons[a].x + 1, Buttons[a].y + 1,
				 Buttons[a].x + Buttons[a].w - 1, Buttons[a].y + Buttons[a].h - 1,
				 1, Bt_R, Bt_G, Bt_B);
		ActiveButton = -1;
	    }
	    else {
		if ((SelectedImage != -1) || (!Buttons[a].ImageManipulate)) {
		    if (ActiveButton != -1) {
			GR_DrawRectangle(Buttons[ActiveButton].x, Buttons[ActiveButton].y,
					 Buttons[ActiveButton].x + Buttons[ActiveButton].w,
					 Buttons[ActiveButton].y + Buttons[ActiveButton].h, 1, Bt_R,
					 Bt_G, Bt_B);
			GR_DrawRectangle(Buttons[ActiveButton].x + 1, Buttons[ActiveButton].y + 1,
					 Buttons[ActiveButton].x + Buttons[ActiveButton].w - 1,
					 Buttons[ActiveButton].y + Buttons[ActiveButton].h - 1, 1,
					 Bt_R, Bt_G, Bt_B);
		    }
		    GR_DrawRectangle(Buttons[a].x, Buttons[a].y, Buttons[a].x + Buttons[a].w,
				     Buttons[a].y + Buttons[a].h, 1, Bts_R, Bts_G, Bts_B);
		    GR_DrawRectangle(Buttons[a].x + 1, Buttons[a].y + 1,
				     Buttons[a].x + Buttons[a].w - 1,
				     Buttons[a].y + Buttons[a].h - 1, 1, Bts2_R, Bts2_G, Bts2_B);
		    ActiveButton = a;
		    if (Buttons[a].PushFunction != NULL)
			Buttons[a].PushFunction(Buttons[a].PushClientData);
		}
	    }
	    CopyDisplayBufferToScreen(Buttons[ActiveButton].x - 2, Buttons[ActiveButton].y - 2,
				      Buttons[ActiveButton].w + 2, Buttons[ActiveButton].h + 2);
	    return (-1);
	}
    }
    if (y > WorkHeight)
	return (-1);		// Menu area

    if (SelectedImage != -1) {
	a = SelectedImage;
	w = (int) ((float) MyImages[a].Width * MyImages[a].ZoomFactor) + MyImages[a].X;
	h = (int) ((float) MyImages[a].Height * MyImages[a].ZoomFactor) + MyImages[a].Y;
	if ((x >= MyImages[a].X) && (x <= w) && (y >= MyImages[a].Y) && (y <= h)) {
	    return (a);
	}
    }

    for (a = 0; a < NumberOfImages; a++)	// check if on picture
    {
	w = (int) ((float) MyImages[a].Width * MyImages[a].ZoomFactor) + MyImages[a].X;
	h = (int) ((float) MyImages[a].Height * MyImages[a].ZoomFactor) + MyImages[a].Y;
	if ((x >= MyImages[a].X) && (x <= w) && (y >= MyImages[a].Y) && (y <= h)) {
	    return (a);
	}
    }
    return (-1);
}

void
HandleSystemKeys(int kmodifier, unsigned char Key)
{

//printf("\n%d [%d]",Key,kmodifier);fflush(stdout);

    switch (Key) {
    case SK_F1:
	if (SelectedImage != -1) {
	    MyImages[SelectedImage].X -=
		(int) ((float) MyImages[SelectedImage].HotPosX *
		       MyImages[SelectedImage].ZoomFactor);
	    MyImages[SelectedImage].Y -=
		(int) ((float) MyImages[SelectedImage].HotPosY *
		       MyImages[SelectedImage].ZoomFactor);
	    if ((MyImages[SelectedImage].ZoomFactor *= (float) 2.0) > MAX_ZOOMFACTOR)
		MyImages[SelectedImage].ZoomFactor = MAX_ZOOMFACTOR;
	    ClearWorkBuffer();
	    CopyImagesToWorkBuffer();
	    CopyWorkBufferToScreen();
	}
	break;
    case SK_F2:
	if (SelectedImage != -1) {
	    if ((MyImages[SelectedImage].ZoomFactor *= (float) 0.5) < MIN_ZOOMFACTOR)
		MyImages[SelectedImage].ZoomFactor = MIN_ZOOMFACTOR;
	    MyImages[SelectedImage].X +=
		(int) ((float) MyImages[SelectedImage].HotPosX *
		       MyImages[SelectedImage].ZoomFactor);
	    MyImages[SelectedImage].Y +=
		(int) ((float) MyImages[SelectedImage].HotPosY *
		       MyImages[SelectedImage].ZoomFactor);
	    ClearWorkBuffer();
	    CopyImagesToWorkBuffer();
	    CopyWorkBufferToScreen();
	}
	break;
    case SK_HOME:
	if (SelectedImage != -1) {
	    MyImages[SelectedImage].X = 0;
	    MyImages[SelectedImage].Y = 0;
	    ButtonPressedImageX = 0;
	    ButtonPressedImageY = 0;
	    MyImages[SelectedImage].HotPosX = 0;
	    MyImages[SelectedImage].HotPosY = 0;
	    ClearWorkBuffer();
	    CopyImagesToWorkBuffer();
	    CopyWorkBufferToScreen();
	    CopyDisplayBufferToScreen(0, 0, DisplayWidth, DisplayHeight);
	}
	break;

    case SK_END:
	if (SelectedImage != -1) {
	    MyImages[SelectedImage].X =
		(int) (((float) DisplayWidth -
			((float) MyImages[SelectedImage].Width *
			 MyImages[SelectedImage].ZoomFactor)) * 0.5f);
	    MyImages[SelectedImage].Y =
		(int) (((float) DisplayHeight -
			((float) MyImages[SelectedImage].Height *
			 MyImages[SelectedImage].ZoomFactor)) * 0.5f);
	    ButtonPressedImageX = 0;
	    ButtonPressedImageY = 0;
	    MyImages[SelectedImage].HotPosX = 0;
	    MyImages[SelectedImage].HotPosY = 0;
	    ClearWorkBuffer();
	    CopyImagesToWorkBuffer();
	    CopyWorkBufferToScreen();
	    CopyDisplayBufferToScreen(0, 0, DisplayWidth, DisplayHeight);
	}
	break;
    }
}

void
HandleButtonKey(int kmodifier, unsigned char Key)
{
    register int a, ret = 0;

    DrawTarget = 1;

//printf("\nHandleButtonKey : '%c' = %d  (mod = %d) Activebutton=%d",Key,Key,kmodifier,ActiveButton);fflush(stdout);

    if (ActiveButton == -1) {
	for (a = 0; a < NumberOfButtons; a++) {
	    if (Key == Buttons[a].PushChar) {
		if (ActiveButton == a)	// deactivate
		{
		    GR_DrawRectangle(Buttons[a].x, Buttons[a].y, Buttons[a].x + Buttons[a].w,
				     Buttons[a].y + Buttons[a].h, 1, Bt_R, Bt_G, Bt_B);
		    GR_DrawRectangle(Buttons[a].x + 1, Buttons[a].y + 1,
				     Buttons[a].x + Buttons[a].w - 1,
				     Buttons[a].y + Buttons[a].h - 1, 1, Bt_R, Bt_G, Bt_B);
		    ActiveButton = -1;
		}
		else {
		    if (ActiveButton != -1) {
			GR_DrawRectangle(Buttons[ActiveButton].x, Buttons[ActiveButton].y,
					 Buttons[ActiveButton].x + Buttons[ActiveButton].w,
					 Buttons[ActiveButton].y + Buttons[ActiveButton].h, 1, Bt_R,
					 Bt_G, Bt_B);
			GR_DrawRectangle(Buttons[ActiveButton].x + 1, Buttons[ActiveButton].y + 1,
					 Buttons[ActiveButton].x + Buttons[ActiveButton].w - 1,
					 Buttons[ActiveButton].y + Buttons[ActiveButton].h - 1, 1,
					 Bt_R, Bt_G, Bt_B);
		    }
		    GR_DrawRectangle(Buttons[a].x, Buttons[a].y, Buttons[a].x + Buttons[a].w,
				     Buttons[a].y + Buttons[a].h, 1, Bts_R, Bts_G, Bts_B);
		    GR_DrawRectangle(Buttons[a].x + 1, Buttons[a].y + 1,
				     Buttons[a].x + Buttons[a].w - 1,
				     Buttons[a].y + Buttons[a].h - 1, 1, Bts2_R, Bts2_G, Bts2_B);
		    ActiveButton = a;
		}
		CopyDisplayBufferToScreen(Buttons[a].x - 1, Buttons[a].y - 1, Buttons[a].w + 2,
					  Buttons[a].h + 2);
		if (Buttons[a].PushFunction != NULL)
		    Buttons[a].PushFunction(Buttons[a].PushClientData);
		return;
	    }
	}
    }
    else {
	if (Buttons[ActiveButton].KeyFunction != NULL) {
	    ret = Buttons[ActiveButton].KeyFunction(kmodifier, Key);
	}
    }
    if (ret == 0)
	HandleSystemKeys(kmodifier, Key);
}

int
InitializeNextImageStructure(void)
{
    int a = NumberOfImages;

    ++NumberOfImages;
    if ((MyImages =
	 (struct MyImagesStructure *) realloc((void *) MyImages,
					      (size_t) ((sizeof(struct MyImagesStructure)) *
							NumberOfImages))) == NULL) {
	printf("Error ReAllocating MyImages structure.");
	fflush(stdout);
	return (-1);
    }

#ifdef ALLOC_TRACK
    printf("\nA(r) : graphics : InitializeNextImageStructure : MyImages %08x (%d)", MyImages,
	   ((sizeof(struct MyImagesStructure)) * NumberOfImages));
    fflush(stdout);
#endif


    MyImages[a].Buffer = NULL;
    MyImages[a].BufferSize = 0;
    MyImages[a].X = 0;
    MyImages[a].Y = 0;
    MyImages[a].HotPosX = 0;
    MyImages[a].HotPosY = 0;
    MyImages[a].Width = 0;
    MyImages[a].Height = 0;

    MyImages[a].NumOfComponent = 3;
    MyImages[a].ScaleX = (float) 1.0;
    MyImages[a].ScaleY = (float) 1.0;
    MyImages[a].ScaleTargetWidth = -1;
    MyImages[a].ScaleTargetHeight = -1;
    MyImages[a].CropTargetStartX = -1;
    MyImages[a].CropTargetStartY = -1;

    MyImages[a].ZoomFactor = (float) 1.0;
    MyImages[a].FadeFactor = (float) 1.0;

    MyImages[a].Mask = M_RED | M_GREEN | M_BLUE | M_ALPHA;

    MyImages[a].OriginalBuffer = NULL;
    MyImages[a].OriginalBufferSize = 0;
    MyImages[a].OriginalX = 0;
    MyImages[a].OriginalY = 0;
    MyImages[a].OriginalWidth = 0;
    MyImages[a].OriginalHeight = 0;

    MyImages[a].ModifiedBuffer = NULL;
    MyImages[a].ModifiedBufferSize = 0;
    MyImages[a].ModifiedX = 0;
    MyImages[a].ModifiedY = 0;
    MyImages[a].ModifiedWidth = 0;
    MyImages[a].ModifiedHeight = 0;

    return (a);
}

int
UpdateImageToOriginal(int a)
{
    if (a >= NumberOfImages)
	return (-1);
    MyImages[a].Buffer = MyImages[a].OriginalBuffer;
    MyImages[a].BufferSize = MyImages[a].OriginalBufferSize;
    MyImages[a].X = MyImages[a].OriginalX;
    MyImages[a].Y = MyImages[a].OriginalY;
    MyImages[a].Width = MyImages[a].OriginalWidth;
    MyImages[a].Height = MyImages[a].OriginalHeight;
    if (MyImages[a].Buffer == NULL)
	return (-1);
    return (0);
}

int
MoveImageModifiedToOriginal(int a)
{
    if (a >= NumberOfImages)
	return (-1);

    if (MyImages[a].OriginalBuffer != NULL)
	free((void *) MyImages[a].OriginalBuffer);

    MyImages[a].OriginalBuffer = MyImages[a].ModifiedBuffer;
    MyImages[a].OriginalBufferSize = MyImages[a].ModifiedBufferSize;
    MyImages[a].OriginalX = MyImages[a].ModifiedX;
    MyImages[a].OriginalY = MyImages[a].ModifiedY;
    MyImages[a].OriginalWidth = MyImages[a].ModifiedWidth;
    MyImages[a].OriginalHeight = MyImages[a].ModifiedHeight;

    MyImages[a].ModifiedBuffer = NULL;
    MyImages[a].ModifiedBufferSize = 0;
    MyImages[a].ModifiedX = 0;
    MyImages[a].ModifiedY = 0;
    MyImages[a].ModifiedWidth = 0;
    MyImages[a].ModifiedHeight = 0;

    return (0);
}

int
UpdateImageToModified(int a)
{
    if (a >= NumberOfImages)
	return;
    if (MyImages[a].ModifiedBuffer == NULL)
	return (-1);

    MyImages[a].Buffer = MyImages[a].ModifiedBuffer;
    MyImages[a].BufferSize = MyImages[a].ModifiedBufferSize;
    MyImages[a].X = MyImages[a].ModifiedX;
    MyImages[a].Y = MyImages[a].ModifiedY;
    MyImages[a].Width = MyImages[a].ModifiedWidth;
    MyImages[a].Height = MyImages[a].ModifiedHeight;

    return (0);
}

int
GetHighestVisualPixmapCombination(void)
{
    int NumRet, a, b, NumVis, maxdpt, selected, wd, wid;
    XPixmapFormatValues *MyPFVInfo, *WorkWithThisPFV;
    XVisualInfo *MyVisInfo, *WorkWithThisVisInfo;


//printf("\nMain_Info = %d",Main_Info);fflush(stdout);

// First Let's see, what visuals does the system support...

    NumRet = 0;
    maxdpt = -1;
    MyVisInfo = XGetVisualInfo(WorkDisplay, VisualNoMask, NULL, &NumVis);
    if ((NumVis < 1) || (MyVisInfo == NULL)) {
	if (Main_Info) {
	    printf("\nXGetVisualInfo returned NO availability.");
	    fflush(stdout);
	}
	return (-3);
    }
    if (Main_Info) {
	printf("\nXGetVisualInfo returned with %d matches", NumVis);
	fflush(stdout);
	WorkWithThisVisInfo = MyVisInfo;
	for (b = 0; b < NumVis; b++) {
	    printf
		("\n\t%d. :\n\t\tVisual=%0x\n\t\tVidusalID=%d\n\t\tScreen=%d\n\t\tDepth=%d\n\t\tClass=%d\n\t\tMasks= %04x : %04x : %04x\n\t\tColormap Size=%d\n\t\tBits per RGB=%d",
		 b, WorkWithThisVisInfo->visual, WorkWithThisVisInfo->visualid, MyVisInfo->screen,
		 WorkWithThisVisInfo->depth, MyVisInfo->class, WorkWithThisVisInfo->screen,
		 WorkWithThisVisInfo->red_mask, WorkWithThisVisInfo->green_mask,
		 WorkWithThisVisInfo->blue_mask, WorkWithThisVisInfo->colormap_size,
		 WorkWithThisVisInfo->bits_per_rgb);
	    fflush(stdout);
	    WorkWithThisVisInfo++;
	}
	printf("\n\n");
	fflush(stdout);
    }

    if ((MyPFVInfo = XListPixmapFormats(WorkDisplay, &NumRet)) != NULL) {
	WorkWithThisPFV = MyPFVInfo;
	if (Main_Info) {
	    printf("\n%d pixmap formats", NumRet);
	    fflush(stdout);
	}
	for (a = 0; a < NumRet; a++) {
	    if (Main_Info) {
		printf("\n\t%d. Depth=%d  Bits_per_Pixel=%d  Scanline_pad=%d)", a,
		       WorkWithThisPFV->depth, WorkWithThisPFV->bits_per_pixel,
		       WorkWithThisPFV->scanline_pad);
		fflush(stdout);
	    }

	    WorkWithThisVisInfo = MyVisInfo;
	    for (b = 0; b < NumVis; b++) {
		if ((WorkWithThisVisInfo->depth == WorkWithThisPFV->depth)
		    && (WorkWithThisVisInfo->screen == WorkScreen)) {
		    if (Main_Info) {
			printf(" -> MATCH!");
			fflush(stdout);
		    }
		    maxdpt = WorkWithThisPFV->depth;
		    WorkScanLinePad = WorkWithThisPFV->scanline_pad;
		    WorkVisual = WorkWithThisVisInfo->visual;
		    wid = WorkWithThisVisInfo->visualid;
		    WorkDepth = WorkWithThisVisInfo->depth;
		    WorkRedMask = WorkWithThisVisInfo->red_mask;
		    WorkGreenMask = WorkWithThisVisInfo->green_mask;
		    WorkBlueMask = WorkWithThisVisInfo->blue_mask;
		    b = NumVis;
		}
	    }
	    ++WorkWithThisPFV;
	}
    }
    else {
	if (Main_Info) {
	    printf("\nXListPixmapFormats returned NULL.");
	    fflush(stdout);
	}
	return (-1);
    }
    if (selected == -1) {
	if (Main_Info) {
	    printf("\nDid not find suitable depth from XListPixmapFormats.");
	    fflush(stdout);
	}
	return (-2);
    }
    else {
	if (Main_Info) {
	    printf("\nSelected %d. with MaxDpt=%d", selected, maxdpt);
	    fflush(stdout);
	}
    }
    XFree(MyPFVInfo);
    XFree(MyVisInfo);

    selected = (unsigned int) WorkRedMask;
    a = 0;
    while (((selected & 1) == 0) && (a < 32)) {
	++a;
	selected >>= 1;
    }
    WorkRedNumberOfUpShifts = a;
    a = 0;
    while (((selected & 1) == 1) && (a < 32)) {
	++a;
	selected = selected >> 1;
    }
    WorkRedNumberOfShifts = 8 - a;
    selected = (unsigned int) WorkGreenMask;
    a = 0;
    while (((selected & 1) == 0) && (a < 32)) {
	++a;
	selected >>= 1;
    }
    WorkGreenNumberOfUpShifts = a;
    a = 0;
    while (((selected & 1) == 1) && (a < 32)) {
	++a;
	selected = selected >> 1;
    }
    WorkGreenNumberOfShifts = 8 - a;
    selected = (unsigned int) WorkBlueMask;
    a = 0;
    while (((selected & 1) == 0) && (a < 32)) {
	++a;
	selected >>= 1;
    }
    WorkBlueNumberOfUpShifts = a;
    a = 0;
    while (((selected & 1) == 1) && (a < 32)) {
	++a;
	selected = selected >> 1;
    }
    WorkBlueNumberOfShifts = 8 - a;


    if (Main_Info) {
	printf("\n\n Visual         = %08x", WorkVisual);
	printf("\n VisualID       = %d", wid);
	printf("\n WorkDepth      = %d", WorkDepth);
	printf("\n WorkRedMask    = %04x (%d) (%d)", WorkRedMask, WorkRedNumberOfShifts,
	       WorkRedNumberOfUpShifts);
	printf("\n WorkGreenMask  = %04x (%d) (%d)", WorkGreenMask, WorkGreenNumberOfShifts,
	       WorkGreenNumberOfUpShifts);
	printf("\n WorkBlueMask   = %04x (%d) (%d)", WorkBlueMask, WorkBlueNumberOfShifts,
	       WorkBlueNumberOfUpShifts);
	printf("\n WorkDepthShift = %d (%d)", (WorkDepth >> 3), sizeof(unsigned short));
	fflush(stdout);
	fflush(stdout);
    }

//      wd=WorkDepth;
    if ((wd = WorkDepth) == 24)
	wd = 32;
    if ((WorkData = (char *) malloc((wd >> 3) * DisplayWidth * DisplayHeight)) == NULL) {
	if (Main_Info) {
	    printf("\nCan not allocate %d bytes for WorkData.",
		   ((WorkDepth >> 3) * DisplayWidth * DisplayHeight));
	    fflush(stdout);
	}
	return (-4);
    }

#ifdef ALLOC_TRACK
    printf("\nA : graphics : GetHighestVisualPixmapCombination : WorkData %08x (%d)", WorkData,
	   ((wd >> 3) * DisplayWidth * DisplayHeight));
    fflush(stdout);
#endif

    if ((ImageBuffer =
	 (struct ImageBufferStructure *) malloc(sizeof(struct ImageBufferStructure) * DisplayWidth *
						DisplayHeight)) == NULL) {
	if (Main_Info) {
	    printf("\nCan not allocate %d bytes for ImageBuffer.",
		   (3 * DisplayWidth * DisplayHeight));
	    fflush(stdout);
	}
	return (-4);
    }

#ifdef ALLOC_TRACK
    printf("\nA : graphics : GetHighestVisualPixmapCombination : ImageBuffer %08x (%d)",
	   ImageBuffer, (sizeof(struct ImageBufferStructure) * DisplayWidth * DisplayHeight));
    fflush(stdout);
#endif

    if (Main_Info) {
	printf("\nXCreateImage");
	fflush(stdout);
    }

    if ((WorkImage =
	 XCreateImage(WorkDisplay, WorkVisual, WorkDepth, ZPixmap, 0, WorkData, DisplayWidth,
		      DisplayHeight, BitmapPad(WorkDisplay), 0)) == NULL)
//      if((WorkImage=XCreateImage(WorkDisplay,WorkVisual,WorkDepth,ZPixmap,0,WorkData,DisplayWidth,DisplayHeight,BitmapPad(WorkDisplay),0))==NULL)
    {
	if (Main_Info) {
	    printf("\nXCreateImage returned NULL.");
	    fflush(stdout);
	}
	return (-5);
    }

    if (Main_Info) {
	printf("->Ok.\nXInitImage");
	fflush(stdout);
    }

    if (XInitImage(WorkImage) == 0) {
	if (Main_Info) {
	    printf("\nError with XInitImage");
	    fflush(stdout);
	}
	return (-6);
    }

    if (Main_Info) {
	printf("->Ok.");
	fflush(stdout);
    }

    WorkBitsPerRGB = WorkImage->bits_per_pixel;

    if (Main_Info) {
	printf("\n WorkImage = %08x", WorkImage);
	printf("\n width = %d", WorkImage->width);
	printf("\n height = %d", WorkImage->height);
	printf("\n xoffset = %d", WorkImage->xoffset);
	printf("\n format = %d", WorkImage->format);
	printf("\n bitmap_unit = %d", WorkImage->bitmap_unit);
	printf("\n byte_order = %d", WorkImage->byte_order);
	printf("\n bitmap_bit_order = %d", WorkImage->bitmap_bit_order);
	printf("\n depth = %d", WorkImage->depth);
	printf("\n bytes_per_line = %d", WorkImage->bytes_per_line);
	printf("\n bits_per_pixel = %d", WorkImage->bits_per_pixel);
	printf("\n red_mask = %04x", WorkImage->red_mask);
	printf("\n green_mask = %04x", WorkImage->green_mask);
	printf("\n blue_mask = %04x", WorkImage->blue_mask);
	printf("\n WorkRedNumberOfShifts = %d", WorkRedNumberOfShifts);
	printf("\n WorkGreenNumberOfShifts = %d", WorkGreenNumberOfShifts);
	printf("\n WorkBlueNumberOfShifts = %d", WorkBlueNumberOfShifts);
	fflush(stdout);
    }

    return (0);
}

void
QuitApplication(void *Data)
{
    ProcessGo = 0;
}

int
CreateMainWindow(void)
{
    XSizeHints *s_h;
    XTextProperty winname, iconame;
    int xpos;

    DrawTarget = 1;

    if ((WorkDisplay = XOpenDisplay(NULL)) == NULL) {
	if ((WorkDisplay = XOpenDisplay(":0")) == NULL) {
	    if ((WorkDisplay = XOpenDisplay(":0.0")) == NULL) {
		printf("\nError opening display...");
		fflush(stdout);
		return (-1);
	    }
	}
    }
    WorkScreen = DefaultScreen(WorkDisplay);

    WorkPosX = ((XDisplayWidth(WorkDisplay, WorkScreen) - DisplayWidth) >> 1);
    WorkPosY = 30;

    if (GetHighestVisualPixmapCombination() < 0) {
	printf("\nError for getting Visual Information...");
	fflush(stdout);
	XCloseDisplay(WorkDisplay);
	return (-1);
    }

    if (Main_Info) {
	printf("\nXCreateWindow");
	fflush(stdout);
    }

    if ((WorkWindow =
	 XCreateWindow(WorkDisplay, RootWindow(WorkDisplay, WorkScreen), WorkPosX, WorkPosY,
		       DisplayWidth, DisplayHeight, 2, WorkDepth, InputOutput, WorkVisual, 0,
		       NULL)) == 0) {
	printf("\nError Creatig Window...");
	fflush(stdout);
	XCloseDisplay(WorkDisplay);
	return (-1);
    }

    if (Main_Info) {
	printf("->Ok.");
	fflush(stdout);
    }

    TmpChrPtrBuffer[0] = &TmpBuffer[0];
    TmpChrPtrBuffer[1] = NULL;
    sprintf(TmpBuffer, "%s", WINDOWNAME);
    if (XStringListToTextProperty(TmpChrPtrBuffer, 1, &winname)) {
	sprintf(TmpBuffer, "%s", ICONNAME);
	if (XStringListToTextProperty(TmpChrPtrBuffer, 1, &iconame)) {
	    if (!(s_h = XAllocSizeHints()))
		XSetWMProperties(WorkDisplay, WorkWindow, &winname, &iconame, NULL, 0, NULL, NULL,
				 NULL);
	    else {
		s_h->flags = PPosition | PSize | PMinSize | PMaxSize;
		s_h->min_width = DisplayWidth;
		s_h->max_width = DisplayWidth;
		s_h->width = DisplayWidth;
		s_h->min_height = DisplayHeight;
		s_h->max_height = DisplayHeight;
		s_h->height = DisplayHeight;
		XSetWMProperties(WorkDisplay, WorkWindow, &winname, &iconame, NULL, 0, s_h, NULL,
				 NULL);
		XResizeWindow(WorkDisplay, WorkWindow, DisplayWidth, DisplayHeight);
	    }
	}
    }

    xpos = 30;
    CreateButton("Quit", QuitApplication, NULL, (unsigned char) 'q', NULL, 0, DisplayWidth - xpos,
		 DisplayHeight - 21, 0, 0, NULL);
    xpos += 2;
    xpos += 38;
    CreateButton("Save", SavePicture, NULL, (unsigned char) 's', NULL, 1, DisplayWidth - xpos,
		 DisplayHeight - 21, 0, 0, NULL);
    xpos += 120;
    CreateButton("Process CAN Data", MiscellaneousButton, NULL, (unsigned char) ' ',
		 MiscellaneousKey, 1, DisplayWidth - xpos, DisplayHeight - 21, 0, 0, NULL);

    XMoveWindow(WorkDisplay, WorkWindow, WorkPosX, WorkPosY);
    XSelectInput(WorkDisplay, WorkWindow,
		 KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonMotionMask |
		 ButtonReleaseMask | PointerMotionMask | ExposureMask | StructureNotifyMask);
    XMapWindow(WorkDisplay, WorkWindow);

    return (0);
}

int
ReconfigureWindow(int width, int height)
{
    XGCValues mygcvalues;

    if (Main_Info) {
	printf("Reconfiguring to %dx%d...", width, height);
	fflush(stdout);
    }

    DisplayWidth = width;
    DisplayHeight = height;

    if (WorkPixmap != (Pixmap) 0)
	XFreePixmap(WorkDisplay, WorkPixmap);
    if (WorkPixmapGC != NULL)
	XFreeGC(WorkDisplay, WorkPixmapGC);

    if ((WorkData =
	 (char *) realloc((void *) WorkData,
			  (size_t) ((WorkDepth >> 3) * DisplayWidth * DisplayHeight))) == NULL) {
	printf("\nCan not reallocate %d bytes for WorkData.",
	       ((WorkDepth >> 3) * DisplayWidth * DisplayHeight));
	fflush(stdout);
	return (1);
    }

    if ((ImageBuffer =
	 (struct ImageBufferStructure *) realloc((void *) ImageBuffer,
						 (size_t) (sizeof(struct ImageBufferStructure) *
							   DisplayWidth * DisplayHeight))) ==
	NULL) {
	printf("\nCan not reallocate %d bytes for ImageBuffer.",
	       (3 * DisplayWidth * DisplayHeight));
	fflush(stdout);
	return (1);
    }

    WorkImage->width = DisplayWidth;
    WorkImage->height = DisplayHeight;
    WorkImage->data = WorkData;

    mygcvalues.function = GXcopy;
    mygcvalues.graphics_exposures = 1;

    WorkPixmap = XCreatePixmap(WorkDisplay, WorkWindow, DisplayWidth, DisplayHeight, WorkDepth);
    WorkPixmapGC =
	XCreateGC(WorkDisplay, WorkPixmap, GCFunction | GCGraphicsExposures, &mygcvalues);

    ClearDisplayBuffer();
    CopyImagesToWorkBuffer();
    DrawButtons();
    CopyDisplayBufferToScreen(0, 0, DisplayWidth, DisplayHeight);

    return (0);
}

void
MainLoop(void)
{
    XEvent report;
    XGCValues mygcvalues;
    register int a, b, h, noc;
    unsigned char buffer[20];
    int bufsize = 20;
    KeySym keysym;
    XComposeStatus compose;
    unsigned char *bp;
    float fv;


#ifdef DEBUG_MESSAGES
    printf("\n\nEntering MainLoop");
    fflush(stdout);
#endif

    while (ProcessGo) {
	XNextEvent(WorkDisplay, &report);
	switch (report.type) {
	case Expose:

#ifdef DEBUG_MESSAGES
	    printf("\n\nExpose %08x (%08x) %d:%d %dx%d", report.xexpose.window, WorkWindow,
		   report.xexpose.x, report.xexpose.y, report.xexpose.width, report.xexpose.height);
	    fflush(stdout);
#endif
	    if ((report.xexpose.count == 0)) {
		if (!WorkWindowInitialized) {
		    mygcvalues.function = GXcopy;
		    mygcvalues.graphics_exposures = 1;
		    WorkWindowGC =
			XCreateGC(WorkDisplay, WorkWindow, GCFunction | GCGraphicsExposures,
				  &mygcvalues);
		    WorkCursor = XCreateFontCursor(WorkDisplay, XC_hand2);
		    XDefineCursor(WorkDisplay, WorkWindow, WorkCursor);
		    WorkPixmap =
			XCreatePixmap(WorkDisplay, WorkWindow, DisplayWidth, DisplayHeight,
				      WorkDepth);
		    WorkPixmapGC =
			XCreateGC(WorkDisplay, WorkPixmap, GCFunction | GCGraphicsExposures,
				  &mygcvalues);
		    XClearWindow(WorkDisplay, WorkWindow);
		    WorkWindowInitialized = 1;
		    XMoveWindow(WorkDisplay, WorkWindow, WorkPosX, WorkPosY);
		    ClearDisplayBuffer();
		    CopyImagesToWorkBuffer();
		    DrawButtons();
		}
//                              CopyDisplayBufferToScreen(report.xexpose.x,report.xexpose.y,report.xexpose.width,report.xexpose.height);

//PutMyString("Alma Helloho ilinoise Alaska",30,260,0,2);

		CopyDisplayBufferToScreen(0, 0, DisplayWidth, DisplayHeight);
	    }
	    break;
	case ConfigureNotify:

#ifdef DEBUG_MESSAGES
	    printf("\n\nConfigureNotify %08x (%08x) %d:%d %dx%d", report.xconfigure.window,
		   WorkWindow, report.xconfigure.x, report.xconfigure.y, report.xconfigure.width,
		   report.xconfigure.height);
	    fflush(stdout);
#endif

	    if (WorkWindowInitialized) {
		if ((report.xconfigure.width != DisplayWidth)
		    || (report.xconfigure.height != DisplayHeight)) {
		    ReconfigureWindow(report.xconfigure.width, report.xconfigure.height);
		}
	    }

	    break;
	case KeyRelease:
	    if (report.xkey.window == WorkWindow) {
		switch ((int) XLookupKeysym((XKeyEvent *) & report, 0)) {
		case XK_Shift_L:
		    InternalKeyModifiers &= ((unsigned int) 0xFFFF - 1);
		    if (!(InternalKeyModifiers & 2))
			KeyModifiers &= ((unsigned char) 255 - (KM_SHIFT));
		    break;
		case XK_Shift_R:
		    InternalKeyModifiers &= ((unsigned int) 0xFFFF - 2);
		    if (!(InternalKeyModifiers & 1))
			KeyModifiers &= ((unsigned char) 255 - (KM_SHIFT));
		    break;
		case XK_Control_L:
		    InternalKeyModifiers &= ((unsigned int) 0xFFFF - 4);
		    if (!(InternalKeyModifiers & 8))
			KeyModifiers &= ((unsigned char) 255 - (KM_CTRL));
		    break;
		case XK_Control_R:
		    InternalKeyModifiers &= ((unsigned int) 0xFFFF - 8);
		    if (!(InternalKeyModifiers & 4))
			KeyModifiers &= ((unsigned char) 255 - (KM_CTRL));
		    break;
		case XK_Alt_L:
		    InternalKeyModifiers &= ((unsigned int) 0xFFFF - 16);
		    if (!(InternalKeyModifiers & 32))
			KeyModifiers &= ((unsigned char) 255 - (KM_ALT));
		    break;
		case XK_Alt_R:
		    InternalKeyModifiers &= ((unsigned int) 0xFFFF - 32);
		    if (!(InternalKeyModifiers & 16))
			KeyModifiers &= ((unsigned char) 255 - (KM_ALT));
		    break;
		}
		if (!XLookupString
		    ((XKeyEvent *) & report, (char *) buffer, bufsize, &keysym, &compose))
		    buffer[0] = 0;

		if (buffer[0] == 'z')	// distance
		{
		    if ((SelectedImage >= 0) && (MyImages[SelectedImage].HotPosX >= 0)) {
			if (SelectedImage != ZPressSel) {
			    ZPressSel = SelectedImage;
			    ZPressX = MyImages[SelectedImage].HotPosX;
			    ZPressY = MyImages[SelectedImage].HotPosY;
			}
			else {
			    fv = ((float) ZPressX -
				  (float) MyImages[SelectedImage].HotPosX) * ((float) ZPressX -
									      (float)
									      MyImages
									      [SelectedImage].
									      HotPosX);
			    fv +=
				((float) ZPressY -
				 (float) MyImages[SelectedImage].HotPosY) * ((float) ZPressY -
									     (float)
									     MyImages
									     [SelectedImage].
									     HotPosY);
			    fv = sqrt(fv);

			    printf("\nDistance %d:%d -> %d:%d = %1.2f  [%d:%d]", ZPressX, ZPressY,
				   MyImages[SelectedImage].HotPosX, MyImages[SelectedImage].HotPosY,
				   fv, ABS((MyImages[SelectedImage].HotPosX - ZPressX)),
				   ABS((MyImages[SelectedImage].HotPosY - ZPressY)));
			    fflush(stdout);

			    ZPressX = MyImages[SelectedImage].HotPosX;
			    ZPressY = MyImages[SelectedImage].HotPosY;
			}
		    }
		}

	    }
#ifdef DEBUG_MESSAGES
	    printf("\nKeyRelease = %08x (%08x)[%d] = %d %04x", report.xkey.window, WorkWindow,
		   KeyModifiers, (int) XLookupKeysym((XKeyEvent *) & report, 0),
		   (int) XLookupKeysym((XKeyEvent *) & report, 0));
	    fflush(stdout);
#endif

	    break;
	case KeyPress:
	    if (report.xkey.window == WorkWindow) {
		buffer[0] = 0;
		switch ((int) XLookupKeysym((XKeyEvent *) & report, 0)) {
		case XK_Shift_L:
		    InternalKeyModifiers |= ((unsigned int) 1);
		    KeyModifiers |= ((unsigned char) (KM_SHIFT));
		    break;
		case XK_Shift_R:
		    InternalKeyModifiers |= ((unsigned int) 2);
		    KeyModifiers |= ((unsigned char) (KM_SHIFT));
		    break;
		case XK_Control_L:
		    InternalKeyModifiers |= ((unsigned int) 4);
		    KeyModifiers |= ((unsigned char) (KM_CTRL));
		    break;
		case XK_Control_R:
		    InternalKeyModifiers |= ((unsigned int) 8);
		    KeyModifiers |= ((unsigned char) (KM_CTRL));
		    break;
		case XK_Alt_L:
		    InternalKeyModifiers |= ((unsigned int) 16);
		    KeyModifiers |= ((unsigned char) (KM_ALT));
		    break;
		case XK_Alt_R:
		    InternalKeyModifiers |= ((unsigned int) 32);
		    KeyModifiers |= ((unsigned char) (KM_ALT));
		    break;

		case XK_Left:
		    buffer[0] = (unsigned char) SK_LFAR;
		    break;
		case XK_Up:
		    buffer[0] = (unsigned char) SK_UPAR;
		    break;
		case XK_Down:
		    buffer[0] = (unsigned char) SK_DNAR;
		    break;
		case XK_Right:
		    buffer[0] = (unsigned char) SK_RIAR;
		    break;
		case XK_Home:
		    buffer[0] = (unsigned char) SK_HOME;
		    break;
		case XK_Page_Up:
		    buffer[0] = (unsigned char) SK_PGUP;
		    break;
		case XK_Page_Down:
		    buffer[0] = (unsigned char) SK_PGDN;
		    break;
		case XK_End:
		    buffer[0] = (unsigned char) SK_END;
		    break;
		case XK_F1:
		    buffer[0] = (unsigned char) SK_F1;
		    break;
		case XK_F2:
		    buffer[0] = (unsigned char) SK_F2;
		    break;
		case XK_F3:
		    buffer[0] = (unsigned char) SK_F3;
		    break;
		case XK_F4:
		    buffer[0] = (unsigned char) SK_F4;
		    break;
		case XK_F5:
		    buffer[0] = (unsigned char) SK_F5;
		    break;
		case XK_F6:
		    buffer[0] = (unsigned char) SK_F6;
		    break;
		case XK_F7:
		    buffer[0] = (unsigned char) SK_F7;
		    break;
		case XK_F8:
		    buffer[0] = (unsigned char) SK_F8;
		    break;
		case XK_F9:
		    buffer[0] = (unsigned char) SK_F9;
		    break;
		case XK_F10:
		    buffer[0] = (unsigned char) SK_F10;
		    break;
		case XK_F11:
		    buffer[0] = (unsigned char) SK_F11;
		    break;
		case XK_F12:
		    buffer[0] = (unsigned char) SK_F12;
		    break;
		case XK_Tab:
		    buffer[0] = (unsigned char) 9;
		    break;
		}

		if (buffer[0] == 0)
		    if (!XLookupString
			((XKeyEvent *) & report, (char *) buffer, bufsize, &keysym, &compose))
			buffer[0] = 0;

		if (buffer[0] != 0)
		    HandleButtonKey(KeyModifiers, buffer[0]);

#ifdef DEBUG_MESSAGES
		printf("\nKeyPress = %08x (%08x) [%d] = %d %04x => '%c' '%d'", report.xkey.window,
		       WorkWindow, KeyModifiers, (int) XLookupKeysym((XKeyEvent *) & report, 0),
		       (int) XLookupKeysym((XKeyEvent *) & report, 0), buffer[0],
		       (unsigned char) buffer[0]);
		fflush(stdout);
#endif

	    }
	    break;
	case MotionNotify:
	    while (XCheckMaskEvent(WorkDisplay, ButtonMotionMask, &report));

// printf("\n%d:%d [%d]",report.xmotion.x,report.xmotion.y,ButtonPressed);fflush(stdout);

	    switch (ButtonPressed) {
	    case 0:
		if (SelectedImage != -1) {
		    a = ((float) (report.xmotion.x - MyImages[SelectedImage].X) /
			 MyImages[SelectedImage].ZoomFactor);
		    b = ((float) (report.xmotion.y - MyImages[SelectedImage].Y) /
			 MyImages[SelectedImage].ZoomFactor);
		    if ((a < 0) || (a >= MyImages[SelectedImage].Width))
			a = -1;
		    if ((b < 0) || (b >= MyImages[SelectedImage].Height))
			b = -1;
		    if (a == -1)
			b = -1;
		    else if (b == -1)
			a = -1;
		    MyImages[SelectedImage].HotPosX = a;
		    MyImages[SelectedImage].HotPosY = b;
		}
		break;
	    case 1:		// Left button
		if (SelectedImage != -1) {
		    if ((KeyModifiers == 2) && (Buttons[ActiveButton].ButtonFunction != NULL)) {
			a = ((float) (report.xmotion.x - MyImages[SelectedImage].X) /
			     MyImages[SelectedImage].ZoomFactor);
			b = ((float) (report.xmotion.y - MyImages[SelectedImage].Y) /
			     MyImages[SelectedImage].ZoomFactor);
			if ((a < 0) || (a >= MyImages[SelectedImage].Width))
			    a = -1;
			if ((b < 0) || (b >= MyImages[SelectedImage].Height))
			    b = -1;
			if (a == -1)
			    b = -1;
			else if (b == -1)
			    a = -1;
			if ((MyImages[SelectedImage].HotPosX != a)
			    || (MyImages[SelectedImage].HotPosY != b)) {
			    MyImages[SelectedImage].HotPosX = a;
			    MyImages[SelectedImage].HotPosY = b;
			    Buttons[ActiveButton].ButtonFunction((int) a, (int) b,
								 (char) (report.xbutton.button),
								 KeyModifiers);
			}
		    }
		    else {
			a = MyImages[SelectedImage].X;
			b = MyImages[SelectedImage].Y;
			MyImages[SelectedImage].X =
			    ButtonPressedImageX + (int) (report.xmotion.x) - ButtonPressedX;
			MyImages[SelectedImage].Y =
			    ButtonPressedImageY + (int) (report.xmotion.y) - ButtonPressedY;
/*
							w=MyImages[SelectedImage].Width+ABS(a-MyImages[SelectedImage].X);
							h=MyImages[SelectedImage].Height+ABS(b-MyImages[SelectedImage].Y);
							if(a>MyImages[SelectedImage].X)
								a=MyImages[SelectedImage].X;
							if(b>MyImages[SelectedImage].Y)
								b=MyImages[SelectedImage].Y;
							ReDrawWorkBuffer(a,b,w,h);
*/
			ClearWorkBuffer();
			CopyImagesToWorkBuffer();
			CopyWorkBufferToScreen();
		    }
		}
		break;
	    }
	    break;
	case ButtonPress:
	    h = 1;
	    ButtonPressed |= (1 << (unsigned char) (report.xbutton.button - 1));
	    if ((ActiveButton != -1) && (SelectedImage != -1)) {
		if (Buttons[ActiveButton].ButtonFunction != NULL) {
		    a = ((float) (report.xmotion.x - MyImages[SelectedImage].X) /
			 MyImages[SelectedImage].ZoomFactor);
		    b = ((float) (report.xmotion.y - MyImages[SelectedImage].Y) /
			 MyImages[SelectedImage].ZoomFactor);
		    if ((a < 0) || (a >= MyImages[SelectedImage].Width))
			a = -1;
		    if ((b < 0) || (b >= MyImages[SelectedImage].Height))
			b = -1;
		    if (a == -1)
			b = -1;
		    else if (b == -1)
			a = -1;
		    MyImages[SelectedImage].HotPosX = a;
		    MyImages[SelectedImage].HotPosY = b;
		    if (Buttons[ActiveButton].
			ButtonFunction((int) a, (int) b, (char) (report.xbutton.button),
				       KeyModifiers))
			h = 0;
		}
	    }

	    if (h)		// Was not processed
	    {
		if (ButtonPressed == 1) {
		    ButtonPressedX = (int) (report.xbutton.x);
		    ButtonPressedY = (int) (report.xbutton.y);
		    if ((a = HandleButtonPush((int) (report.xbutton.x), (int) (report.xbutton.y))) >= 0)	// click on image
		    {
			if (SelectedImage != a)
			    PreviousSelectedImage = SelectedImage;
			SelectedImage = a;
			ButtonPressedImageX = MyImages[SelectedImage].X;
			ButtonPressedImageY = MyImages[SelectedImage].Y;
			a = ((float) (report.xbutton.x - MyImages[SelectedImage].X) /
			     MyImages[SelectedImage].ZoomFactor);
			b = ((float) (report.xbutton.y - MyImages[SelectedImage].Y) /
			     MyImages[SelectedImage].ZoomFactor);
			if ((a < 0) || (a >= MyImages[SelectedImage].Width))
			    a = -1;
			if ((b < 0) || (b >= MyImages[SelectedImage].Height))
			    b = -1;
			if (a == -1)
			    b = -1;
			else if (b == -1)
			    a = -1;
			MyImages[SelectedImage].HotPosX = a;
			MyImages[SelectedImage].HotPosY = b;
			if (MyImages[SelectedImage].Name[0] == '%') {
			    noc = MyImages[SelectedImage].NumOfComponent;
			    bp = (unsigned char *) MyImages[SelectedImage].Buffer;
			    DRed = bp[a * noc + b * MyImages[SelectedImage].Width * noc];
			    DGreen = bp[a * noc + b * MyImages[SelectedImage].Width * noc + 1];
			    DBlue = bp[a * noc + b * MyImages[SelectedImage].Width * noc + 2];
			}

			ClearWorkBuffer();
			CopyImagesToWorkBuffer();
			CopyWorkBufferToScreen();
			CopyDisplayBufferToScreen(0, 0, DisplayWidth, DisplayHeight);
		    }
		}
	    }
#ifdef DEBUG_MESSAGES
	    printf("\n%d %d [%d / %d]", (unsigned int) (report.xbutton.x),
		   (unsigned int) (report.xbutton.y), (unsigned char) (report.xbutton.button),
		   ButtonPressed);
	    fflush(stdout);
#endif
	    break;
	case ButtonRelease:

	    ButtonPressed &= 0xFF - (1 << (unsigned char) (report.xbutton.button - 1));

	    if ((report.xbutton.x > (DisplayWidth - 10))
		&& (report.xbutton.y > (DisplayHeight - 10)))
		return;

#if 0
	    (unsigned int) (report.xbutton.x);
	    (unsigned int) (report.xbutton.y);
	    (unsigned char) (report.xbutton.button);
#endif
	    break;
	default:
	    break;
	}
    }				/* while */
}

char *
CreatePicture(unsigned int *ret_width, unsigned int *ret_height, int *SrcBufferSize)
{
    int x, y, w, h, bs;
    char *TB;

    w = 32;
    h = 32;
    bs = 3 * w * h;

    if ((TB = (char *) malloc((size_t) bs)) == NULL) {
	printf("\nCan NOT allocate memory for %d bytes (%dx%d)", bs, w, h);
	return (NULL);
    }

    for (y = 0; y < h; y++) {
	for (x = 0; x < w; x++) {
	    TB[x * 3 + y * w * 3] = (unsigned char) x << 3;
	    TB[x * 3 + y * w * 3 + 1] = (unsigned char) y << 3;
	    TB[x * 3 + y * w * 3 + 2] = (unsigned char) 0;
	}
    }

    *ret_width = w;
    *ret_height = h;
    *SrcBufferSize = bs;
    return (TB);
}
