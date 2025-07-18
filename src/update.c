/* Avalanche
 * (c) 2025 Chris Young
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

#include "update.h"

void update_gui(struct avalanche_version_numbers avn[], void *ssl_ctx)
{
                for(int i = 0; i < ACHECKVER_MAX; i++) {
#ifdef __amigaos4__
                        DebugPrintF("%d: Current: %d.%d, New: %d.%d\n", i, avn[i].current_version, avn[i].current_revision, avn[i].latest_version, avn[i].latest_revision);
#endif
                }
}
