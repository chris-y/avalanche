/* Avalanche
 * (c) 2022 Chris Young
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <clib/alib_protos.h>

#include <proto/button.h>
#include <proto/getfile.h>
#include <proto/layout.h>
#include <proto/requester.h>
#include <proto/window.h>

#include <classes/requester.h>
#include <classes/window.h>
#include <gadgets/getfile.h>
#include <workbench/startup.h>

#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include "libs.h"
#include "xad.h"

#include "avalanche_rev.h"

const char *version = VERSTAG;

enum
{
	GID_MAIN=0,
	GID_ARCHIVE,
	GID_DEST,
	GID_EXTRACT,
	GID_LAST
};

enum
{
	WID_MAIN=0,
	WID_LAST
};

enum
{
	OID_MAIN=0,
	OID_REQ,
	OID_LAST
};

/** Global config **/
static char *dest;
static char *archive = NULL;

static BOOL archive_needs_free = FALSE;
static BOOL dest_needs_free = FALSE;

/** Useful functions **/

char *strdup(const char *s)
{
  size_t len = strlen (s) + 1;
  char *result = (char*) malloc (len);
  if (result == (char*) 0)
  return (char*) 0;
  return (char*) memcpy (result, s, len);
}

ULONG OpenRequesterTags(Object *obj, struct Window *win, ULONG Tag1, ...)
{
	struct orRequest msg[1];
	msg->MethodID = RM_OPENREQ;
	msg->or_Window = win;	/* window OR screen is REQUIRED */
	msg->or_Screen = NULL;
	msg->or_Attrs = (struct TagItem *)&Tag1;

	return(DoMethodA(obj, (Msg)msg));
}


/** Private functions **/
static void show_error(Object *obj, struct Window *win, long code)
{
	char message[100];

	if(code == -1) {
		sprintf(message, "Unable to open xadmaster.library");
	} else {
		sprintf(message, "Error %d", code);
	}

	if(obj) {
		OpenRequesterTags(obj, win, 
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_ERROR, 
			REQ_BodyText, message,
			REQ_GadgetText, "OK", TAG_DONE);
	} else {
		printf("Unable to open requester to show error;\n%s\n", message);
	}
}

static void free_archive_path(void)
{
	if(archive) FreeVec(archive);
	archive = NULL;
	archive_needs_free = FALSE;
}

static void free_dest_path(void)
{
	if(dest) free(dest);
	dest = NULL;
	dest_needs_free = FALSE;
}

static void gui(void)
{
	struct MsgPort *AppPort;
	struct Window *windows[WID_LAST];
	struct Gadget *gadgets[GID_LAST];
	Object *objects[OID_LAST];

	if ( AppPort = CreateMsgPort() ) {
		/* Create the window object.
		 */
		objects[OID_MAIN] = WindowObj,
			WA_ScreenTitle, VERS,
			WA_Title, VERS,
			WA_Activate, TRUE,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_SizeGadget, TRUE,
			WINDOW_IconifyGadget, TRUE,
			WINDOW_IconTitle, "Avalanche",
			WINDOW_AppPort, AppPort,
			WINDOW_Position, WPOS_CENTERSCREEN,
			WINDOW_ParentGroup, gadgets[GID_MAIN] = LayoutVObj,
				LAYOUT_DeferLayout, TRUE,
				LAYOUT_SpaceOuter, TRUE,

				LAYOUT_AddChild, LayoutVObj,
					LAYOUT_EvenSize, TRUE,
					LAYOUT_AddChild, gadgets[GID_ARCHIVE] = GetFileObj,
						GA_ID, GID_ARCHIVE,
						GA_RelVerify, TRUE,
						GETFILE_TitleText, "Select Archive",
						GETFILE_FullFile, archive,
						GETFILE_ReadOnly, TRUE,
					End,
					LAYOUT_AddChild, gadgets[GID_DEST] = GetFileObj,
						GA_ID, GID_DEST,
						GA_RelVerify, TRUE,
						GETFILE_TitleText, "Select Destination",
						GETFILE_Drawer, dest,
						GETFILE_DoSaveMode, TRUE,
						GETFILE_DrawersOnly, TRUE,
						GETFILE_ReadOnly, TRUE,
					End,
					LAYOUT_AddChild, gadgets[GID_EXTRACT] = ButtonObj,
						GA_ID, GID_EXTRACT,
						GA_RelVerify, TRUE,
						GA_Text, "E_xtract",
					ButtonEnd,
				LayoutEnd,
			EndGroup,
		EndWindow;

		objects[OID_REQ] = RequesterObj,
			REQ_TitleText, VERS,
		End;

	 	/*  Object creation sucessful?
	 	 */
		if (objects[OID_MAIN])
		{
			/*  Open the window.
			 */
			if (windows[WID_MAIN] = (struct Window *) RA_OpenWindow(objects[OID_MAIN]))
			{
				ULONG wait, signal, app = (1L << AppPort->mp_SigBit);
				ULONG done = FALSE;
				ULONG result;
				UWORD code;
				long ret = 0;

			 	/* Obtain the window wait signal mask.
				 */
				GetAttr(WINDOW_SigMask, objects[OID_MAIN], &signal);

				/* Input Event Loop
				 */
				while (!done)
				{
					wait = Wait( signal | SIGBREAKF_CTRL_C | app );

					if ( wait & SIGBREAKF_CTRL_C )
					{
						done = TRUE;
					}
					else
					{
						while ( (result = RA_HandleInput(objects[OID_MAIN], &code) ) != WMHI_LASTMSG )
						{
							switch (result & WMHI_CLASSMASK)
							{
								case WMHI_CLOSEWINDOW:
									windows[WID_MAIN] = NULL;
									done = TRUE;
									break;

								case WMHI_GADGETUP:
									switch (result & WMHI_GADGETMASK)
									{
										case GID_ARCHIVE:
											if(archive_needs_free) free_archive_path();
											DoMethod(gadgets[GID_ARCHIVE], GFILE_REQUEST, windows[WID_MAIN]);
											GetAttr(GETFILE_FullFile, gadgets[GID_ARCHIVE], &archive);
										break;
										
										case GID_DEST:
											if(dest_needs_free) free_dest_path();
											DoMethod(gadgets[GID_DEST], GFILE_REQUEST, windows[WID_MAIN]);
											GetAttr(GETFILE_Drawer, gadgets[GID_DEST], &dest);
										break;

										case GID_EXTRACT:
											if(archive && dest) {
												SetWindowPointer(windows[WID_MAIN],
													WA_BusyPointer, TRUE,
													TAG_DONE);
												ret = xad_extract(archive, dest);
												SetWindowPointer(windows[WID_MAIN],
													WA_BusyPointer, FALSE,
													TAG_DONE);

												if(ret != 0) show_error(objects[OID_REQ],
																 windows[WID_MAIN], ret);
											}
										break;
									}
									break;

								case WMHI_ICONIFY:
									RA_Iconify(objects[OID_MAIN]);
									windows[WID_MAIN] = NULL;
									break;

								case WMHI_UNICONIFY:
									windows[WID_MAIN] = (struct Window *) RA_OpenWindow(objects[OID_MAIN]);

									if (windows[WID_MAIN])
									{
										GetAttr(WINDOW_SigMask, objects[OID_MAIN], &signal);
									}
									else
									{
										done = TRUE;	// error re-opening window!
									}
								 	break;
							}
						}
					}
				}
			}

			DisposeObject(objects[OID_MAIN]);
		}

		DeleteMsgPort(AppPort);
	}
}

static void gettooltypes(UBYTE **tooltypes)
{
	char *s;
	
	dest = strdup(ArgString(tooltypes, "DEST", "RAM:"));
	dest_needs_free = TRUE;
}

/** Main program **/
int main(int argc, char **argv)
{

	if(libs_open() == FALSE) {
		return 10;
	}

	if(argc == 0) {
		/* Started from WB */
		struct WBStartup *WBenchMsg = (struct WBStartup *)argv;
		struct WBArg *wbarg = NULL;
 
		if(WBenchMsg->sm_NumArgs > 0) {
			/* Started as default tool, get the path+filename of the (first) project */
			wbarg = WBenchMsg->sm_ArgList + 1;

            if((wbarg->wa_Lock)&&(*wbarg->wa_Name)) {
				if(archive = AllocVec(512, MEMF_CLEAR)) {
					archive_needs_free = TRUE;
					NameFromLock(wbarg->wa_Lock, archive, 512);
					AddPart(archive, wbarg->wa_Name, 512);
				}
			} 
        }
	}
	
	UBYTE **tooltypes = ArgArrayInit(argc, argv);
	gettooltypes(tooltypes);
	ArgArrayDone();

	gui();

	if(archive_needs_free) free_archive_path();
	if(dest_needs_free) free_dest_path();
	
	libs_close();

	return 0;
}
