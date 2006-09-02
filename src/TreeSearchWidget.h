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
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TREESEARCH_H__ */
