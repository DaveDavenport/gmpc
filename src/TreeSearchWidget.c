#include <gtk/gtk.h>
#include <gtk/gtksignal.h>
#include <gdk/gdkkeysyms.h>
#include <regex.h>
#include "TreeSearchWidget.h"
static void treesearch_class_init          (TreeSearchClass *klass);
static void treesearch_init                (TreeSearch      *ts);
static void treesearch_close(GtkButton *button, TreeSearch *ts);
static void treesearch_entry_changed(GtkEntry *entry, TreeSearch *ts);
enum {
	RESULT_ACTIVATE,
	LAST_SIGNAL
};



static gint treesearch_signals[LAST_SIGNAL] = { 0 };


GType treesearch_get_type (void)
{
	static GType ts_type = 0;

	if (!ts_type)
	{
		static const GTypeInfo ts_info =
		{
			
			sizeof (TreeSearchClass),
			NULL,
			NULL,
			(GClassInitFunc) treesearch_class_init,
			NULL,
			NULL,
			sizeof (TreeSearch),
			0,
			(GInstanceInitFunc) treesearch_init,
		};

		ts_type = g_type_register_static(GTK_TYPE_VBOX, "TreeSearch", &ts_info, 0);
	}

	return ts_type;
}


static void treesearch_class_init (TreeSearchClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*) class;


	treesearch_signals[RESULT_ACTIVATE] = g_signal_new ("result-activate",
			G_TYPE_FROM_CLASS(class),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0,0);
			

	class->treesearch = NULL;
}
void treesearch_close(GtkButton *button, TreeSearch *ts)
{
	gtk_widget_hide(GTK_WIDGET(ts));
	gtk_widget_grab_focus(GTK_WIDGET(ts->treeview));
}

void treesearch_start(TreeSearch *ts)
{
	gtk_widget_show(GTK_WIDGET(ts));
	gtk_widget_grab_focus(ts->entry);
	treesearch_entry_changed(ts->entry, ts);
}

static int treesearch_search_from_iter_forward(TreeSearch *ts,GtkTreeIter *iter) {
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(ts->treeview));
	const char *text = gtk_entry_get_text(GTK_ENTRY(ts->entry));
	regex_t regt;
	if(regcomp(&regt, text, REG_EXTENDED|REG_ICASE|REG_NOSUB)) {
		return FALSE;
	}

	do{
		char *title;
		gtk_tree_model_get(model, iter,ts->search_row, &title, -1); 
		if(!regexec(&regt, title, 0,NULL,0))
		{
			g_free(title);
			return TRUE;
		}
		g_free(title);
	} while(gtk_tree_model_iter_next(model,  iter));
	return FALSE;
}
static void treesearch_search_next(GtkButton *but,TreeSearch *ts){
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(ts->treeview));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ts->treeview));
	gtk_widget_set_sensitive(ts->but_down, FALSE);
	if (gtk_tree_selection_count_selected_rows (selection) == 1)
	{
		GList *list = NULL;
		GtkTreeIter iter;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		if(gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) list->data))
		{
			if(gtk_tree_model_iter_next(model, &iter))
				if(treesearch_search_from_iter_forward(ts, &iter))
				{
					GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
					gtk_tree_selection_unselect_all(selection);
					gtk_tree_selection_select_iter(selection, &iter);
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ts->treeview), path,NULL,TRUE,0.5,0.5);
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(ts->treeview), path, NULL,0);
					gtk_tree_path_free(path);
					if(gtk_tree_model_iter_next(model, &iter))
					{
						if(treesearch_search_from_iter_forward(ts,&iter)) {
							gtk_widget_set_sensitive(ts->but_down, TRUE);
						}
					}
				}
				else{
					GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
					gtk_tree_selection_unselect_all(selection);                                       					
					gtk_tree_selection_select_iter(selection, &iter);
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ts->treeview), path,NULL,TRUE,0.5,0.5);
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(ts->treeview), path, NULL,0);
					gtk_tree_path_free(path);
				}
		}
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}
static void treesearch_entry_changed(GtkEntry *entry, TreeSearch *ts)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(ts->treeview));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ts->treeview));
	const gchar *text = gtk_entry_get_text(entry);


	gtk_widget_set_sensitive(ts->but_down, FALSE);

	if(g_utf8_strlen(text,-1) == 0)
	{
		gtk_tree_selection_unselect_all(selection);       
		return;
	}
	if (gtk_tree_selection_count_selected_rows (selection) == 1)
	{

		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) list->data);
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
	else{
		if(!gtk_tree_model_get_iter_first(model, &iter))
		{
			return;
		}
	}                                                                                               	
	if(treesearch_search_from_iter_forward(ts,&iter))
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_selection_unselect_all(selection);
		gtk_tree_selection_select_iter(selection, &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ts->treeview), path,NULL,TRUE,0.5,0.5);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(ts->treeview), path, NULL,0);
		gtk_tree_path_free(path);

		if(gtk_tree_model_iter_next(model, &iter))
		{
			if(treesearch_search_from_iter_forward(ts,&iter)) {
				gtk_widget_set_sensitive(ts->but_down, TRUE);
			}
		}

	}
	else{
		gtk_tree_selection_unselect_all(selection);       
	}
}
static void treesearch_entry_activate(GtkEntry *entry, TreeSearch *ts)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ts->treeview));
	if (gtk_tree_selection_count_selected_rows (selection) == 1)
	{
		g_signal_emit(ts, treesearch_signals[RESULT_ACTIVATE],0,0);
	}
	treesearch_close(NULL,ts);
}
static int treesearch_entry_key_press(GtkWidget *entry, GdkEventKey *event, TreeSearch *ts)
{
	if(event->keyval == GDK_Down)
	{
		treesearch_search_next(NULL,ts);
		return TRUE;
	}	
	if(event->keyval == GDK_Escape)
	{
		treesearch_close(NULL,ts);
		return TRUE;
	}
	if((event->keyval == GDK_KP_Enter || event->keyval == GDK_Return)&& event->state&GDK_CONTROL_MASK)
	{
		treesearch_entry_activate(NULL,ts);
		return TRUE;
	}
	if((event->keyval == GDK_KP_Enter || event->keyval == GDK_Return))
	{
		treesearch_close(NULL,ts);
		return TRUE;
	}

	return FALSE;
}

static void treesearch_init (TreeSearch *ts)
{
	GtkWidget *label;
	GtkWidget *vbox = NULL;
	vbox = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(ts), vbox);
	label = gtk_label_new("Find:");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE,0);

	ts->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), ts->entry, FALSE, TRUE,0);
	g_signal_connect(G_OBJECT(ts->entry), "changed", G_CALLBACK(treesearch_entry_changed), ts);
//	g_signal_connect(G_OBJECT(ts->entry), "activate", G_CALLBACK(treesearch_entry_activate), ts);
	g_signal_connect(G_OBJECT(ts->entry), "key-press-event", G_CALLBACK(treesearch_entry_key_press), ts);

	ts->but_down = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_box_pack_start(GTK_BOX(vbox), ts->but_down, FALSE, TRUE,0);
	gtk_widget_set_sensitive(ts->but_down, FALSE);
	g_signal_connect(G_OBJECT(ts->but_down), "clicked", G_CALLBACK(treesearch_search_next), ts);
	ts->but_close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_end(GTK_BOX(vbox), ts->but_close, FALSE, TRUE,0);

	g_signal_connect(G_OBJECT(ts->but_close), "clicked", G_CALLBACK(treesearch_close), ts);


	gtk_widget_show_all(vbox);
}
static void treesearch_set_treeview(TreeSearch *ts, GtkTreeView *view)
{
	ts->treeview =view;
}
static void treesearch_set_search_row(TreeSearch *ts,int search_row)
{
	ts->search_row = search_row;
}
GtkWidget* treesearch_new (GtkTreeView *view, int search_row)
{
	GtkWidget *widget = NULL;
	if(!view) {
		g_warning("view != NULL failed\n");
		return NULL;
	}
	if(search_row < 0){
		g_warning("search_row >= 0 failed\n");
		return NULL;
	}
	widget = GTK_WIDGET ( gtk_type_new (treesearch_get_type ()));
	treesearch_set_treeview(TREESEARCH(widget), view);
	treesearch_set_search_row(TREESEARCH(widget),search_row);
	return widget;
}

void treesearch_clear (TreeSearch *ts)
{
}


