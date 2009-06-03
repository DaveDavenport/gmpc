
#ifndef __GMPC_RATING_H__
#define __GMPC_RATING_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>

G_BEGIN_DECLS


#define GMPC_TYPE_RATING (gmpc_rating_get_type ())
#define GMPC_RATING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_RATING, GmpcRating))
#define GMPC_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_RATING, GmpcRatingClass))
#define GMPC_IS_RATING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_RATING))
#define GMPC_IS_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_RATING))
#define GMPC_RATING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_RATING, GmpcRatingClass))

typedef struct _GmpcRating GmpcRating;
typedef struct _GmpcRatingClass GmpcRatingClass;
typedef struct _GmpcRatingPrivate GmpcRatingPrivate;

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
struct _GmpcRating {
	GtkFrame parent_instance;
	GmpcRatingPrivate * priv;
	GtkEventBox* event_box;
};

struct _GmpcRatingClass {
	GtkFrameClass parent_class;
};


GType gmpc_rating_get_type (void);
gboolean gmpc_rating_button_press_event_callback (GmpcRating* self, GtkEventBox* wid, const GdkEventButton* event);
GmpcRating* gmpc_rating_new (MpdObj* server, const mpd_Song* song);
GmpcRating* gmpc_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song);
void gmpc_rating_set_rating (GmpcRating* self, gint rating);
void gmpc_rating_update (GmpcRating* self);


G_END_DECLS

#endif
