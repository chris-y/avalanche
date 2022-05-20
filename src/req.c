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

#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <clib/alib_protos.h>

#include <proto/requester.h>
#include <classes/requester.h>

#include <reaction/reaction_macros.h>

#include "avalanche.h"
#include "libs.h"
#include "locale.h"
#include "req.h"
#include "xad.h"

#include "Avalanche_rev.h"

#define MSG_COPYRIGHT VERS " (" DATE ")\n" "(c) 2022 Chris Young\n\33uhttps://github.com/chris-y/avalanche\33n\n\n"

void open_info_req(char *message, char *buttons)
{
	Object *obj = RequesterObj,
					REQ_TitleText, VERS,
					REQ_Type, REQTYPE_INFO,
					REQ_Image, REQIMAGE_INFO,
					REQ_BodyText, 	message,
					REQ_GadgetText, buttons,
				End;

	if(obj) {
		OpenRequester(obj, get_window());
		DisposeObject(obj);
	}
}

void show_about(void)
{
	char *msg = AllocVec(strlen(MSG_COPYRIGHT) + strlen(locale_get_string( MSG_GPL )) + 1, MEMF_CLEAR);

	if(msg) {
		sprintf(msg, "%s%s", MSG_COPYRIGHT, locale_get_string(MSG_GPL));
		open_info_req(msg,  locale_get_string( MSG_OK ) );
		FreeVec(msg);
	}
}

void open_error_req(char *message, char *button)
{
	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_ERROR, 
			REQ_BodyText, message,
			REQ_GadgetText, button,
		End;

	if(obj) {
		OpenRequester(obj, get_window());
		DisposeObject(obj);
	} else {
		printf( locale_get_string( MSG_UNABLETOOPENREQUESTERTOSHOWERRORS ) , message, button);
	}
}

void show_error(long code)
{
	char message[100];

	if(code == -1) {
		sprintf(message,  locale_get_string( MSG_UNABLETOOPENLIBRARY ), "xadmaster.library", 12 );
	} else {
		sprintf(message, "%s", xad_error(code));
	}

	open_error_req(message,  locale_get_string( MSG_OK ) );
}

ULONG ask_quit_req(void)
{
	int ret = 1;

	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_WARNING,
			REQ_BodyText,  locale_get_string( MSG_AREYOUSUREYOUWANTTOEXIT ) ,
			REQ_GadgetText,  locale_get_string( MSG_YESNO ) ,
		End;

	if(obj) {
		ret = OpenRequester(obj, get_window());
		DisposeObject(obj);
	}
	return ret;
}

ULONG ask_question(char *q, char *f)
{
	char message[200];
	int ret = 0;

	sprintf(message, q, f);

	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_QUESTION,
			REQ_BodyText, message,
			REQ_GadgetText,  locale_get_string( MSG_YESYESTOALLNONOTOALLABORT ) ,
		End;

	if(obj) {
		ret = OpenRequester(obj, get_window()); 
		DisposeObject(obj);
	} else {
		printf( locale_get_string( MSG_UNABLETOOPENREQUESTERTOSHOWERRORS ) , message, "");
	}

	return ret;
}

ULONG ask_password(char *pw, ULONG pwlen)
{
	int ret = 0;

	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_STRING,
			REQ_Image, REQIMAGE_QUESTION,
			REQS_Invisible, TRUE,
			REQS_Buffer, pw,
			REQS_MaxChars, pwlen,
			REQ_BodyText,  locale_get_string( MSG_ARCHIVEISENCRYPTEDPLEASEENTERPASSWORD ) ,
			REQ_GadgetText,  locale_get_string( MSG_OKCANCEL ) ,
		End;

	if(obj) {
		ret = OpenRequester(obj, get_window()); 
		DisposeObject(obj);
	} else {
		printf( locale_get_string( MSG_UNABLETOOPENREQUESTERTOASKPASSWORD ) );
	}

	return ret;
}
