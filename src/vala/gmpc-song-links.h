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

#ifndef __GMPC_SONG_LINKS_H__
#define __GMPC_SONG_LINKS_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libmpd/libmpdclient.h>

G_BEGIN_DECLS


#define GMPC_SONG_TYPE_LINKS (gmpc_song_links_get_type ())
#define GMPC_SONG_LINKS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_SONG_TYPE_LINKS, GmpcSongLinks))
#define GMPC_SONG_LINKS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_SONG_TYPE_LINKS, GmpcSongLinksClass))
#define GMPC_SONG_IS_LINKS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_SONG_TYPE_LINKS))
#define GMPC_SONG_IS_LINKS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_SONG_TYPE_LINKS))
#define GMPC_SONG_LINKS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_SONG_TYPE_LINKS, GmpcSongLinksClass))

typedef struct _GmpcSongLinks GmpcSongLinks;
typedef struct _GmpcSongLinksClass GmpcSongLinksClass;
typedef struct _GmpcSongLinksPrivate GmpcSongLinksPrivate;

#define GMPC_SONG_LINKS_TYPE_TYPE (gmpc_song_links_type_get_type ())

struct _GmpcSongLinks {
	GtkFrame parent_instance;
	GmpcSongLinksPrivate * priv;
};

struct _GmpcSongLinksClass {
	GtkFrameClass parent_class;
};

typedef enum  {
	GMPC_SONG_LINKS_TYPE_ARTIST,
	GMPC_SONG_LINKS_TYPE_ALBUM,
	GMPC_SONG_LINKS_TYPE_SONG
} GmpcSongLinksType;


#define use_transition TRUE
GType gmpc_song_links_type_get_type (void);
GmpcSongLinks* gmpc_song_links_construct (GType object_type, GmpcSongLinksType type, const mpd_Song* song);
GmpcSongLinks* gmpc_song_links_new (GmpcSongLinksType type, const mpd_Song* song);
GType gmpc_song_links_get_type (void);


G_END_DECLS

#endif
