/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
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

typedef void (*ProgressCallback)(int downloaded, int total, gpointer data);

typedef struct _gmpc_easy_download_struct{
	char *data;
	int size;
	int max_size;
	ProgressCallback callback;
	gpointer callback_data;
}gmpc_easy_download_struct;


int gmpc_easy_download(const char *url,gmpc_easy_download_struct *dld);
void gmpc_easy_download_clean(gmpc_easy_download_struct *dld);
void quit_easy_download(void);
