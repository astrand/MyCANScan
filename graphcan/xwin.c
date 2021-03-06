/*
    CAN data visualizer for the Toyota Prius
    X11 user interface code

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

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "images.h"
#include "ui.h"
#include "graphcan.h"

static Window WorkWindow = 0;
static Display *WorkDisplay;
static int WorkScreen, WorkDepth, WorkBitsPerRGB, WorkScanLinePad;
static unsigned long WorkRedMask, WorkGreenMask, WorkBlueMask;
static unsigned int WorkRedNumberOfShifts, WorkGreenNumberOfShifts, WorkBlueNumberOfShifts;
static unsigned int WorkRedNumberOfUpShifts, WorkGreenNumberOfUpShifts, WorkBlueNumberOfUpShifts;
static Visual *WorkVisual = NULL;
static XImage *WorkImage = NULL;
static char WorkWindowInitialized = 0;
static GC WorkWindowGC = 0, WorkPixmapGC = 0;
static Pixmap WorkPixmap = 0;

extern char *WorkData;
extern struct ImageBufferStructure *ImageBuffer;
extern unsigned char FullScreenMode;

/* X11 window size, possibly larger than UI size */
static int width = WIDTH;
static int height = HEIGHT;

static int
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
        fprintf(stderr, "XGetVisualInfo returned NO availability.\n");
        return (-3);
    }

    if ((MyPFVInfo = XListPixmapFormats(WorkDisplay, &NumRet)) != NULL) {
        WorkWithThisPFV = MyPFVInfo;
        for (a = 0; a < NumRet; a++) {
            WorkWithThisVisInfo = MyVisInfo;
            for (b = 0; b < NumVis; b++) {
                if ((WorkWithThisVisInfo->depth == WorkWithThisPFV->depth)
                    && (WorkWithThisVisInfo->screen == WorkScreen)) {
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
        fprintf(stderr, "XListPixmapFormats returned NULL.\n");
        return (-1);
    }
    if (selected == -1) {
        fprintf(stderr, "Did not find suitable depth from XListPixmapFormats.\n");
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
        fprintf(stderr, "Can not allocate %d bytes for WorkData.\n",
                ((WorkDepth >> 3) * WIDTH * HEIGHT));
        return (-4);
    }

    if ((ImageBuffer =
         (struct ImageBufferStructure *) malloc(sizeof(struct ImageBufferStructure) * WIDTH *
                                                HEIGHT)) == NULL) {
        fprintf(stderr, "Can not allocate %d bytes for ImageBuffer.\n", (3 * WIDTH * HEIGHT));
        return (-4);
    }

    DEBUG("XCreateImage");

    if ((WorkImage =
         XCreateImage(WorkDisplay, WorkVisual, WorkDepth, ZPixmap, 0, WorkData, WIDTH, HEIGHT,
                      BitmapPad(WorkDisplay), 0)) == NULL)
//      if((WorkImage=XCreateImage(WorkDisplay,WorkVisual,WorkDepth,ZPixmap,0,WorkData,WIDTH,HEIGHT,BitmapPad(WorkDisplay),0))==NULL)
    {
        fprintf(stderr, "XCreateImage returned NULL.\n");
        return (-5);
    }

    DEBUG("->Ok.\nXInitImage");

    if (XInitImage(WorkImage) == 0) {
        fprintf(stderr, "Error with XInitImage\n");
        return (-6);
    }

    WorkBitsPerRGB = WorkImage->bits_per_pixel;

    DEBUG("->Ok. (%d)\n", WorkBitsPerRGB);

    return (0);
}


void
UICopyDisplayBufferToScreen(int x, int y, int w, int h)
{
    register unsigned short *TmpWrk = (unsigned short *) WorkData;
    register int a, b, c, d, wx = x, wy = y, wh = (h + y), ww = w;

    if (!WorkWindowInitialized)
        return;

    wx = 0, wy = 0;
    ww = WIDTH;
    wh = HEIGHT;

    if (wx < 0) {
        if ((ww -= (0 - wx)) <= 0)
            return;             // nothing to draw...
        wx = 0;
    }
    if (wx >= WIDTH)
        return;
    if (wy < 0) {
        if ((wh -= (0 - wy)) <= 0)
            return;
        wy = 0;
    }

    if (wh > HEIGHT)
        wh = HEIGHT;
    if (ww > WIDTH)
        ww = WIDTH;

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

    /* clear unused area */
    XSetForeground(WorkDisplay, WorkWindowGC,
                   BlackPixelOfScreen(DefaultScreenOfDisplay(WorkDisplay)));
    XFillRectangle(WorkDisplay, WorkWindow, WorkWindowGC, WIDTH, 0, width - WIDTH, height);
    XFillRectangle(WorkDisplay, WorkWindow, WorkWindowGC, 0, HEIGHT, width, height - HEIGHT);

    XFlush(WorkDisplay);
}



void
UIMainLoop()
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

                UICopyDisplayBufferToScreen(0, 0, WIDTH, HEIGHT);
            }
            break;
        default:
            break;
        }
    }                           /* while */
}

int
UICreateWindow()
{
    XSizeHints *s_h;
    XTextProperty winname, iconame;
    char TmpBuffer[64];
    char *TmpChrPtrBuffer[2];
    XSetWindowAttributes attribs;

    attribs.override_redirect = FullScreenMode;

    if ((WorkDisplay = XOpenDisplay(NULL)) == NULL) {
        fprintf(stderr, "Error opening display.\n");
        return (-1);
    }
    WorkScreen = DefaultScreen(WorkDisplay);

    if (FullScreenMode) {
        width = WidthOfScreen(DefaultScreenOfDisplay(WorkDisplay));
        height = HeightOfScreen(DefaultScreenOfDisplay(WorkDisplay));
    }

    if (GetHighestVisualPixmapCombination() < 0) {
        fprintf(stderr, "Error for getting Visual Information.\n");
        XCloseDisplay(WorkDisplay);
        return (-1);
    }

    DEBUG("XCreateWindow");

    if ((WorkWindow =
         XCreateWindow(WorkDisplay, RootWindow(WorkDisplay, WorkScreen), 0, 0, width, height, 2,
                       WorkDepth, InputOutput, WorkVisual, CWOverrideRedirect, &attribs)) == 0) {
        fprintf(stderr, "Error Creating Window.\n");
        XCloseDisplay(WorkDisplay);
        return (-1);
    }

    DEBUG("->Ok.\n");

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
                s_h->max_width = width;
                s_h->width = width;
                s_h->min_height = HEIGHT;
                s_h->max_height = height;
                s_h->height = height;
                XSetWMProperties(WorkDisplay, WorkWindow, &winname, &iconame, NULL, 0, s_h, NULL,
                                 NULL);
                XResizeWindow(WorkDisplay, WorkWindow, width, height);
            }
        }
    }
    XMoveWindow(WorkDisplay, WorkWindow, 0, 0);
    XSelectInput(WorkDisplay, WorkWindow, ExposureMask);
    XMapWindow(WorkDisplay, WorkWindow);

    UIMainLoop();

    return (0);
}

void
UIAdjustBacklight(unsigned char dimmed)
{
}

void
UICleanUp(int vis)
{
}
