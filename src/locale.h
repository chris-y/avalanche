#ifndef LOCALE_H
#define LOCALE_H 1

#define CATCOMP_NUMBERS

/*
** locale.h
**
** (c) 2006 by Guido Mersmann
**
** Object source created by SimpleCat
*/

/*************************************************************************/

#include "locale_strings.h" /* change name to correct locale header if needed */

/*
** Prototypes
*/

BOOL   Locale_Open( STRPTR catname, ULONG version, ULONG revision);
void   Locale_Close(void);
STRPTR locale_get_string(long ID);
struct Locale *locale_get_locale(void);

/*************************************************************************/

#endif /* LOCALE_H */
