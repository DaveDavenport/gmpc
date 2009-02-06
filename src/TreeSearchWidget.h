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

#ifndef __TREESEARCH_H__
#define __TREESEARCH_H__


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TYPE_TREESEARCH            (treesearch_get_type ())
#define TREESEARCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TREESEARCH, TreeSearch))
#define TREESEARCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_TREESEARCH, TreeSearchClass))
#define IS_TREESEARCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TREESEARCH))
#define IS_TREESEARCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_TREESEARCH))
#define TREESEARCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_TREESEARCH, TreeSearchClass))




	

	typedef struct _TreeSearch       TreeSearch;
	typedef struct _TreeSearchClass  TreeSearchClass;

	struct _TreeSearch
	{
		GtkVBox 	parent;
		GtkWidget 	*entry;
		GtkWidget 	*but_down;
		GtkWidget 	*but_close;
		GtkTreeView 	*treeview;
		gint 		search_row;

	};

	struct _TreeSearchClass
	{
		GtkVBoxClass parent_class;

		void (* treesearch) (TreeSearch *ts);
	};

	GType          treesearch_get_type        (void);
	GtkWidget*     treesearch_new             (GtkTreeView *view, int search_row);
	void 		treesearch_start	(TreeSearch *ts);
    GtkWidget *treesearch_get_treeview(TreeSearch *ts);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TREESEARCH_H__ */
