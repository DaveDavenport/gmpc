/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __GMPC_EASY_DOWNLOAD_H__
#define __GMPC_EASY_DOWNLOAD_H__
/**
 * Async easy download api 
 * This is based on libsoup
 */

typedef struct _GEADAsyncHandler GEADAsyncHandler;
typedef enum {
	GEAD_DONE,
	GEAD_PROGRESS,
	GEAD_FAILED,
	GEAD_CANCELLED
} GEADStatus;

typedef void (*GEADAsyncCallback) (const GEADAsyncHandler * handle, GEADStatus status, gpointer user_data);
typedef void (*GEADAsyncCallbackVala) (const GEADAsyncHandler * handle, GEADStatus status, gpointer user_data,
gpointer userdata_callback);
/**
 * @param uri       the http uri to download
 * @param callback  the callback function. Giving status updates on the download.
 * @param user_data Data to pass along to callback.
 *
 * returns: a GEADAsyncHandler (or NULL on failure), remember you need to free this. This can be done f.e. in the callback. (same Handler get passed)
 */
GEADAsyncHandler *gmpc_easy_async_downloader(const gchar * uri, GEADAsyncCallback callback, gpointer user_data);

GEADAsyncHandler *gmpc_easy_async_downloader_with_headers(const gchar * uri,
														  GEADAsyncCallback callback, gpointer user_data, ...);
/**
 * Cancel download, triggers GEAD_CANCEL in callback 
 */
void gmpc_easy_async_cancel(const GEADAsyncHandler * handle);

/**
 * Get the size of the download, usefull for progressbar
 */
goffset gmpc_easy_handler_get_content_size(const GEADAsyncHandler * handle);
const char *gmpc_easy_handler_get_uri(const GEADAsyncHandler * handle);

const char *gmpc_easy_handler_get_data(const GEADAsyncHandler * handle, goffset * length);

const guchar *gmpc_easy_handler_get_data_vala_wrap(const GEADAsyncHandler * handle, gint * length);
/**
 * Get the data as a string. 
 * If you know it will be a text. This function will return the data \0 
 * terminated.  (if you need length, without the trailing \0 use get_data
 * This can be usefull for f.e. vala that might copy the 'raw'data  and remove 
 * the trailing '\0'.
 */
const char *gmpc_easy_handler_get_data_as_string(const GEADAsyncHandler * handle);

void gmpc_easy_handler_set_user_data(const GEADAsyncHandler *handle, gpointer user_data);
gpointer gmpc_easy_handler_get_user_data(const GEADAsyncHandler *handle);


char *gmpc_easy_download_uri_escape(const char *part);
GEADAsyncHandler * gmpc_easy_async_downloader_vala(const char *path, gpointer user_data2, GEADAsyncCallbackVala callback,
														  gpointer user_data
														  );
#endif
/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
