
#ifndef __GMPC_MENU_ITEM_RATING_H__
#define __GMPC_MENU_ITEM_RATING_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include "gmpc_rating.h"

G_BEGIN_DECLS


#define GMPC_MENU_ITEM_TYPE_RATING (gmpc_menu_item_rating_get_type ())
#define GMPC_MENU_ITEM_RATING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRating))
#define GMPC_MENU_ITEM_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRatingClass))
#define GMPC_MENU_ITEM_IS_RATING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_MENU_ITEM_TYPE_RATING))
#define GMPC_MENU_ITEM_IS_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_MENU_ITEM_TYPE_RATING))
#define GMPC_MENU_ITEM_RATING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRatingClass))

typedef struct _GmpcMenuItemRating GmpcMenuItemRating;
typedef struct _GmpcMenuItemRatingClass GmpcMenuItemRatingClass;
typedef struct _GmpcMenuItemRatingPrivate GmpcMenuItemRatingPrivate;

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
struct _GmpcMenuItemRating {
	GtkMenuItem parent_instance;
	GmpcMenuItemRatingPrivate * priv;
	GtkVBox* hbox;
	GmpcRating* rating;
};

struct _GmpcMenuItemRatingClass {
	GtkMenuItemClass parent_class;
};


gint gmpc_menu_item_rating_get_rating (GmpcMenuItemRating* self);
GmpcMenuItemRating* gmpc_menu_item_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song);
GmpcMenuItemRating* gmpc_menu_item_rating_new (MpdObj* server, const mpd_Song* song);
GType gmpc_menu_item_rating_get_type (void);


G_END_DECLS

#endif
