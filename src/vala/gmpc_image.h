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

#ifndef __GMPC_IMAGE_H__
#define __GMPC_IMAGE_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixdata.h>

G_BEGIN_DECLS


#define GMPC_TYPE_IMAGE (gmpc_image_get_type ())
#define GMPC_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_IMAGE, GmpcImage))
#define GMPC_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_IMAGE, GmpcImageClass))
#define GMPC_IS_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_IMAGE))
#define GMPC_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_IMAGE))
#define GMPC_IMAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_IMAGE, GmpcImageClass))

typedef struct _GmpcImage GmpcImage;
typedef struct _GmpcImageClass GmpcImageClass;
typedef struct _GmpcImagePrivate GmpcImagePrivate;

/**
 * Widget that shows a pixbuf by nicely fadeing in and out.
 * Draws a nice border.
 */
struct _GmpcImage {
	GtkEventBox parent_instance;
	GmpcImagePrivate * priv;
};

struct _GmpcImageClass {
	GtkEventBoxClass parent_class;
};


void gmpc_image_set_pixbuf (GmpcImage* self, GdkPixbuf* buf, gboolean border);
void gmpc_image_clear_pixbuf (GmpcImage* self);
GmpcImage* gmpc_image_new (void);
GType gmpc_image_get_type (void);


G_END_DECLS

#endif
