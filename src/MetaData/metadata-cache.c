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

#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "main.h"
#include "metadata.h"
#include "config1.h"
#include "metadata-cache-sqlite.h"
#include "metadata-cache.h"
#include "pixbuf-cache.h"

#define CACHE_NAME "Metadata cache"
#define MDC_LOG_DOMAIN "MetaDataCache"


MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, MetaData **met)
{
	return meta_data_sqlite_get_from_cache(song, type, met);
}

void meta_data_set_cache_real(mpd_Song *song, MetaDataResult result, MetaData *met)
{
	if(met->type == META_ARTIST_ART || met->type == META_ALBUM_ART) {
		MetaData *m= NULL;
		MetaDataResult r = meta_data_sqlite_get_from_cache(song, met->type, &m);
		if(r == META_DATA_AVAILABLE &&  meta_data_is_uri(m)) {
			const gchar *uri = meta_data_get_uri(m);
			pixbuf_cache_invalidate_pixbuf_entry(uri);
		}	
		meta_data_free(m);
	}
	meta_data_sqlite_set_cache_real(song, result, met);
}

void meta_data_set_cache(mpd_Song *song, MetaDataResult result, MetaData *met)
{
	meta_data_sqlite_set_cache(song, result, met);
}

void metadata_cache_init(void)
{
	meta_data_sqlite_cache_init();
}


void metadata_cache_cleanup(void)
{
	meta_data_sqlite_cache_cleanup();
}
void metadata_cache_destroy(void)
{
	meta_data_sqlite_cache_destroy();
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
