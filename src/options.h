/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2011-2012 Qball Cow <qball@gmpclient.org>
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
#ifndef __OPTIONS_h__
#define __OPTIONS_H__

/* see options.h entries[]  to see the use of every entry. */
typedef struct _Options
{
	gboolean show_version;
	gboolean disable_plugins;
	gboolean start_hidden;
	gboolean clean_config;
	gboolean quit;
	gboolean do_debug_updates;
	gboolean show_bug_information;
	gboolean fullscreen;
	gchar *config_path;
	gint debug_level;
	gchar *profile_name;
	char *icon_theme;
}Options;

extern Options settings;

gboolean parse_options(int *argc, char ***argv);
#endif
