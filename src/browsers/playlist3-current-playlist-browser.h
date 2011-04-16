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

#ifndef  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__
#define  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__

typedef struct _PlayQueuePlugin      PlayQueuePlugin;
typedef struct _PlayQueuePluginClass PlayQueuePluginClass;
typedef struct _PlayQueuePluginPrivate PlayQueuePluginPrivate;
struct _PlayQueuePlugin
{
        GmpcPluginBase  parent_instance;
        PlayQueuePluginPrivate *priv;
};

struct _PlayQueuePluginClass
{
        GmpcPluginBaseClass parent_class;
};


PlayQueuePlugin * play_queue_plugin_new(const gchar *uid);

#endif
