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
		g_string_append_printf(str, "%i days ", days);
	}	
	if(houres != 0)
	{
		g_string_append_printf(str, "%i hours ", houres);
	}
	if(minutes != 0)
	{
		g_string_append_printf(str, "%i minutes ", minutes);
	}
	ret = str->str;
	g_string_free(str, FALSE);
	return ret;
}

/**
 * this draws a 1 pixel border around a pixbuf.
 * It doesn't work for all color depths (I think)
 */
void draw_pixbuf_border(GdkPixbuf *pb)
{
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

