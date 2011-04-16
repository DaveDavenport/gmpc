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
#include <config.h>
#include <glib.h>
#include "main.h"
#include "misc.h"
#include "advanced-search.h"

static GRegex *search_regex = NULL;

/**
 * Initialize the advanced_search system.
 */
void advanced_search_init(void)
{
	int i = 0;
	GString *string = g_string_new("(");
	if (search_regex) {
		g_regex_unref(search_regex);
		search_regex = NULL;
	}
	for (i = 0; i < MPD_TAG_NUM_OF_ITEM_TYPES; i++)
	{
		if (mpd_server_tag_supported(connection, i))
		{
			g_string_append(string, mpdTagItemKeys[i]);
			if (i < (MPD_TAG_NUM_OF_ITEM_TYPES - 1))
				g_string_append(string, "|");
		}
	}
	g_string_append(string, ")[ ]*[=:][ ]*|[ ]*(\\|\\|)[ ]*");
	search_regex = g_regex_new(string->str, G_REGEX_CASELESS, 0, NULL);
	g_string_free(string, TRUE);
}

/**
 * Update the advanced_search regex to include only the supported tags
 */
void advanced_search_update_taglist(void)
{
	advanced_search_init();
}

/**
 * Destroy all the advanced_search system and clean all allocated memory.
 */
void advanced_search_destroy(void)
{
	if (search_regex)
		g_regex_unref(search_regex);
	search_regex = NULL;
}

/**
 * Execute query.
 * @param query the query to execute.
 * @param playlist set to TRUE to search only songs in the playlist.
 *
 * @returns the search result in a #MpdData list.
 */
MpdData *advanced_search(const gchar * query, int in_playlist)
{
	MpdData *data_return = NULL;
	gchar **text = g_regex_split(search_regex, query, 0);
	int i = 0;
	gboolean found = FALSE;
	for (i = 0; text && text[i]; i++)
	{
		int type;
		/* Or sign, if hit, a new query is started */
		if (strcmp(text[i], "||") == 0)
		{
			MpdData *data;
			/* Commit the currently in active search and append the results */
			if (in_playlist)
				data = mpd_playlist_search_commit(connection);
			else
				data = mpd_database_search_commit(connection);
			data_return = mpd_data_concatenate(data_return, data);
			found = FALSE;
			continue;
		}
		/* empty element */
		if (text[i][0] == '\0')
			continue;

		/* Parse the tag name. */
		type = mpd_misc_get_tag_by_name(g_strstrip(text[i]));
		if (type != MPD_TAG_NOT_FOUND && text[i + 1])
		{
			gchar **split = tokenize_string(text[i + 1]);
			int j;
			for (j = 0; split && split[j]; j++)
			{
				if (!found)
				{
					if (in_playlist)
						mpd_playlist_search_start(connection, FALSE);
					else
						mpd_database_search_start(connection, FALSE);
					found = TRUE;
				}
				if (in_playlist)
					mpd_playlist_search_add_constraint(connection, type, g_strstrip(split[j]));
				else
					mpd_database_search_add_constraint(connection, type, g_strstrip(split[j]));
			}
			if (split)
				g_strfreev(split);
			i++;
		} else
		{
			gchar **split = tokenize_string(text[i]);
			int j;
			for (j = 0; split && split[j]; j++)
			{
				if (!found)
				{
					if (in_playlist)
						mpd_playlist_search_start(connection, FALSE);
					else
						mpd_database_search_start(connection, FALSE);
					found = TRUE;
				}
				if (in_playlist)
					mpd_playlist_search_add_constraint(connection, MPD_TAG_ITEM_ANY, split[j]);
				else
					mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ANY, split[j]);
			}
			if (split)
				g_strfreev(split);
		}
	}
	if (text)
		g_strfreev(text);
	/* Execute the active search and append the results */
	if (found)
	{
		MpdData *data;
		if (in_playlist)
			data = mpd_playlist_search_commit(connection);
		else
			data = mpd_database_search_commit(connection);
		data_return = mpd_data_concatenate(data_return, data);
	}
	/* remove possible duplicates (because of concatenating queries) */
	return misc_mpddata_remove_duplicate_songs(data_return);
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
