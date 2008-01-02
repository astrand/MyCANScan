/*
    CAN data visualizer for the Toyota Prius
    Zaurus user interface code

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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "images.h"
#include "ui.h"


#define	FL_IOCTL_STEP_CONTRAST	100
int fbfd = -1;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;

#define	XOFFS	0
#define	YOFFS	0
#define	LLENGTH	960
#define	BBPX	16
#define	BBPXSH	2

extern char *WorkData;
extern struct ImageBufferStructure *ImageBuffer;


void
UIAdjustBacklight(unsigned char dimmed)
{
    int bl = 6;
    int fd;

    if (dimmed)
        bl = 1;

    if ((fd = open("/dev/fl", O_WRONLY)) <= 0) {
        fprintf(stderr, "Error opening '/dev/fl'\n");
        return;
    }
    ioctl(fd, FL_IOCTL_STEP_CONTRAST, bl);
    close(fd);
}


int
UICreateWindow()
{
    int c;

    fbfd = open("/dev/fb0", O_RDWR);    // Open the file for reading and writing
    if (!fbfd) {
        fprintf(stderr, "Error: cannot open framebuffer device.\n");
        close(fbfd);
        return (-1);
    }


    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))       // Get fixed screen information
    {
        fprintf(stderr, "Error reading framebuffer fixed information.\n");
        close(fbfd);
        return (-2);
    }


    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))       // Get variable screen information
    {
        fprintf(stderr, "Error reading framebuffer variable information.\n");
        close(fbfd);
        return (-3);
    }

//      if((vinfo.xres!=WIDTH)||(vinfo.yres!=HEIGHT)||(vinfo.bits_per_pixel!=BBPX)||(vinfo.xoffset!=XOFFS)||(vinfo.yoffset!=YOFFS)||(finfo.line_length!=LLENGTH))
    if ((vinfo.xres != HEIGHT) || (vinfo.yres != WIDTH) || (vinfo.bits_per_pixel != BBPX)
        || (vinfo.xoffset != XOFFS) || (vinfo.yoffset != YOFFS) || (finfo.line_length != LLENGTH)) {
        fprintf(stderr, "Error, screen does not match optimized parameters :");
        fprintf(stderr, "%dx%d, %dbpp offs:%d:%d  llng:%d\n", vinfo.xres, vinfo.yres,
                vinfo.bits_per_pixel, vinfo.xoffset, vinfo.yoffset, finfo.line_length);
        close(fbfd);
        return (-4);
    }


    screensize = (WIDTH * HEIGHT * BBPXSH);     // Figure out the size of the screen in bytes

    // Map the device to memory
    fbp = (char *) mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int) fbp == -1) {
        fprintf(stderr, "Error: failed to map framebuffer device to memory.\n");
        close(fbfd);
        return (-5);
    }

    if ((WorkData = (char *) malloc(BBPXSH * WIDTH * HEIGHT)) == NULL) {
        fprintf(stderr, "Error: failed to allocate Workdata buffer.\n");
        munmap(fbp, screensize);
        close(fbfd);
        return (-6);
    }

    if ((ImageBuffer =
         (struct ImageBufferStructure *) malloc(sizeof(struct ImageBufferStructure) * WIDTH *
                                                HEIGHT)) == NULL) {
        fprintf(stderr, "Error: failed to allocate ImageBuffer..\n");
        free((void *) WorkData);
        munmap(fbp, screensize);
        close(fbfd);
        return (-7);
    }

// NOW SET UP TOUCHSCREEN               45:45  560:45  45:445   560:445

#ifdef USE_TOUCHSCREEN
    if ((touch_screen_fd = open("/dev/sharp_ts", O_RDONLY | O_NONBLOCK)) == -1) // |O_NONBLOCK
    {
        fprintf(stderr, "Can not open Touchscreen Device...\n\n");
        return (-1);
    }

//printf("Touchscreen = %d",touch_screen_fd);fflush(stdout);
    c = 0;
    if (fcntl(touch_screen_fd, F_SETOWN, getpid()) >= 0) {
#if 0
        if (fcntl(touch_screen_fd, F_SETFL, O_NONBLOCK) < 0) {
            fprintf(stderr, "Error with O_NONBLOCK SIGIO for TouchScreen.\n");
        }
#endif
        if (fcntl(touch_screen_fd, F_SETFL, FASYNC) < 0) {
            fprintf(stderr, "Error with FASYNC SIGIO for TouchScreen.\n");
        }
    }

#endif

    return (0);
}


void
UICopyDisplayBufferToScreen(int x, int y, int w, int h)
{
    register unsigned short int *TmpWrk = (unsigned short *) WorkData;
    register int a, b, c, d, wx = x, wy = y, wh = (h + y), ww = (x + w);

    for (b = wy; b < wh; b++) {
        c = wx + b * WIDTH;
        for (a = wx; a < ww; a++) {
            TmpWrk[c] =
                (((unsigned short) (ImageBuffer[c].
                                    R & 0xF8) << 8) | ((unsigned short) (ImageBuffer[c].
                                                                         G & 0xFC) << 3) |
                 ((unsigned short) (ImageBuffer[c].B & 0xF8) >> 3));
            ++c;
        }
    }

    for (b = wy; b < wh; b++)   // Actual Copy to screen
    {
//              c=b*LLENGTH+wx*BBPXSH;          // since YOFFS/XOFFS is 0

        c = ((HEIGHT - 1) - b) * BBPXSH + (wx * LLENGTH);
        d = wx + b * WIDTH;
        for (a = wx; a < ww; a++) {
            *((unsigned short int *) (fbp + c)) = TmpWrk[d];
            c += LLENGTH;
            ++d;
        }
    }
}


void
UICleanUp(int vis)
{
    if (fbfd != -1) {
        munmap(fbp, screensize);
        close(fbfd);
        fbfd = -1;
    }
}
