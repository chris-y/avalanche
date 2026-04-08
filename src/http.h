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

BOOL http_get_url(char *url, void *ssl_ctx, char *buffer, ULONG bufsize, BPTR fh);
BOOL http_check_version(BOOL np);
struct Process *http_get_process_check_version(void);

/* Open and close SSL_CTX */
void *http_open_socket_libs(void);
void http_ssl_free_ctx(void *SSL_ctx);
#endif
