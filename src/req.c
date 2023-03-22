/* Avalanche
 * (c) 2022-3 Chris Young
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
#include "module.h"
#include "req.h"
#include "win.h"

#include "Avalanche_rev.h"

#define MSG_COPYRIGHT VERS " (" DATE ")\n" "(c) 2022-3 Chris Young\n\33uhttps://github.com/chris-y/avalanche\33n\n\n"

void open_info_req(const char *message, const char *buttons, void *awin)
{
	Object *obj = RequesterObj,
					REQ_TitleText, VERS,
					REQ_Type, REQTYPE_INFO,
					REQ_Image, REQIMAGE_INFO,
					REQ_BodyText, 	message,
					REQ_GadgetText, buttons,
				End;

	if(obj) {
		OpenRequester(obj, window_get_window(awin));
		DisposeObject(obj);
	}
}

void show_about(void *awin)
{
	int len = strlen(MSG_COPYRIGHT) + strlen(locale_get_string( MSG_GPL )) + 1;
	char *msg = AllocVec(len, MEMF_PRIVATE | MEMF_CLEAR);

	if(msg) {
		snprintf(msg, len, "%s%s", MSG_COPYRIGHT, locale_get_string(MSG_GPL));
		open_info_req(msg, locale_get_string(MSG_OK), awin);
		FreeVec(msg);
	}
}

int open_error_req(const char *message, const char *button, void *awin)
{
	int ret = 0;

	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_ERROR, 
			REQ_BodyText, message,
			REQ_GadgetText, button,
		End;

	if(obj) {
		ret = OpenRequester(obj, window_get_window(awin));
		DisposeObject(obj);
	} else {
		printf( locale_get_string( MSG_UNABLETOOPENREQUESTERTOSHOWERRORS ) , message, button);
	}
	
	return ret;
}

void show_error(long code, void *awin)
{
	char message[100];

	if(code == -1) {
		/* TODO: check this, code is the same for xfd */
		snprintf(message, 100, locale_get_string( MSG_UNABLETOOPENLIBRARY ), "xadmaster.library", 12 );
	} else {
		snprintf(message, 100, "%s", module_get_error(awin, code));
	}

	open_error_req(message, locale_get_string(MSG_OK), awin);
}

ULONG ask_yesno_req(void *awin, const char *message)
{
	int ret = 1;

	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_WARNING,
			REQ_BodyText,  message ,
			REQ_GadgetText,  locale_get_string( MSG_YESNO ) ,
		End;

	if(obj) {
		ret = OpenRequester(obj, window_get_window(awin));
		DisposeObject(obj);
	}
	return ret;
}

ULONG ask_quithide_req(void)
{
	int ret = 1;

	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_WARNING,
			REQ_BodyText,  locale_get_string( MSG_LASTWINDOWCLOSED ) ,
			REQ_GadgetText,  locale_get_string( MSG_QUITHIDECANCEL ) ,
		End;

	if(obj) {
		ret = OpenRequester(obj, NULL);
		DisposeObject(obj);
	}
	return ret;
}

ULONG ask_question(void *awin, const char *q, const char *f)
{
	char message[200];
	int ret = 0;

	snprintf(message, 200, q, f);

	Object *obj = RequesterObj,
			REQ_TitleText, VERS,
			REQ_Type, REQTYPE_INFO,
			REQ_Image, REQIMAGE_QUESTION,
			REQ_BodyText, message,
			REQ_GadgetText,  locale_get_string( MSG_YESYESTOALLNONOTOALLABORT ) ,
		End;

	if(obj) {
		ret = OpenRequester(obj, window_get_window(awin)); 
		DisposeObject(obj);
	} else {
		printf( locale_get_string( MSG_UNABLETOOPENREQUESTERTOSHOWERRORS ) , message, "");
	}

	return ret;
}

ULONG ask_password(void *awin, const char *pw, ULONG pwlen)
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
		ret = OpenRequester(obj, window_get_window(awin)); 
		DisposeObject(obj);
	} else {
		printf( locale_get_string( MSG_UNABLETOOPENREQUESTERTOASKPASSWORD ) );
	}

	return ret;
}

void req_show_arc_info(void *awin)
{
	char message[100];
	const char *modname = module_get_read_module(awin);
	
	if(modname == NULL) return;

	const char *subf = module_get_subformat(awin);
	
	if(subf) {
		snprintf(message, 100, "%s (%s) - %s", module_get_format(awin), subf, modname);	
	} else {
		snprintf(message, 100, "%s - %s", module_get_format(awin), modname);	
	}
	
	open_info_req(message, locale_get_string(MSG_OK), awin);
}
