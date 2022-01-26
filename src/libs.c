/* Avalanche
 * (c) 2022 Chris Young
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <proto/exec.h>

#include <proto/intuition.h>

#ifndef __amigaos4__
#include <proto/button.h>
#include <proto/fuelgauge.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/requester.h>
#include <proto/window.h>
#endif

#include <stdbool.h>
#include <stdio.h>

#include "libs.h"

#ifdef __amigaos4__
#define ALIB_OPEN(LIB, LIBVER, PREFIX)	\
	if((PREFIX##Base = (struct Library *)OpenLibrary(LIB, LIBVER))) {	\
		I##PREFIX = (struct PREFIX##IFace *)GetInterface((struct Library *)PREFIX##Base, "main", 1, NULL);	\
		if(I##PREFIX == NULL) {	\
			ALIB_CLOSE(PREFIX)	\
			printf("Failed to open %s v%d (interface)\n", LIB, LIBVER); \
			return FALSE;	\
		}	\
	} else {	\
		printf("Failed to open %s v%d\n", LIB, LIBVER); \
		return FALSE;	\
	}

#define ALIB_CLOSE(PREFIX)	\
	if(I##PREFIX) DropInterface((struct Interface *)I##PREFIX);	\
	if(PREFIX##Base) CloseLibrary((struct Library *)PREFIX##Base);	\
	I##PREFIX = NULL;	\
	PREFIX##Base = NULL;

#define ALIB_STRUCT(PREFIX)	\
	struct Library *PREFIX##Base = NULL;	\
	struct PREFIX##IFace *I##PREFIX = NULL;

#define CLASS_OPEN(CLASS, CLASSVER, PREFIX, CLASSGET, NEEDINTERFACE)	\
	if((PREFIX##Base = OpenClass(CLASS, CLASSVER, &PREFIX##Class))) {	\
		if(NEEDINTERFACE == true) {	\
			I##PREFIX = (struct PREFIX##IFace *)GetInterface((struct Library *)PREFIX##Base, "main", 1, NULL);	\
			if(I##PREFIX == NULL) {	\
				printf("Failed to open %s v%d (interface)\n", CLASS, CLASSVER); \
				return FALSE;	\
			}	\
		}	\
	}	\
	if(PREFIX##Class == NULL) {	\
		printf("Failed to open %s v%d\n", CLASS, CLASSVER); \
		return FALSE;	\
	}

#define CLASS_CLOSE(PREFIX)	\
	if(I##PREFIX) DropInterface((struct Interface *)I##PREFIX);	\
	if(PREFIX##Base) CloseClass(PREFIX##Base);	\
	I##PREFIX = NULL;	\
	PREFIX##Base = NULL;

#define CLASS_STRUCT(PREFIX)	\
	struct ClassLibrary *PREFIX##Base = NULL;	\
	struct PREFIX##IFace *I##PREFIX = NULL;	\
	Class *PREFIX##Class = NULL;

#else

#define ALIB_OPEN(LIB, LIBVER, PREFIX)	\
	if((!(PREFIX##Base = (struct PREFIX##Base *)OpenLibrary(LIB, LIBVER)))) {	\
		printf("Failed to open %s v%d\n", LIB, LIBVER); \
			return FALSE;	\
	}

#define ALIB_CLOSE(PREFIX)	\
	if(PREFIX##Base) CloseLibrary((struct Library *)PREFIX##Base);	\
	PREFIX##Base = NULL;

#define ALIB_STRUCT(PREFIX)	struct PREFIX##Base *PREFIX##Base = NULL;

#define CLASS_OPEN(CLASS, CLASSVER, PREFIX, CLASSGET, N)	\
	if((PREFIX##Base = OpenLibrary(CLASS, CLASSVER))) {	\
		PREFIX##Class = CLASSGET##_GetClass();	\
	}	\
	if(PREFIX##Class == NULL) {	\
		printf("Failed to open %s v%d\n", CLASS, CLASSVER); \
		return FALSE;	\
	}

#define CLASS_CLOSE(PREFIX)	\
	if(PREFIX##Base) CloseLibrary(PREFIX##Base);	\
	PREFIX##Base = NULL;

#define CLASS_STRUCT(PREFIX)	\
	struct Library *PREFIX##Base = NULL;	\
	Class *PREFIX##Class = NULL;

#endif

ALIB_STRUCT(Icon)
ALIB_STRUCT(Intuition)
ALIB_STRUCT(Locale)
ALIB_STRUCT(Workbench)

#ifdef __amigaos4__
ALIB_STRUCT(XadMaster)
#else
ALIB_STRUCT(xadMaster)
#endif

CLASS_STRUCT(Button)
CLASS_STRUCT(FuelGauge)
CLASS_STRUCT(GetFile)
CLASS_STRUCT(Label)
CLASS_STRUCT(Layout)
CLASS_STRUCT(ListBrowser)
CLASS_STRUCT(Requester)
CLASS_STRUCT(Window)

BOOL libs_open(void)
{
	ALIB_OPEN("icon.library",         44, Icon)
	ALIB_OPEN("intuition.library",    40, Intuition)
	ALIB_OPEN("locale.library",       38, Locale)
	ALIB_OPEN("workbench.library",    40, Workbench)

	CLASS_OPEN("gadgets/button.gadget",        41, Button,        BUTTON,      FALSE)
	CLASS_OPEN("gadgets/fuelgauge.gadget",     41, FuelGauge,     FUELGAUGE,   FALSE)
	CLASS_OPEN("gadgets/getfile.gadget",       41, GetFile,       GETFILE,     FALSE)
	CLASS_OPEN("images/label.image",           41, Label,         LABEL,       FALSE)
	CLASS_OPEN("gadgets/layout.gadget",        41, Layout,        LAYOUT,      TRUE)
	CLASS_OPEN("gadgets/listbrowser.gadget",   45, ListBrowser,   LISTBROWSER, TRUE)
	CLASS_OPEN("requester.class",              41, Requester,     REQUESTER,   FALSE)
	CLASS_OPEN("window.class",                 47, Window,        WINDOW,      FALSE)

	return TRUE;
}

void libs_close(void)
{
	CLASS_CLOSE(Button)
	CLASS_CLOSE(FuelGauge)
	CLASS_CLOSE(GetFile)
	CLASS_CLOSE(Label)
	CLASS_CLOSE(Layout)
	CLASS_CLOSE(ListBrowser)
	CLASS_CLOSE(Requester)
	CLASS_CLOSE(Window)

	ALIB_CLOSE(Icon)
	ALIB_CLOSE(Intuition)
	ALIB_CLOSE(Locale)
	ALIB_CLOSE(Workbench)
}

BOOL libs_xad_init(void)
{
#ifdef __amigaos4__
	if(XadMasterBase == NULL) {
		ALIB_OPEN("xadmaster.library", 13, XadMaster);
	}
#else
	if(xadMasterBase == NULL) {
		ALIB_OPEN("xadmaster.library", 12, xadMaster);
	}
#endif

return TRUE;
}

void libs_xad_exit(void)
{
#ifdef __amigaos4__
	if(XadMasterBase != NULL) {
		ALIB_CLOSE(XadMaster);
	}
#else
	if(xadMasterBase != NULL) {
		ALIB_CLOSE(xadMaster);
	}
#endif
}
