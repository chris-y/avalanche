/* Avalanche
 * (c) 2022 Chris Young
 */

#ifndef XAD_H
#define XAD_H 1

long xad_info(char *file, void(*addnode)(char *name, LONG *size, void *userdata));
long xad_extract(char *file, char *dest, struct List *list, void *(getnode)(struct Node *node));
char *xad_error(long code);
void xad_exit(void);
#endif
