#ifndef LOCALE_STRINGS_H_H
#define LOCALE_STRINGS_H_H


/****************************************************************************/


/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_CODE
#ifndef CATCOMP_BLOCK
#define CATCOMP_ARRAY
#endif
#endif

#ifdef CATCOMP_ARRAY
#ifndef CATCOMP_NUMBERS
#define CATCOMP_NUMBERS
#endif
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif

#ifdef CATCOMP_BLOCK
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif


/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_EXTRACT 0
#define MSG_PROJECT 1
#define MSG_OPEN 2
#define MSG_ARCHIVEINFO 3
#define MSG_ABOUT 4
#define MSG_QUIT 5
#define MSG_EDIT 6
#define MSG_SELECTALL 7
#define MSG_CLEARSELECTION 8
#define MSG_INVERTSELECTION 9
#define MSG_SETTINGS 10
#define MSG_SCANFORVIRUSES 11
#define MSG_HIERARCHICALBROWSEREXPERIMENTAL 12
#define MSG_SAVEWINDOWPOSITION 13
#define MSG_CONFIRMQUIT 14
#define MSG_SAVESETTINGS 15
#define MSG_STOP 16
#define MSG_NAME 17
#define MSG_SIZE 18
#define MSG_DATE 19
#define MSG_SELECTARCHIVE 20
#define MSG_ARCHIVE 21
#define MSG_SELECTDESTINATION 22
#define MSG_DESTINATION 23
#define MSG_ALREADYEXISTSOVERWRITE 24
#define MSG_FILEARCHIVE 25
#define MSG_DISKARCHIVE 26
#define MSG_DISKIMAGE 27
#define MSG_UNKNOWN 28
#define MSG_OK 29
#define MSG_CRUNCHED 30
#define MSG_ERRORDECRUNCHING 31
#define MSG_VIRUSSCANNINGWILLBEDISABLED 32
#define MSG_XVSLIBRARYFAILEDSELFTEST 33
#define MSG_OUTOFMEMORYSCANNINGFILE 34
#define MSG_FILEHASBEENDELETED 35
#define MSG_FILECOULDNOTBEDELETED 36
#define MSG_VIRUSFOUNDETECTIONNAME 37
#define MSG_GPL 38
#define MSG_UNABLETOOPENREQUESTERTOSHOWERRORS 39
#define MSG_UNABLETOOPENLIBRARY 40
#define MSG_AREYOUSUREYOUWANTTOEXIT 41
#define MSG_YESNO 42
#define MSG_YESYESTOALLNONOTOALLABORT 43
#define MSG_ARCHIVEISENCRYPTEDPLEASEENTERPASSWORD 44
#define MSG_OKCANCEL 45
#define MSG_UNABLETOOPENREQUESTERTOASKPASSWORD 46
#define MSG_INTERFACE 47
#define MSG_IGNOREFILESYSTEMS 48
#define MSG_CXDESCRIPTION 49
#define MSG_APPMENU_EXTRACTHERE 50

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_EXTRACT_STR "E_xtract"
#define MSG_PROJECT_STR "Project"
#define MSG_OPEN_STR "Open..."
#define MSG_ARCHIVEINFO_STR "Archive Info..."
#define MSG_ABOUT_STR "About..."
#define MSG_QUIT_STR "Quit..."
#define MSG_EDIT_STR "Edit"
#define MSG_SELECTALL_STR "Select all"
#define MSG_CLEARSELECTION_STR "Clear selection"
#define MSG_INVERTSELECTION_STR "Invert selection"
#define MSG_SETTINGS_STR "Settings"
#define MSG_SCANFORVIRUSES_STR "Scan for viruses"
#define MSG_HIERARCHICALBROWSEREXPERIMENTAL_STR "Hierarchical browser (experimental)"
#define MSG_SAVEWINDOWPOSITION_STR "Save window position"
#define MSG_CONFIRMQUIT_STR "Confirm quit"
#define MSG_SAVESETTINGS_STR "Save settings"
#define MSG_STOP_STR "_Stop"
#define MSG_NAME_STR "Name"
#define MSG_SIZE_STR "Size"
#define MSG_DATE_STR "Date"
#define MSG_SELECTARCHIVE_STR "Select Archive"
#define MSG_ARCHIVE_STR "_Archive"
#define MSG_SELECTDESTINATION_STR "Select Destination"
#define MSG_DESTINATION_STR "_Destination"
#define MSG_ALREADYEXISTSOVERWRITE_STR "%s already exists, overwrite?"
#define MSG_FILEARCHIVE_STR "file archive"
#define MSG_DISKARCHIVE_STR "disk archive"
#define MSG_DISKIMAGE_STR "disk image"
#define MSG_UNKNOWN_STR "unknown"
#define MSG_OK_STR "_OK"
#define MSG_CRUNCHED_STR "%s crunched"
#define MSG_ERRORDECRUNCHING_STR "Error decrunching"
#define MSG_VIRUSSCANNINGWILLBEDISABLED_STR "Virus scanning will be disabled.\n"
#define MSG_XVSLIBRARYFAILEDSELFTEST_STR "xvs.library failed self-test,"
#define MSG_OUTOFMEMORYSCANNINGFILE_STR "Out of memory scanning file"
#define MSG_FILEHASBEENDELETED_STR "File has been deleted."
#define MSG_FILECOULDNOTBEDELETED_STR "File could not be deleted."
#define MSG_VIRUSFOUNDETECTIONNAME_STR "Virus found in %s\nDetection name: %s\n\n"
#define MSG_GPL_STR "This program is free software; you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation; either version 2 of the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details."
#define MSG_UNABLETOOPENREQUESTERTOSHOWERRORS_STR "Unable to open requester to show error;\n%s [%s]\n"
#define MSG_UNABLETOOPENLIBRARY_STR "Unable to open %s v%d\n"
#define MSG_AREYOUSUREYOUWANTTOEXIT_STR "Are you sure you want to exit?"
#define MSG_YESNO_STR "_Yes|_No"
#define MSG_YESYESTOALLNONOTOALLABORT_STR "_Yes|Yes to _all|_No|N_o to all|Abort"
#define MSG_ARCHIVEISENCRYPTEDPLEASEENTERPASSWORD_STR "Archive is encrypted, please enter password."
#define MSG_OKCANCEL_STR "_OK|_Cancel"
#define MSG_UNABLETOOPENREQUESTERTOASKPASSWORD_STR "Unable to open requester to ask password\n"
#define MSG_INTERFACE_STR "(interface)"
#define MSG_IGNOREFILESYSTEMS_STR "Ignore filesystems"
#define MSG_CXDESCRIPTION_STR "Unarchive GUI for XAD/XFD"
#define MSG_APPMENU_EXTRACTHERE_STR "Extract here"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


#ifdef CATCOMP_BLOCK

STATIC CONST UBYTE CatCompBlock[] =
{
    "\x00\x00\x00\x00\x00\x0A"
    MSG_EXTRACT_STR "\x00\x00"
    "\x00\x00\x00\x01\x00\x08"
    MSG_PROJECT_STR "\x00"
    "\x00\x00\x00\x02\x00\x08"
    MSG_OPEN_STR "\x00"
    "\x00\x00\x00\x03\x00\x10"
    MSG_ARCHIVEINFO_STR "\x00"
    "\x00\x00\x00\x04\x00\x0A"
    MSG_ABOUT_STR "\x00\x00"
    "\x00\x00\x00\x05\x00\x08"
    MSG_QUIT_STR "\x00"
    "\x00\x00\x00\x06\x00\x06"
    MSG_EDIT_STR "\x00\x00"
    "\x00\x00\x00\x07\x00\x0C"
    MSG_SELECTALL_STR "\x00\x00"
    "\x00\x00\x00\x08\x00\x10"
    MSG_CLEARSELECTION_STR "\x00"
    "\x00\x00\x00\x09\x00\x12"
    MSG_INVERTSELECTION_STR "\x00\x00"
    "\x00\x00\x00\x0A\x00\x0A"
    MSG_SETTINGS_STR "\x00\x00"
    "\x00\x00\x00\x0B\x00\x12"
    MSG_SCANFORVIRUSES_STR "\x00\x00"
    "\x00\x00\x00\x0C\x00\x24"
    MSG_HIERARCHICALBROWSEREXPERIMENTAL_STR "\x00"
    "\x00\x00\x00\x0D\x00\x16"
    MSG_SAVEWINDOWPOSITION_STR "\x00\x00"
    "\x00\x00\x00\x0E\x00\x0E"
    MSG_CONFIRMQUIT_STR "\x00\x00"
    "\x00\x00\x00\x0F\x00\x0E"
    MSG_SAVESETTINGS_STR "\x00"
    "\x00\x00\x00\x10\x00\x06"
    MSG_STOP_STR "\x00"
    "\x00\x00\x00\x11\x00\x06"
    MSG_NAME_STR "\x00\x00"
    "\x00\x00\x00\x12\x00\x06"
    MSG_SIZE_STR "\x00\x00"
    "\x00\x00\x00\x13\x00\x06"
    MSG_DATE_STR "\x00\x00"
    "\x00\x00\x00\x14\x00\x10"
    MSG_SELECTARCHIVE_STR "\x00\x00"
    "\x00\x00\x00\x15\x00\x0A"
    MSG_ARCHIVE_STR "\x00\x00"
    "\x00\x00\x00\x16\x00\x14"
    MSG_SELECTDESTINATION_STR "\x00\x00"
    "\x00\x00\x00\x17\x00\x0E"
    MSG_DESTINATION_STR "\x00\x00"
    "\x00\x00\x00\x18\x00\x1E"
    MSG_ALREADYEXISTSOVERWRITE_STR "\x00"
    "\x00\x00\x00\x19\x00\x0E"
    MSG_FILEARCHIVE_STR "\x00\x00"
    "\x00\x00\x00\x1A\x00\x0E"
    MSG_DISKARCHIVE_STR "\x00\x00"
    "\x00\x00\x00\x1B\x00\x0C"
    MSG_DISKIMAGE_STR "\x00\x00"
    "\x00\x00\x00\x1C\x00\x08"
    MSG_UNKNOWN_STR "\x00"
    "\x00\x00\x00\x1D\x00\x04"
    MSG_OK_STR "\x00"
    "\x00\x00\x00\x1E\x00\x0C"
    MSG_CRUNCHED_STR "\x00"
    "\x00\x00\x00\x1F\x00\x12"
    MSG_ERRORDECRUNCHING_STR "\x00"
    "\x00\x00\x00\x20\x00\x22"
    MSG_VIRUSSCANNINGWILLBEDISABLED_STR "\x00"
    "\x00\x00\x00\x21\x00\x1E"
    MSG_XVSLIBRARYFAILEDSELFTEST_STR "\x00"
    "\x00\x00\x00\x22\x00\x1C"
    MSG_OUTOFMEMORYSCANNINGFILE_STR "\x00"
    "\x00\x00\x00\x23\x00\x18"
    MSG_FILEHASBEENDELETED_STR "\x00\x00"
    "\x00\x00\x00\x24\x00\x1C"
    MSG_FILECOULDNOTBEDELETED_STR "\x00\x00"
    "\x00\x00\x00\x25\x00\x28"
    MSG_VIRUSFOUNDETECTIONNAME_STR "\x00\x00"
    "\x00\x00\x00\x26\x01\xDC"
    MSG_GPL_STR "\x00\x00"
    "\x00\x00\x00\x27\x00\x32"
    MSG_UNABLETOOPENREQUESTERTOSHOWERRORS_STR "\x00\x00"
    "\x00\x00\x00\x28\x00\x18"
    MSG_UNABLETOOPENLIBRARY_STR "\x00\x00"
    "\x00\x00\x00\x29\x00\x20"
    MSG_AREYOUSUREYOUWANTTOEXIT_STR "\x00\x00"
    "\x00\x00\x00\x2A\x00\x0A"
    MSG_YESNO_STR "\x00\x00"
    "\x00\x00\x00\x2B\x00\x26"
    MSG_YESYESTOALLNONOTOALLABORT_STR "\x00"
    "\x00\x00\x00\x2C\x00\x2E"
    MSG_ARCHIVEISENCRYPTEDPLEASEENTERPASSWORD_STR "\x00\x00"
    "\x00\x00\x00\x2D\x00\x0C"
    MSG_OKCANCEL_STR "\x00"
    "\x00\x00\x00\x2E\x00\x2A"
    MSG_UNABLETOOPENREQUESTERTOASKPASSWORD_STR "\x00"
    "\x00\x00\x00\x2F\x00\x0C"
    MSG_INTERFACE_STR "\x00"
    "\x00\x00\x00\x30\x00\x14"
    MSG_IGNOREFILESYSTEMS_STR "\x00\x00"
    "\x00\x00\x00\x31\x00\x1A"
    MSG_CXDESCRIPTION_STR "\x00"
    "\x00\x00\x00\x32\x00\x0E"
    MSG_APPMENU_EXTRACTHERE_STR "\x00\x00"
};

#endif /* CATCOMP_BLOCK */


/****************************************************************************/



#endif /* LOCALE_STRINGS_H_H */
