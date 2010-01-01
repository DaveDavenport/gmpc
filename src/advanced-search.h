/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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
#ifndef __ADVANCED_SEARCH_H__
#define __ADVANCED_SEARCH_H__

/**
 * Initialize the advanced_search system.
 */
void advanced_search_init(void);
/**
 * Update the advanced_search regex to include only the supported tags
 */
void advanced_search_update_taglist(void);
/**
 * Destroy all the advanced_search system and clean all allocated memory.
 */
void advanced_search_destroy(void);
/**
 * Execute query.
 * @param query the query to execute.
 * @param playlist set to TRUE to search only songs in the playlist.
 *
 * @returns the search result in a #MpdData list.
 */
MpdData *advanced_search(const gchar *query, int playlist);

#endif
