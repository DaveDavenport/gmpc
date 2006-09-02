/*
 *Copyright (C) 2004-2006 Qball Cow <Qball@qballcow.nl>
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
#include <math.h>
#include "main.h"
#include "misc.h"
/**
 * format time into 
 * Total time: %i days %i hours %i minutes
 */
gchar * format_time(unsigned long seconds)
{
	GString *str = NULL;
	int days = seconds/86400;
	int houres = (seconds % 86400)/3600;
	int minutes = (seconds % 3600)/60;
	char *ret;
	if(seconds == 0)
	{
		return g_strdup("");
	}
	str = g_string_new(" Total time: ");
	if(days != 0)
	{
		
		g_string_append_printf(str, "%i %s ", days, (days == 1)?_("day"):_("days"));
	}	
	if(houres != 0)
	{
		g_string_append_printf(str, "%i %s ", houres, (houres == 1)?_("hour"):_("hours"));
	}
	if(minutes != 0)
	{
		g_string_append_printf(str, "%i %s", minutes,(minutes==1)?_("minute"):_("minutes"));
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
	char *temp = NULL;
	mpd_Song *song = mpd_songDup(song2);
	if(song->title)
	{
		temp = song->title;
		song->title = g_markup_escape_text(temp,-1);
		g_free(temp);
	}
	if(song->artist)
	{
		temp = song->artist;
		song->artist= g_markup_escape_text(temp, -1);
		g_free(temp);
	}                                                	
	if(song->album)
	{
		temp = song->album;
		song->album= g_markup_escape_text(temp, -1);
		g_free(temp);
	}                                                	
	if(song->track)
	{
		temp = song->track;
		song->track = g_markup_escape_text(temp, -1);
		g_free(temp);
	}
	if(song->name)
	{
		temp = song->name;
		song->name = g_markup_escape_text(temp, -1);
		g_free(temp);
	}
	if(song->date)
	{
		temp = song->date;
		song->date = g_markup_escape_text(temp, -1);
		g_free(temp);
	}
	if(song->genre)
	{
		temp = song->genre;
		song->genre= g_markup_escape_text(temp, -1);
		g_free(temp);
	}
	if(song->composer)
	{
		temp = song->composer;
		song->composer = g_markup_escape_text(temp, -1);
		g_free(temp);
	}
	if(song->disc)
	{
		temp = song->disc;
		song->disc = g_markup_escape_text(temp, -1);
		g_free(temp);
	}
	if(song->comment)
	{
		temp = song->comment;
		song->comment = g_markup_escape_text(temp, -1);
		g_free(temp);
	}
	
	mpd_song_markup(buffer, size, markup, song);
	mpd_freeSong(song);
}

#define BLUR_RADIUS    5
#define SHADOW_OFFSET  (BLUR_RADIUS * 4 / 5)
#define SHADOW_OPACITY 0.7

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

static ConvFilter *
create_outline_filter (int radius)
{
  ConvFilter *filter;
  int x, y;
  
  filter = g_new0 (ConvFilter, 1);
  filter->size = radius * 2 + 1;
  filter->data = g_new (double, filter->size * filter->size);

  for (y = 0; y < filter->size; y++)
  {
    for (x = 0 ; x < filter->size; x++) 
    {
      filter->data [y * filter->size + x] = (dist (x, y, radius, radius) < radius + 0.2) ?
	      1.0 : 0.0;
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

void
screenshot_add_border (GdkPixbuf **src)
{
  GdkPixbuf *dest;
  static ConvFilter *filter = NULL;
  
  if (!filter)
    filter = create_outline_filter (OUTLINE_RADIUS);
  
  dest = create_effect (*src, filter, 
			OUTLINE_RADIUS, OUTLINE_OFFSET, OUTLINE_OPACITY);

  if (dest == NULL)
	  return;

  gdk_pixbuf_composite (*src, dest,
			OUTLINE_RADIUS, OUTLINE_RADIUS,
			gdk_pixbuf_get_width (*src),
			gdk_pixbuf_get_height (*src),
			OUTLINE_RADIUS, OUTLINE_RADIUS, 1.0, 1.0,
			GDK_INTERP_NEAREST, 255);
  g_object_unref (*src);
  *src = dest;
}
