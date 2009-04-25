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

#ifndef __GMPC_LISTSTORE_SORT_H__
#define __GMPC_LISTSTORE_SORT_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_LISTSTORE_TYPE_SORT (gmpc_liststore_sort_get_type ())
#define GMPC_LISTSTORE_SORT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_LISTSTORE_TYPE_SORT, GmpcListstoreSort))
#define GMPC_LISTSTORE_SORT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_LISTSTORE_TYPE_SORT, GmpcListstoreSortClass))
#define GMPC_LISTSTORE_IS_SORT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_LISTSTORE_TYPE_SORT))
#define GMPC_LISTSTORE_IS_SORT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_LISTSTORE_TYPE_SORT))
#define GMPC_LISTSTORE_SORT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_LISTSTORE_TYPE_SORT, GmpcListstoreSortClass))

typedef struct _GmpcListstoreSort GmpcListstoreSort;
typedef struct _GmpcListstoreSortClass GmpcListstoreSortClass;
typedef struct _GmpcListstoreSortPrivate GmpcListstoreSortPrivate;

struct _GmpcListstoreSort {
	GtkListStore parent_instance;
	GmpcListstoreSortPrivate * priv;
};

struct _GmpcListstoreSortClass {
	GtkListStoreClass parent_class;
};


GmpcListstoreSort* gmpc_liststore_sort_construct (GType object_type);
GmpcListstoreSort* gmpc_liststore_sort_new (void);
GType gmpc_liststore_sort_get_type (void);


G_END_DECLS

#endif
