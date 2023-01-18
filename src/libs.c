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
#include <proto/arexx.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/chooser.h>
#include <proto/fuelgauge.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/requester.h>
#include <proto/window.h>
#else
#include <proto/utility.h>
#endif

#include <stdio.h>

#include "libs.h"
#include "locale.h"

#ifdef __amigaos4__
#define ALIB_OPEN(LIB, LIBVER, PREFIX)	\
	if((PREFIX##Base = (struct Library *)OpenLibrary(LIB, LIBVER))) {	\
		I##PREFIX = (struct PREFIX##IFace *)GetInterface((struct Library *)PREFIX##Base, "main", 1, NULL);	\
		if(I##PREFIX == NULL) {	\
			ALIB_CLOSE(PREFIX)	\
			printf(locale_get_string(MSG_INTERFACE));	\
			printf(locale_get_string(MSG_UNABLETOOPENLIBRARY), LIB, LIBVER); \
			return FALSE;	\
		}	\
	} else {	\
		printf(locale_get_string(MSG_UNABLETOOPENLIBRARY), LIB, LIBVER); \
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
		if(NEEDINTERFACE == TRUE) {	\
			I##PREFIX = (struct PREFIX##IFace *)GetInterface((struct Library *)PREFIX##Base, "main", 1, NULL);	\
			if(I##PREFIX == NULL) {	\
				printf(locale_get_string(MSG_INTERFACE));	\
				printf(locale_get_string(MSG_UNABLETOOPENLIBRARY), CLASS, CLASSVER); \
				return FALSE;	\
			}	\
		}	\
	}	\
	if(PREFIX##Class == NULL) {	\
		printf(locale_get_string(MSG_UNABLETOOPENLIBRARY), CLASS, CLASSVER); \
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
		printf(locale_get_string(MSG_UNABLETOOPENLIBRARY), LIB, LIBVER); \
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
		printf(locale_get_string(MSG_UNABLETOOPENLIBRARY), CLASS, CLASSVER); \
		return FALSE;	\
	}

#define CLASS_CLOSE(PREFIX)	\
	if(PREFIX##Base) CloseLibrary(PREFIX##Base);	\
	PREFIX##Base = NULL;

#define CLASS_STRUCT(PREFIX)	\
	struct Library *PREFIX##Base = NULL;	\
	Class *PREFIX##Class = NULL;

#endif

ALIB_STRUCT(Asl)
#ifdef __amigaos4__
ALIB_STRUCT(Commodities)
ALIB_STRUCT(Graphics)
#else
ALIB_STRUCT(Cx)
ALIB_STRUCT(Gfx)
#endif

ALIB_STRUCT(Icon)
ALIB_STRUCT(Intuition)
//ALIB_STRUCT(Locale)

#ifndef __amigaos4__
ALIB_STRUCT(Utility)
#endif

ALIB_STRUCT(Workbench)

ALIB_STRUCT(xfdMaster)
ALIB_STRUCT(xvs)

#ifdef __amigaos4__
ALIB_STRUCT(XadMaster)
#else
ALIB_STRUCT(xadMaster)
#endif

#ifdef __amigaos4__
ALIB_STRUCT(Zip)
#endif

CLASS_STRUCT(ARexx)
CLASS_STRUCT(Button)
CLASS_STRUCT(CheckBox)
CLASS_STRUCT(Chooser)
CLASS_STRUCT(FuelGauge)
CLASS_STRUCT(GetFile)
CLASS_STRUCT(Label)
CLASS_STRUCT(Layout)
CLASS_STRUCT(ListBrowser)
CLASS_STRUCT(Requester)
CLASS_STRUCT(Window)

BOOL libs_xvs_init(void)
{
	if(xvsBase == NULL) {
		ALIB_OPEN("xvs.library", 33, xvs);
	}

	return TRUE;
}

static void libs_xvs_exit(void)
{
	if(xvsBase != NULL) {
		ALIB_CLOSE(xvs);
	}
}

BOOL libs_zip_init(void)
{
#ifdef __amigaos4__
	if(ZipBase == NULL) {
		ALIB_OPEN("zip.library",  54, Zip)
	}
	
	return TRUE;
#else
	return FALSE;
#endif

}

void libs_zip_exit(void)
{
#ifdef __amigaos4__
	if(ZipBase != NULL) {
		ALIB_CLOSE(Zip);
	}
#endif
}


BOOL libs_open(void)
{
	ALIB_OPEN("asl.library",          36, Asl)
#ifdef __amigaos4__
	ALIB_OPEN("commodities.library",  37, Commodities)
	ALIB_OPEN("graphics.library",     39, Graphics)
#else
	ALIB_OPEN("commodities.library",  37, Cx)
	ALIB_OPEN("graphics.library",     39, Gfx)
#endif
	ALIB_OPEN("icon.library",         44, Icon)
	ALIB_OPEN("intuition.library",    40, Intuition)
//	ALIB_OPEN("locale.library",       38, Locale)
	ALIB_OPEN("utility.library",      36, Utility)
	ALIB_OPEN("workbench.library",    40, Workbench)

	CLASS_OPEN("gadgets/button.gadget",        41, Button,        BUTTON,      FALSE)
	CLASS_OPEN("gadgets/checkbox.gadget",      41, CheckBox,      CHECKBOX,    FALSE)
	CLASS_OPEN("gadgets/chooser.gadget",       45, Chooser,       CHOOSER,     TRUE)
	CLASS_OPEN("gadgets/fuelgauge.gadget",     41, FuelGauge,     FUELGAUGE,   FALSE)
	CLASS_OPEN("gadgets/getfile.gadget",       41, GetFile,       GETFILE,     FALSE)
	CLASS_OPEN("images/label.image",           41, Label,         LABEL,       FALSE)
	CLASS_OPEN("gadgets/layout.gadget",        41, Layout,        LAYOUT,      FALSE)
	CLASS_OPEN("gadgets/listbrowser.gadget",   45, ListBrowser,   LISTBROWSER, TRUE)
	CLASS_OPEN("requester.class",              41, Requester,     REQUESTER,   FALSE)
	CLASS_OPEN("window.class",                 47, Window,        WINDOW,      FALSE)
	CLASS_OPEN("arexx.class",                  41, ARexx,         AREXX,       FALSE)
	
	return TRUE;
}

void libs_close(void)
{
	libs_xvs_exit();
	libs_zip_exit();

	CLASS_CLOSE(Button)
	CLASS_CLOSE(CheckBox)
	CLASS_CLOSE(Chooser)
	CLASS_CLOSE(FuelGauge)
	CLASS_CLOSE(GetFile)
	CLASS_CLOSE(Label)
	CLASS_CLOSE(Layout)
	CLASS_CLOSE(ListBrowser)
	CLASS_CLOSE(Requester)
	CLASS_CLOSE(Window)
	CLASS_CLOSE(ARexx)

	ALIB_CLOSE(Asl)
#ifdef __amigaos4__
	ALIB_CLOSE(Commodities)
	ALIB_CLOSE(Graphics)
#else
	ALIB_CLOSE(Cx)
	ALIB_CLOSE(Gfx)
#endif
	ALIB_CLOSE(Icon)
	ALIB_CLOSE(Intuition)
//	ALIB_CLOSE(Locale)
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

BOOL libs_xfd_init(void)
{
	if(xfdMasterBase == NULL) {
		ALIB_OPEN("xfdmaster.library", 36, xfdMaster);
	}

	return TRUE;
}

void libs_xfd_exit(void)
{
	if(xfdMasterBase != NULL) {
		ALIB_CLOSE(xfdMaster);
	}
}
