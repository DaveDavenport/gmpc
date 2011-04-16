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

#ifndef __METADATA_CACHE_H__
#define __METADATA_CACHE_H__
/**
 * Get metadata from cache. 
 * *met needs to be free-ed.
 */
MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, MetaData **met);

/**
 * Set a cache entry directly. Don't use. 
 */
void meta_data_set_cache_real(mpd_Song *song, MetaDataResult result, MetaData *met);
/**
 * Set the rewrited value.
 */
void meta_data_set_cache(mpd_Song *song, MetaDataResult result, MetaData *met);

/**
 * Removes all entries that are set to META_DATA_CONTENT_EMPTY (or META_DATA_UNAVAILABLE)
 */
void metadata_cache_cleanup(void);



/**
 * Start the cache. 
 * (initializes sqlite db, etc)
 */
void metadata_cache_init(void);

/**
 * Destroy the cache 
 */
void metadata_cache_destroy(void);
#endif
