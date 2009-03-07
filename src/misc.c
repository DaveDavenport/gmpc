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

#include <gtk/gtk.h>
#include <string.h>


#include <config.h>
#include "main.h"
#include <libmpd/libmpd-internal.h>
#include "misc.h"
#ifndef __USE_BSD
#define __USE_BSD
#endif
#include <math.h>
/**
 * format time into 
 * Total time: %i days %i hours %i minutes
 */
gchar *format_time(unsigned long seconds)
{
	return format_time_real(seconds, _(" Total time: "));
}
gchar * format_time_real(unsigned long seconds, const gchar *data)
{
	GString *str = NULL;
	int days = seconds/86400;
	int hours = (seconds % 86400)/3600;
	int minutes = (seconds % 3600)/60;
    int sec = (seconds % 60);
	char *ret;
	if(seconds == 0)
	{
		return g_strdup("");
	}
	str = g_string_new(data);
	if(days != 0)
	{
		g_string_append_printf(str, "%i %s ", days, ngettext("day", "days", days));
	}	
	if(hours != 0)
	{
		g_string_append_printf(str, "%i %s ", hours, ngettext("hour", "hours", hours));
	}
	if(minutes != 0)
	{
		g_string_append_printf(str, "%i %s ", minutes, ngettext("minute", "minutes", minutes));
	}
    if(sec != 0)
    {
		g_string_append_printf(str, "%i %s", sec, ngettext("second", "seconds", sec));
    }
	ret = str->str;
	g_string_free(str, FALSE);
	return ret;
}

/**
 * Wrapper around mpd_song_markup, that escapes the entries for use with pango markup
 * You still need to make sure "markup" is pango-markup proof
 */
void mpd_song_markup_escaped(char *buffer, int size, char *markup, mpd_Song *song2)
{
	mpd_Song *song = mpd_newSong();
	if(song2->title) {
		song->title = g_markup_escape_text(song2->title,-1);
	}
	if(song2->artist) {
		song->artist= g_markup_escape_text(song2->artist, -1);
	}                                                	
	if(song2->album) {
		song->album= g_markup_escape_text(song2->album, -1);
	}                                                	
	if(song2->track)	{
		song->track = g_markup_escape_text(song2->track, -1);
	}
	if(song2->name) {
		song->name = g_markup_escape_text(song2->name, -1);
	}
	if(song2->date) {
		song->date = g_markup_escape_text(song2->date, -1);
	}
	if(song2->genre) {
		song->genre= g_markup_escape_text(song2->genre, -1);
	}
	if(song2->composer) {
		song->composer = g_markup_escape_text(song2->composer, -1);
	}
	if(song2->disc) {
		song->disc = g_markup_escape_text(song2->disc, -1);
	}
	if(song2->comment) {
		song->comment = g_markup_escape_text(song2->comment, -1);
	}
	if(song2->file) {
		song->file = g_markup_escape_text(song2->file, -1);
	}                                                      	
	song->id    = song2->id;
	song->pos   = song2->pos;
	song->time  = song2->time;

	mpd_song_markup(buffer, size, markup, song);
	mpd_freeSong(song);
}

/**
 * Code copied from gnome-screenshot
 */

void screenshot_add_border (GdkPixbuf **src)
{
    GdkPixbuf *pb = *src;
    int x,y,width,height;
    int pixel;
    int n_channels = 0;
    int rowstride = 0;
    guchar *pixels;

    if(!pb){
        return;
    }

    rowstride = gdk_pixbuf_get_rowstride(pb);	
    n_channels = gdk_pixbuf_get_n_channels(pb);
    width = gdk_pixbuf_get_width (pb);
    height = gdk_pixbuf_get_height (pb);
    pixels = gdk_pixbuf_get_pixels(pb);

    for(y=0;y<height;y++)
    {
        for(x=0;x<width;x++)
        {
            if(y == 0 || y == (height-1) || x == 0 || x == (width-1))
            {
                for(pixel=0; pixel < n_channels;pixel++)
                {
                    pixels[x*n_channels+y*rowstride+pixel] = 0;
                }
            }
        }
    }
}

/**
 * @param: The filename or empty, to get dir path.
 *
 * Function to get the full user of filename for gmpc. 
 * On unix this is ~/.gmpc/filename 
 *
 * returns: an allocated string.
 */
gchar * gmpc_get_user_path(const gchar *filename)
{
	const gchar *homedir = g_get_home_dir();
	gchar *ret = NULL;

	/* Build the path */
#ifdef USE_CONFIG_DIR
	ret = g_build_path(G_DIR_SEPARATOR_S, homedir,".config", "gmpc", filename,NULL);
#else
	ret = g_build_path(G_DIR_SEPARATOR_S, homedir, ".gmpc", filename,NULL);
#endif

	return ret;
}
/**
 * @param: The filename or empty, to get dir path.
 *
 * Function to get the full user of filename for gmpc's cover. 
 * On unix this is ~/.covers/filename 
 *
 * returns: an allocated string.
 */
gchar * gmpc_get_covers_path(const gchar *filename)
{
	const gchar *homedir = g_get_home_dir();
	gchar *ret = NULL;

	/* Build the path */
#ifdef USE_CONFIG_DIR
	ret = g_build_path(G_DIR_SEPARATOR_S, homedir,".config", "covers", filename,NULL);
#else
	ret = g_build_path(G_DIR_SEPARATOR_S, homedir, ".covers", filename,NULL);
#endif
	return ret;
}

/**
 * Get's the full path to an image,
 * While this is compile time on linux, windows
 * needs to determine it run-time.
 */
char *gmpc_get_full_image_path(const char *filename)
{
    gchar *path;
#ifdef WIN32
    gchar *packagedir;
    packagedir = g_win32_get_package_installation_directory_of_module(NULL);
    debug_printf(DEBUG_INFO, "Got %s as package installation dir", packagedir);

    path = g_build_filename(packagedir, "share","gmpc","icons", filename, NULL);

    q_free(packagedir);

#else
    path = g_build_filename(G_DIR_SEPARATOR_S, PIXMAP_PATH, filename,NULL);
#endif
    return path;
}

/** 
 * Get the full path to the glade files.
 * While this is compile time on linux, windows 
 * needs to determine it run-time
 */
char *gmpc_get_full_glade_path(const char *filename)
{
    gchar *path;
#ifdef WIN32
    gchar *packagedir;
    packagedir = g_win32_get_package_installation_directory_of_module(NULL);
    debug_printf(DEBUG_INFO, "Got %s as package installation dir", packagedir);

    path = g_build_filename(packagedir, "share","gmpc", filename, NULL);

    q_free(packagedir);

#else
    path = g_build_filename(G_DIR_SEPARATOR_S, GLADE_PATH, filename,NULL);
#endif
    return path;
}

void open_uri(const gchar *uri)
{
	int result;
	gchar *command;
    GError *error = NULL;
#ifdef WIN32
	gchar *browser_command = cfg_get_single_value_as_string_with_default(config, "Misc","browser-win32", "cmd /c start %s");
#else
#ifdef OSX
	gchar *browser_command = cfg_get_single_value_as_string_with_default(config, "Misc","browser-osx", "open '%s'");
#else
	gchar *browser_command = cfg_get_single_value_as_string_with_default(config, "Misc","browser", "xdg-open '%s'");
#endif
#endif
	command	= g_strdup_printf(browser_command, uri);
	result = g_spawn_command_line_async (command, &error);
	if(error)
	{

		gchar *str = g_markup_printf_escaped("%s: '%s': %s", _("Failed to execute"),command,error->message); 
		playlist3_show_error_message(str, ERROR_WARNING);
		g_free(str);
        g_error_free(error);
        error = NULL;
	}
	g_free(browser_command);
	g_free(command);
}

int *split_version(const char *uri)
{
    int *retv= g_malloc0(4*sizeof(int));
    char **sp  = g_strsplit(uri, ".",4);
    int i;
    for(i=0;i<4 && sp[i];i++)
    {
        retv[i] = atoi(sp[i]);
    }
    g_strfreev(sp);
    return retv;
}

MpdData * misc_sort_mpddata(MpdData *data, GCompareDataFunc func, void *user_data)
{
	int j;
	int i=0;
    MpdData_real *node;
    MpdData_real **nodes;
    INIT_TIC_TAC(); 
    
    if(data == NULL)
        return NULL;
    

    for(node = (MpdData_real*)mpd_data_get_first(data);node;node =(MpdData_real *)mpd_data_get_next_real((MpdData *)node, FALSE))i++;
    TEC("Counted items");

    nodes = g_malloc0(i*sizeof(MpdData_real *));
    
    node = (MpdData_real *)mpd_data_get_first(data);
    for(j=0;j<i;j++)
    {
        nodes[j] = node;
        node =(MpdData_real *) mpd_data_get_next_real((MpdData *)node, FALSE);
    }
    TEC("Created array");
    g_qsort_with_data(nodes, i, sizeof(*nodes), func,user_data);
    TEC("Sorted array");
    nodes[0]->prev = NULL;
    nodes[0]->next= NULL;
    nodes[0]->first=nodes[0];
    for(j=1;j<i;j++)
    {
        nodes[j]->prev = nodes[j-1];
        nodes[j]->next = NULL;
        nodes[j]->prev->next = nodes[j];
        nodes[j]->first = nodes[0];
    }
    data =(MpdData*)nodes[0];
    g_free(nodes);
    TEC("Recreated linked list");
    return data;
}
static gint __add_sort(gpointer aa, gpointer bb, gpointer data)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;
    if(a->type == MPD_DATA_TYPE_DIRECTORY && b->type == MPD_DATA_TYPE_DIRECTORY)
    {
        if(a->directory== NULL && b->directory != NULL)
            return -1;
        else if(b->directory == NULL && a->directory != NULL)
            return 1;
        else if (a->directory  && b->directory)
        {
            int val;
            if(a->directory && b->directory) {
                gchar *sa,*sb;
                sa = g_utf8_strdown(a->directory, -1);
                sb = g_utf8_strdown(b->directory, -1);
                val = g_utf8_collate(sa,sb);
                g_free(sa);
                g_free(sb);
            } else {
                val = (a == NULL)?((b==NULL)?0:-1):1;
            }
            return val;
        }
    }
    if(a->type == MPD_DATA_TYPE_TAG && b->type == MPD_DATA_TYPE_TAG)
    {
        if(a->tag_type != b->tag_type)
            return a->tag_type - b->tag_type;
        if(a->tag== NULL && b->tag != NULL)
            return -1;
        else if(b->tag == NULL && a->tag != NULL)
            return 1;
        else if (a->tag  && b->tag)
        {
            int val;
            if(a->tag && b->tag) {
                gchar *sa,*sb;
                sa = g_utf8_strdown(a->tag, -1);
                sb = g_utf8_strdown(b->tag, -1);
                val = g_utf8_collate(sa,sb);
                g_free(sa);
                g_free(sb);
            } else {
                val = (a == NULL)?((b==NULL)?0:-1):1;
            }
            return val;
        }
    }
    if(a->type == MPD_DATA_TYPE_SONG && b->type == MPD_DATA_TYPE_SONG)
    {
        if(a->song->date == NULL && b->song->date != NULL)
            return -1;
        else if(b->song->date == NULL && a->song->date != NULL)
            return 1;
        else if (a->song->date  && b->song->date)
        {
            int compv = atoi(a->song->date)-atoi(b->song->date); 
            if(compv != 0)
                return compv;
        }
        if(a->song->albumartist == NULL && b->song->albumartist != NULL)
            return -1;
        else if(b->song->albumartist == NULL && a->song->albumartist != NULL)
            return 1;
        else if (a->song->albumartist  && b->song->albumartist)
        {
            int compv = g_utf8_collate(a->song->albumartist, b->song->albumartist);
            if(compv != 0)
            {
                return compv;
            }
        }
        if(a->song->album == NULL && b->song->album != NULL)
            return -1;
        else if(b->song->album == NULL && a->song->album != NULL)
            return 1;
        else if (a->song->album  && b->song->album)
        {
            int compv = g_utf8_collate(a->song->album, b->song->album);

            if(compv != 0)
            {
                return compv;
            }
        }
        if(a->song->disc == NULL && b->song->disc != NULL)
            return -1;
        else if(b->song->disc == NULL && a->song->disc != NULL)
            return 1;
        else if (a->song->disc  && b->song->disc)
        {
            int compv = g_utf8_collate(a->song->disc, b->song->disc);
            if(compv != 0)
            {
                return compv;
            }
        }
        if(a->song->track == NULL && b->song->track != NULL)
            return -1;
        else if(b->song->track == NULL && a->song->track != NULL)
            return 1;
        else if (a->song->track  && b->song->track)
        {
            int compv = atoi(a->song->track)-atoi(b->song->track); 
            return compv;
        }
    }
    return a->type - b->type;
}
MpdData * misc_sort_mpddata_by_album_disc_track(MpdData *data)
{
    return misc_sort_mpddata(data, (GCompareDataFunc)__add_sort,NULL);
}
static gint __sort_filename(gpointer aa, gpointer bb, gpointer data)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;

    if(a->type == MPD_DATA_TYPE_SONG && b->type == MPD_DATA_TYPE_SONG)
    {
        return strcmp(a->song->file, b->song->file);
    }
    else if(a->type == MPD_DATA_TYPE_TAG && b->type == MPD_DATA_TYPE_TAG)
    {
        if (a->tag_type == b->tag_type)
            return strcmp(a->tag, b->tag);
    }
    return a->type - b->type;
}

MpdData *misc_mpddata_remove_duplicate_songs(MpdData *data)
{
    MpdData_real *node;
    if(!data)
        return NULL;
    /* sort on file filename, this is our key */
    node  = (MpdData_real *)misc_sort_mpddata(data,(GCompareDataFunc) __sort_filename, NULL);
    /* loop through it */
    while(node->next) 
    {
        if(node->type == MPD_DATA_TYPE_SONG && node->next->type == MPD_DATA_TYPE_SONG)
        {
            if(strcmp(node->song->file, node->next->song->file) == 0)
            {
                node = (MpdData_real *) mpd_data_delete_item((MpdData *)node);
                continue;
            }
        }
        else if(node->type == MPD_DATA_TYPE_TAG && node->next->type == MPD_DATA_TYPE_TAG)
        {
            if(strcmp(node->tag, node->next->tag) == 0)
            {
                node = (MpdData_real *) mpd_data_delete_item((MpdData *)node);
                continue;
            }
        }
        node = node->next;
    }
    return (MpdData *)node->first;
}


gchar ** tokenize_string(const gchar *string)
{
    gchar ** result = NULL; 	/* the result with tokens 		*/
	int i = 0;		/* position in string 			*/
	int br = 0;		/* number for open ()[]'s		*/
	int bpos = 0;		/* begin position of the cur. token 	*/
    int bstop = 0;
	int tokens=0;
    int length = 0;
	if(string == NULL) return NULL;
    length = strlen(string);
	for(i=0; i <= length;i++)
	{
		/* check for opening  [( */
		if((br !=0 || bpos == i) && (string[i] == '(' || string[i] == '[' || string[i] == '{')){
            if(!br && bpos == i)
                bpos = i+1;
            br++;
        }
		/* check closing */
		else if(br && (string[i] == ')' || string[i] == ']' || string[i] == '}')){
            br--;
            bstop = i;
            if(!br && (string[i+1] == ' ' || string[i+1] == '\0')) { 
                i++;
            }
        }
        else 
            bstop=i;
        /* if multiple spaces at begin of token skip them */
		if(string[i] == ' ' && !(i-bpos))bpos++;
		/* if token end or string end add token to list */
		else if((string[i] == ' ' && !br) || string[i] == '\0')
		{
            if((bstop-bpos) >0)
            {
                result = g_realloc(result,(tokens+2)*sizeof(gchar *));
                result[tokens] = g_strndup(&string[bpos], bstop-bpos);
                result[tokens+1] = NULL;
                bpos = i+1;
                bstop = bpos;
                tokens++;
            }
		}

	}
	return result;
}


gboolean misc_header_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{

	int width = widget->allocation.width;
	int height = widget->allocation.height;
	
	gtk_paint_flat_box(widget->style, 
					widget->window, 
					GTK_STATE_SELECTED,
					GTK_SHADOW_NONE,
					NULL, 
					widget,
					"cell_odd",
					0,0,width,height);
	gtk_paint_focus(widget->style, widget->window, 
				GTK_STATE_NORMAL, 
				NULL, 
				widget,
				"button",
				0,0,width,height);
	return FALSE;
}

gchar * mpd_song_checksum(const mpd_Song *song)
{
    gchar *retv = NULL;
    if(song)
    {
        GChecksum *cs=g_checksum_new(G_CHECKSUM_SHA256);
        if(song->file) g_checksum_update(cs, song->file, -1);
        if(song->artist) g_checksum_update(cs, song->artist, -1);
        if(song->title) g_checksum_update(cs, song->title, -1);
        if(song->album) g_checksum_update(cs, song->album, -1);
        if(song->track) g_checksum_update(cs, song->track, -1);
        if(song->name) g_checksum_update(cs, song->name, -1);
        if(song->date) g_checksum_update(cs, song->date, -1);
        if(song->genre) g_checksum_update(cs, song->genre, -1);
        if(song->composer) g_checksum_update(cs, song->composer, -1);
        if(song->performer) g_checksum_update(cs, song->performer, -1);
        if(song->disc) g_checksum_update(cs, song->disc, -1);
        if(song->albumartist) g_checksum_update(cs, song->albumartist, -1);

        retv = g_strdup(g_checksum_get_string(cs));
        g_checksum_free(cs);
    }
    return retv;
}
