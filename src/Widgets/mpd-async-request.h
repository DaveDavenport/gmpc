#ifndef __MPD_ASYNC_REQUEST_H__
#define __MPD_ASYNC_REQUEST_H__
#include "main.h"
typedef void (MpdAsyncCallback)(MpdData *data, gpointer callback_data);
typedef MpdData * (MpdAsyncFunction)(MpdObj *mi, gpointer function_data);

void mpd_async_request(MpdAsyncCallback *callback, gpointer callback_data, MpdAsyncFunction *function, gpointer function_data);
#endif
