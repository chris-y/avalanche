/* Avalanche
 * (c) 2022 Chris Young
 */

#ifndef XAD_H
#define XAD_H 1

long xad_extract(char *file, char *dest);
char *xad_error(long code);
void xad_exit(void);
#endif
