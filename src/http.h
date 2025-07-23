/* Avalanche
 * (c) 2023-5 Chris Young
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

#ifndef AVALANCHE_HTTP_H
#define AVALANCHE_HTTP_H 1

#include <exec/types.h>
#include <openssl/types.h>

BOOL http_get_url(char *url, SSL_CTX *sslctx, char *buffer, ULONG bufsize, BPTR fh);
BOOL http_check_version(void *awin, struct MsgPort *winport, struct MsgPort *appport, struct MsgPort *appwin_mp, BOOL np);
struct Process *http_get_process_check_version(void);
#endif
