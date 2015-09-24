
#ifndef _DIALOGS_H
#define _DIALOGS_H

#define DLG_CANCEL		1
#define DLG_OK			2

typedef int (*DialogCallback)(int type, char *buffer, void *p);

int dlgGetString(char *title, char *buffer, unsigned int maxlen, DialogCallback cb, void *p);

#endif
