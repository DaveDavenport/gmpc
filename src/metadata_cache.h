/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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

MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, MetaData **met);
mpd_Song *rewrite_mpd_song(mpd_Song *tsong, MetaDataType type);

void meta_data_set_cache_real(mpd_Song *song, MetaDataResult result, MetaData *met);

void meta_data_set_cache(mpd_Song *song, MetaDataResult result, MetaData *met);

void metadata_cache_init(void);

void metadata_cache_cleanup(void);
void metadata_cache_destroy(void);
#endif
