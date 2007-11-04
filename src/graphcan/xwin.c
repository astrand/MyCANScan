/*
    CAN data visualizer for the '04 Toyota Prius on a Sharp SL-C700
    X11 user interface code
    Copyright (C) 2004-2005 Attila Vass

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
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "images.h"
#include "ui.h"

// FIXME: eliminate/move as many as possible
Window WorkWindow = 0;
Display *WorkDisplay;
int WorkScreen, WorkDepth, WorkBitsPerRGB, WorkScanLinePad;
unsigned long WorkRedMask, WorkGreenMask, WorkBlueMask;
unsigned int WorkRedNumberOfShifts, WorkGreenNumberOfShifts, WorkBlueNumberOfShifts;
unsigned int WorkRedNumberOfUpShifts, WorkGreenNumberOfUpShifts, WorkBlueNumberOfUpShifts;
Visual *WorkVisual = NULL;
XImage *WorkImage = NULL;
char WorkWindowInitialized = 0;
GC WorkWindowGC = 0, WorkPixmapGC = 0;
Pixmap WorkPixmap = 0;
char NextStep = 2;

extern char *WorkData;
extern struct ImageBufferStructure *ImageBuffer;
#define	WIDTH	640
#define	HEIGHT	480



int
GetHighestVisualPixmapCombination(void)
{
    int NumRet, a, b, NumVis, maxdpt, selected = 0, wd, wid;
    XPixmapFormatValues *MyPFVInfo, *WorkWithThisPFV;
    XVisualInfo *MyVisInfo, *WorkWithThisVisInfo;

// First Let's see, what visuals does the system support...

    NumRet = 0;
    maxdpt = -1;
    MyVisInfo = XGetVisualInfo(WorkDisplay, VisualNoMask, NULL, &NumVis);
    if ((NumVis < 1) || (MyVisInfo == NULL)) {
	printf("\nXGetVisualInfo returned NO availability.");
	fflush(stdout);
	return (-3);
    }

    if ((MyPFVInfo = XListPixmapFormats(WorkDisplay, &NumRet)) != NULL) {
	WorkWithThisPFV = MyPFVInfo;
	for (a = 0; a < NumRet; a++) {
	    WorkWithThisVisInfo = MyVisInfo;
	    for (b = 0; b < NumVis; b++) {
		if ((WorkWithThisVisInfo->depth == WorkWithThisPFV->depth)
		    && (WorkWithThisVisInfo->screen == WorkScreen)) {
		    printf(" -> MATCH!");
		    fflush(stdout);
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
	printf("\nXListPixmapFormats returned NULL.");
	fflush(stdout);
	return (-1);
    }
    if (selected == -1) {
	printf("\nDid not find suitable depth from XListPixmapFormats.");
	fflush(stdout);
	return (-2);
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

//      wd=WorkDepth;
    if ((wd = WorkDepth) == 24)
	wd = 32;
    if ((WorkData = (char *) malloc((wd >> 3) * WIDTH * HEIGHT)) == NULL) {
	printf("\nCan not allocate %d bytes for WorkData.", ((WorkDepth >> 3) * WIDTH * HEIGHT));
	fflush(stdout);
	return (-4);
    }

    if ((ImageBuffer =
	 (struct ImageBufferStructure *) malloc(sizeof(struct ImageBufferStructure) * WIDTH *
						HEIGHT)) == NULL) {
	printf("\nCan not allocate %d bytes for ImageBuffer.", (3 * WIDTH * HEIGHT));
	fflush(stdout);
	return (-4);
    }

    printf("\nXCreateImage");
    fflush(stdout);

    if ((WorkImage =
	 XCreateImage(WorkDisplay, WorkVisual, WorkDepth, ZPixmap, 0, WorkData, WIDTH, HEIGHT,
		      BitmapPad(WorkDisplay), 0)) == NULL)
//      if((WorkImage=XCreateImage(WorkDisplay,WorkVisual,WorkDepth,ZPixmap,0,WorkData,WIDTH,HEIGHT,BitmapPad(WorkDisplay),0))==NULL)
    {
	printf("\nXCreateImage returned NULL.");
	fflush(stdout);
	return (-5);
    }

    printf("->Ok.\nXInitImage");
    fflush(stdout);

    if (XInitImage(WorkImage) == 0) {
	printf("\nError with XInitImage");
	fflush(stdout);
	return (-6);
    }

    WorkBitsPerRGB = WorkImage->bits_per_pixel;

    printf("->Ok. (%d)", WorkBitsPerRGB);
    fflush(stdout);

    return (0);
}


void
CopyDisplayBufferToScreen(int x, int y, int w, int h)
{
    register unsigned short *TmpWrk = (unsigned short *) WorkData;
    register int a, b, c, d, wx = x, wy = y, wh = (h + y), ww = w;

    if (!WorkWindowInitialized)
	return;

    wx = 0, wy = 0;
    ww = WIDTH - 1;
    wh = HEIGHT;

    if (wx < 0) {
	if ((ww -= (0 - wx)) <= 0)
	    return;		// nothing to draw...
	wx = 0;
    }
    if (wx >= WIDTH)
	return;
    if (wy < 0) {
	if ((wh -= (0 - wy)) <= 0)
	    return;
	wy = 0;
    }

    if (wh >= HEIGHT)
	wh = HEIGHT - 1;
    if (ww >= WIDTH)
	ww = WIDTH - 1;

    switch (WorkBitsPerRGB) {
    case 16:
	for (b = wy; b < wh; b++)
	    for (a = wx; a < ww; a++) {
		c = a + b * WIDTH;
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
    case 32:
	for (b = wy; b < wh; b++)
	    for (a = wx; a < ww; a++) {
		c = a + b * WIDTH;
		d = c << 1;
		TmpWrk[d++] = ImageBuffer[c].B | (ImageBuffer[c].G << 8);
		TmpWrk[d] = ImageBuffer[c].R;
	    }
	break;
    }
    XPutImage(WorkDisplay, WorkPixmap, WorkPixmapGC, WorkImage, wx, wy, wx, wy, ww, wh);
    XCopyArea(WorkDisplay, WorkPixmap, WorkWindow, WorkWindowGC, wx, wy, ww, wh, wx, wy);
    XFlush(WorkDisplay);
}



void ui_main_loop()
{
    XEvent report;
    XGCValues mygcvalues;

    while ((XEventsQueued(WorkDisplay, QueuedAfterFlush)) != 0) {
	XNextEvent(WorkDisplay, &report);
	switch (report.type) {
	case Expose:
	    if ((report.xexpose.count == 0)) {
		if (!WorkWindowInitialized) {
		    mygcvalues.function = GXcopy;
		    mygcvalues.graphics_exposures = 1;
		    WorkWindowGC =
			XCreateGC(WorkDisplay, WorkWindow, GCFunction | GCGraphicsExposures,
				  &mygcvalues);
		    WorkPixmap = XCreatePixmap(WorkDisplay, WorkWindow, WIDTH, HEIGHT, WorkDepth);
		    WorkPixmapGC =
			XCreateGC(WorkDisplay, WorkPixmap, GCFunction | GCGraphicsExposures,
				  &mygcvalues);
		    XClearWindow(WorkDisplay, WorkWindow);
		    WorkWindowInitialized = 1;
		    XMoveWindow(WorkDisplay, WorkWindow, 0, 0);
		}

//PutMyString("Alma Helloho ilinoise Alaska",30,260,0,2);

		CopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
	    }
	    break;
	default:
	    break;
	}
    }				/* while */
}

int ui_create_window()
{
    XSizeHints *s_h;
    XTextProperty winname, iconame;
    char TmpBuffer[64];
    char *TmpChrPtrBuffer[2];

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

    if (GetHighestVisualPixmapCombination() < 0) {
	printf("\nError for getting Visual Information...");
	fflush(stdout);
	XCloseDisplay(WorkDisplay);
	return (-1);
    }

    printf("\nXCreateWindow");
    fflush(stdout);

    if ((WorkWindow =
	 XCreateWindow(WorkDisplay, RootWindow(WorkDisplay, WorkScreen), 0, 0, WIDTH, HEIGHT, 2,
		       WorkDepth, InputOutput, WorkVisual, 0, NULL)) == 0) {
	printf("\nError Creatig Window...");
	fflush(stdout);
	XCloseDisplay(WorkDisplay);
	return (-1);
    }

    printf("->Ok.");
    fflush(stdout);

    TmpChrPtrBuffer[0] = &TmpBuffer[0];
    TmpChrPtrBuffer[1] = NULL;
    sprintf(TmpBuffer, "GraphCan - Linux");
    if (XStringListToTextProperty(TmpChrPtrBuffer, 1, &winname)) {
	sprintf(TmpBuffer, "Graphcan");
	if (XStringListToTextProperty(TmpChrPtrBuffer, 1, &iconame)) {
	    if (!(s_h = XAllocSizeHints()))
		XSetWMProperties(WorkDisplay, WorkWindow, &winname, &iconame, NULL, 0, NULL, NULL,
				 NULL);
	    else {
		s_h->flags = PPosition | PSize | PMinSize | PMaxSize;
		s_h->min_width = WIDTH;
		s_h->max_width = WIDTH;
		s_h->width = WIDTH;
		s_h->min_height = HEIGHT;
		s_h->max_height = HEIGHT;
		s_h->height = HEIGHT;
		XSetWMProperties(WorkDisplay, WorkWindow, &winname, &iconame, NULL, 0, s_h, NULL,
				 NULL);
		XResizeWindow(WorkDisplay, WorkWindow, WIDTH, HEIGHT);
	    }
	}
    }
    XMoveWindow(WorkDisplay, WorkWindow, 0, 0);
    XSelectInput(WorkDisplay, WorkWindow, ExposureMask);
    XMapWindow(WorkDisplay, WorkWindow);

    ui_main_loop();

    return (0);
}


