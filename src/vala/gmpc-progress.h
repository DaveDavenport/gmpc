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

#ifndef __GMPC_PROGRESS_H__
#define __GMPC_PROGRESS_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_TYPE_PROGRESS (gmpc_progress_get_type ())
#define GMPC_PROGRESS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_PROGRESS, GmpcProgress))
#define GMPC_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_PROGRESS, GmpcProgressClass))
#define GMPC_IS_PROGRESS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_PROGRESS))
#define GMPC_IS_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_PROGRESS))
#define GMPC_PROGRESS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_PROGRESS, GmpcProgressClass))

typedef struct _GmpcProgress GmpcProgress;
typedef struct _GmpcProgressClass GmpcProgressClass;
typedef struct _GmpcProgressPrivate GmpcProgressPrivate;

struct _GmpcProgress {
	GtkHBox parent_instance;
	GmpcProgressPrivate * priv;
	gboolean _hide_text;
};

struct _GmpcProgressClass {
	GtkHBoxClass parent_class;
};


#define use_transition TRUE
void gmpc_progress_set_time (GmpcProgress* self, guint total, guint current);
GmpcProgress* gmpc_progress_construct (GType object_type);
GmpcProgress* gmpc_progress_new (void);
gboolean gmpc_progress_get_hide_text (GmpcProgress* self);
void gmpc_progress_set_hide_text (GmpcProgress* self, gboolean value);
GType gmpc_progress_get_type (void);


G_END_DECLS

#endif
