/*
 *Copyright (C) 2004-2007 Qball Cow <qball@sarine.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
extern GladeXML *pl3_xml;
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
		g_string_append_printf(str, "%i %s", minutes, ngettext("minute", "minutes", minutes));
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
	song->id = song2->id;
	song->pos = song2->pos;
	song->time = song2->time;

	mpd_song_markup(buffer, size, markup, song);
	mpd_freeSong(song);
}

/**
 * Code copied from gnome-screenshot
 */
#define BLUR_RADIUS    4
#define SHADOW_OFFSET  (BLUR_RADIUS * 3 / 5)
#define SHADOW_OPACITY 0.5

#define OUTLINE_RADIUS  1.1
#define OUTLINE_OFFSET  0
#define OUTLINE_OPACITY 1.0

#define dist(x0, y0, x1, y1) sqrt(((x0) - (x1))*((x0) - (x1)) + ((y0) - (y1))*((y0) - (y1)))

typedef struct {
  int size;
  double *data;
} ConvFilter;

static double
gaussian (double x, double y, double r)
{
    return ((1 / (2 * M_PI * r)) *
	    exp ((- (x * x + y * y)) / (2 * r * r)));
}

static ConvFilter *
create_blur_filter (int radius)
{
  ConvFilter *filter;
  int x, y;
  double sum;
  
  filter = g_new0 (ConvFilter, 1);
  filter->size = radius * 2 + 1;
  filter->data = g_new (double, filter->size * filter->size);

  sum = 0.0;
  
  for (y = 0 ; y < filter->size; y++)
    {
      for (x = 0 ; x < filter->size; x++)
	{
	  sum += filter->data[y * filter->size + x] = gaussian (x - (filter->size >> 1),
								y - (filter->size >> 1),
								radius);
	}
    }

  for (y = 0; y < filter->size; y++)
    {
      for (x = 0; x < filter->size; x++)
	{
	  filter->data[y * filter->size + x] /= sum;
	}
    }

  return filter;
  
}


static GdkPixbuf *
create_effect (GdkPixbuf *src, ConvFilter const *filter, int radius, int offset, double opacity)
{
    GdkPixbuf *dest;
    int x, y, i, j;
    int src_x, src_y;
    int suma;
    int dest_width, dest_height;
    int src_width, src_height;
    int src_rowstride, dest_rowstride;
    gboolean src_has_alpha;


    guchar *src_pixels, *dest_pixels;

    /* No source, nothing todo */
    if(!src) {
        return NULL;
    }

    src_has_alpha =  gdk_pixbuf_get_has_alpha (src);

    src_width = gdk_pixbuf_get_width (src);
    src_height = gdk_pixbuf_get_height (src);
    dest_width = src_width + 2 * radius + offset;
    dest_height = src_height + 2 * radius + offset;

    dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
            TRUE,
            gdk_pixbuf_get_bits_per_sample (src),
            dest_width, dest_height);

    gdk_pixbuf_fill (dest, 0);  

    src_pixels = gdk_pixbuf_get_pixels (src);
    src_rowstride = gdk_pixbuf_get_rowstride (src);

    dest_pixels = gdk_pixbuf_get_pixels (dest);
    dest_rowstride = gdk_pixbuf_get_rowstride (dest);

    for (y = 0; y < dest_height; y++)
    {
        for (x = 0; x < dest_width; x++)
        {
            suma = 0;

            src_x = x - radius;
            src_y = y - radius;

            /* We don't need to compute effect here, since this pixel will be 
             * discarded when compositing */
            if (src_x >= 0 && src_x < src_width &&
                    src_y >= 0 && src_y < src_height && 
                    (!src_has_alpha ||
                     src_pixels [src_y * src_rowstride + src_x * 4 + 3] == 0xFF))
                continue;

            for (i = 0; i < filter->size; i++)
            {
                for (j = 0; j < filter->size; j++)
                {
                    src_y = -(radius + offset) + y - (filter->size >> 1) + i;
                    src_x = -(radius + offset) + x - (filter->size >> 1) + j;

                    if (src_y < 0 || src_y >= src_height ||
                            src_x < 0 || src_x >= src_width)
                        continue;

                    suma += ( src_has_alpha ? 
                            src_pixels [src_y * src_rowstride + src_x * 4 + 3] :
                            0xFF ) * filter->data [i * filter->size + j];
                }
            }

            dest_pixels [y * dest_rowstride + x * 4 + 3] = CLAMP (suma * opacity, 0x00, 0xFF);
        }
    }

    return dest;
}

void
screenshot_add_shadow (GdkPixbuf **src)
{
    GdkPixbuf *dest;
    static ConvFilter *filter = NULL;

    /* If *src == NULL, return nothing to do 
    */
    if(!(*src))
        return;


    if (!filter)
        filter = create_blur_filter (BLUR_RADIUS);

    dest = create_effect (*src, filter, 
            BLUR_RADIUS, SHADOW_OFFSET, SHADOW_OPACITY);

    /* Failed to create effect */
    if (dest == NULL)
        return;

    gdk_pixbuf_composite (*src, dest,
            BLUR_RADIUS, BLUR_RADIUS,
            gdk_pixbuf_get_width (*src),
            gdk_pixbuf_get_height (*src),
            BLUR_RADIUS, BLUR_RADIUS, 1.0, 1.0,
            GDK_INTERP_NEAREST, 255);
    g_object_unref (*src);
    *src = dest;
}

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
char *gmpc_get_full_image_path(char *filename)
{
    gchar *path;
#ifdef WIN32
    gchar *packagedir;
    packagedir = g_win32_get_package_installation_directory(NULL, NULL);
    debug_printf(DEBUG_INFO, "Got %s as package installation dir", packagedir);

    path = g_build_filename(packagedir, "data", "images", filename, NULL);

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
char *gmpc_get_full_glade_path(char *filename)
{
    gchar *path;
#ifdef WIN32
    gchar *packagedir;
    packagedir = g_win32_get_package_installation_directory(NULL, NULL);
    debug_printf(DEBUG_INFO, "Got %s as package installation dir", packagedir);

    path = g_build_filename(packagedir, "data", "glade", filename, NULL);

    q_free(packagedir);

#else
    path = g_build_filename(G_DIR_SEPARATOR_S, GLADE_PATH, filename,NULL);
#endif
    return path;
}

static gint
count_of_char_in_string (const gchar * string,
			 const gchar c)
{
	int cnt = 0;
	for(; *string; string++) {
		if (*string == c) cnt++;
	}
	return cnt;
}

gchar *
escape_single_quotes (const gchar * string)
{
	GString * gs;

	if (string == NULL) {
		return NULL;
	}

	if (count_of_char_in_string (string, '\'') == 0) {
		return g_strdup(string);
	}
	gs = g_string_new ("");
	for(; *string; string++) {
		if (*string == '\'') {
			g_string_append(gs, "'\\''");
		}
		else {
			g_string_append_c(gs, *string);
		}
	}
	return g_string_free (gs, FALSE);
}


void open_uri(const gchar *uri)
{
	int result;
	gchar *command;
	gchar *browser_command = cfg_get_single_value_as_string_with_default(config, "Misc","browser", "xdg-open '%s'");
	command	= g_strdup_printf(browser_command, uri);
	result = g_spawn_command_line_async (command, NULL);
	if(!result)
	{

		gchar *str = g_markup_printf_escaped("%s: '%s'", _("Failed to execute"),command); 
		playlist3_show_error_message(str, ERROR_WARNING);
		g_free(str);
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
    
    
    if(data == NULL)
        return NULL;

    for(node = (MpdData_real*)mpd_data_get_first(data);node;node =(MpdData_real *)mpd_data_get_next_real((MpdData *)node, FALSE))i++;

    nodes = g_malloc0(i*sizeof(MpdData_real *));
    
    node = (MpdData_real *)mpd_data_get_first(data);
    for(j=0;j<i;j++)
    {
        nodes[j] = node;
        node =(MpdData_real *) mpd_data_get_next_real((MpdData *)node, FALSE);
    }
    g_qsort_with_data(nodes, i, sizeof(*nodes), func,user_data);
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
        /*
        if(a->song->artist == NULL && b->song->artist != NULL)
            return -1;
        else if(b->song->artist == NULL && a->song->artist != NULL)
            return 1;
        else if (a->song->artist  && b->song->artist)
        {
            int compv = strcmp(a->song->artist, b->song->artist);
            if(compv != 0)
            {
                return compv;
            }
        }
        */
        if(a->song->album == NULL && b->song->album != NULL)
            return -1;
        else if(b->song->album == NULL && a->song->album != NULL)
            return 1;
        else if (a->song->album  && b->song->album)
        {
            int compv = strcmp(a->song->album, b->song->album);

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
            int compv = strcmp(a->song->disc, b->song->disc);
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
	if(string == NULL) return NULL;
	for(i=0; i <= strlen(string);i++)
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
