/* Avalanche
 * (c) 2022 Chris Young
 */

#include <proto/exec.h>
#include <proto/intuition.h>

#include <proto/button.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/requester.h>
#include <proto/window.h>

#include <stdio.h>

#include "libs.h"

#define ALIB_OPEN(LIB, LIBVER, PREFIX)	\
	if((!(PREFIX##Base = (struct PREFIX##Base *)OpenLibrary(LIB, LIBVER)))) {	\
		printf("Failed to open %s v%d\n", LIB, LIBVER); \
			return FALSE;	\
	}

#define ALIB_CLOSE(PREFIX)	\
	if(PREFIX##Base) CloseLibrary((struct Library *)PREFIX##Base);	\
	PREFIX##Base = NULL;

#define ALIB_STRUCT(PREFIX)	struct PREFIX##Base *PREFIX##Base = NULL;

#define CLASS_OPEN(CLASS, CLASSVER, PREFIX, CLASSGET)	\
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


ALIB_STRUCT(Icon)
ALIB_STRUCT(Intuition)
ALIB_STRUCT(Workbench)

CLASS_STRUCT(Button)
CLASS_STRUCT(GetFile)
CLASS_STRUCT(Label)
CLASS_STRUCT(Layout)
CLASS_STRUCT(ListBrowser)
CLASS_STRUCT(Requester)
CLASS_STRUCT(Window)

BOOL libs_open(void)
{
	ALIB_OPEN("icon.library",         40, Icon)
	ALIB_OPEN("intuition.library",    40, Intuition)
	ALIB_OPEN("workbench.library",    40, Workbench)

	CLASS_OPEN("gadgets/button.gadget",        42, Button,        BUTTON)
	CLASS_OPEN("gadgets/getfile.gadget",       41, GetFile,       GETFILE)
	CLASS_OPEN("images/label.image",           41, Label,         LABEL)
	CLASS_OPEN("gadgets/layout.gadget",        43, Layout,        LAYOUT)
	CLASS_OPEN("gadgets/listbrowser.gadget",   41, ListBrowser,   LISTBROWSER)
	CLASS_OPEN("requester.class",              42, Requester,     REQUESTER)
	CLASS_OPEN("window.class",                 42, Window,        WINDOW)

	return TRUE;
}

void libs_close(void)
{
	CLASS_CLOSE(Button)
	CLASS_CLOSE(GetFile)
	CLASS_CLOSE(Label)
	CLASS_CLOSE(Layout)
	CLASS_CLOSE(ListBrowser)
	CLASS_CLOSE(Requester)
	CLASS_CLOSE(Window)

	ALIB_CLOSE(Icon)
	ALIB_CLOSE(Intuition)
	ALIB_CLOSE(Workbench)
}

