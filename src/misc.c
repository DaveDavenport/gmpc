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
#define BLUR_RADIUS    5
#define SHADOW_OFFSET  (BLUR_RADIUS * 4 / 5)
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
  
  if (!filter)
    filter = create_blur_filter (BLUR_RADIUS);
  
  dest = create_effect (*src, filter, 
			BLUR_RADIUS, SHADOW_OFFSET, SHADOW_OPACITY);

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
