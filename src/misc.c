/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
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

/* this should be something configurable by the user */
/* or .f.e window size dependent */ 
gint max_string_length = 32;

char *remove_extention_and_basepath(const char *filename)
{
	int i = 0;
	char *buf = NULL;
	if(filename == NULL)
	{
		return NULL;
	}
	buf  = g_path_get_basename(filename);

	if(buf != NULL)
	{
		/* replace _ with spaces */
		for(i=0; i< strlen(buf);i++)
		{
			if(buf[i] == '_') buf[i] = ' ';
		}
		for(i=strlen(buf);buf[i] != '.';i--);
		/* cut of the extention */
		if(i > 0)
		{
			gchar *buf2 = g_strndup(buf, i);
			g_free(buf);
			return buf2;
		}
	}
	return buf;
}

char * shorter_string(const char *long_string)
{
	char *ret_val = NULL;
	int strl = 0;
	if(long_string == NULL)
	{
		return NULL;
	}
	strl = g_utf8_strlen(long_string, -1);
	/* this should be configurable? */
	if(strl > max_string_length)
	{
		ret_val = g_strndup(long_string, max_string_length);
		ret_val[max_string_length-3] = '.';
		ret_val[max_string_length-2] = '.';
		ret_val[max_string_length-1] = '.';

	/*	ret_val[max_string_length] = '\0';*/
	}
	else ret_val = g_strdup(long_string);
	return ret_val;

}


gchar * format_time(unsigned long seconds)
{
	int days = seconds/86400;
	int houres = (seconds % 86400)/3600;
	int minutes = (seconds % 3600)/60;
	char *ret;
	GString *str = g_string_new("Total time: ");
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
	if(seconds == 0)
	{
		g_string_append(str, "0");
	}
	ret = str->str;
	g_string_free(str, FALSE);
	return ret;
}
