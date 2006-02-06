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

gchar * format_time(unsigned long seconds)
{
	int days = seconds/86400;
	int houres = (seconds % 86400)/3600;
	int minutes = (seconds % 3600)/60;
	char *ret;
	if(seconds == 0)
	{
		GString *str = g_string_new(NULL);
		return str->str;
	}
	GString *str = g_string_new(" Total time: ");
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
