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
#include <time.h>
#include <signal.h>

#ifndef BADSIG
#define	BADSIG	((void (*)())-1)
#endif

#define	MAIN_C	1

#include "main.h"

extern char Main_Info;
char CommandLineBuffer[256];

void
IntReXSigCatch(int sig)
{
    if (signal(sig, SIG_IGN) == BADSIG)
	return;
    switch (sig) {
//              case SIGPWR     : 
    case SIGHUP:
    case SIGQUIT:
    case SIGILL:
    case SIGFPE:
    case SIGBUS:
    case SIGSEGV:
    case SIGPIPE:
    case SIGINT:
    case SIGTERM:
	CleanUp();
	exit(-1);
    }
}

static void
SetReXSig(int sig, void (*fcn) ())
{
    void (*ssr) ();

    ssr = signal(sig, SIG_IGN);
    if (ssr == BADSIG)
	return;
    else if (ssr == SIG_IGN)
	return;
    else if (ssr != SIG_DFL)
	return;
    if (signal(sig, fcn) == BADSIG)
	return;
}

void
SetUpMySignals()
{
    SetReXSig(SIGPIPE, IntReXSigCatch);
    SetReXSig(SIGSEGV, IntReXSigCatch);
    SetReXSig(SIGBUS, IntReXSigCatch);
    SetReXSig(SIGFPE, IntReXSigCatch);
    SetReXSig(SIGILL, IntReXSigCatch);
//      SetReXSig(SIGPWR,IntReXSigCatch);
    SetReXSig(SIGHUP, IntReXSigCatch);
    SetReXSig(SIGINT, IntReXSigCatch);
    SetReXSig(SIGQUIT, IntReXSigCatch);
    SetReXSig(SIGTERM, IntReXSigCatch);
}

int
main(int argc, char **argv)
{
    int RetSize = -1;
    int ow, oh, obs;
    char *ob;


//      SetUpMySignals();

    if (argc == 1) {
	printf("\nPlease provide filename to analyze...\n\n");
	fflush(stdout);
	return (0);
    }

    if (argc > 2)
	Main_Info = 1;

    ob = NULL;
    CreateMainWindow();

    for (ow = 0; ((ow < 255) && (argv[1][ow] != '\0')); ow++)
	CommandLineBuffer[ow] = argv[1][ow];

    ob = (char *) CreatePicture(&ow, &oh, &obs);
    if (ob != NULL) {
	if ((RetSize = InitializeNextImageStructure()) == -1) {
	    printf("\n\nCan not allocate next image structure... ?! whaaat?!!!");
	    fflush(stdout);
	    return;
	}
	MyImages[RetSize].OriginalBuffer = ob;
	MyImages[RetSize].OriginalWidth = ow;
	MyImages[RetSize].OriginalHeight = oh;
	MyImages[RetSize].OriginalBufferSize = obs;
	MyImages[RetSize].Name[0] = '%';
	UpdateImageToOriginal(RetSize);
	SelectedImage = RetSize;
	MainLoop();
    }
    else {
	printf("\nError creating Picture");
	fflush(stdout);
    }
    CleanUp();

    printf("\n");
    fflush(stdout);
}
