#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <glade/glade.h>
#include <time.h>
#include "libmpdclient.h"
#include "strfsong.h"
#include "main.h"
#include "misc.h"
#include "playlist2.h"

GladeXML *pl2_xml = NULL;
GtkListStore *pl2_store = NULL;
GtkTreeModel *pl2_fil = NULL;
GPatternSpec *compare_key= NULL;

void load_playlist2();

void init_playlist2()
{
	g_print("creating listore\n");
	pl2_store = gtk_list_store_new(NROWS, 
			GTK_TYPE_INT, /* song id */
			GTK_TYPE_INT, /* pos id */	 	
			GTK_TYPE_STRING, /* song title */
			GTK_TYPE_INT, /* weight int */
			G_TYPE_BOOLEAN); /* weight color */

}

void update_playlist2()
{
	if(pl2_store == NULL) return;
	
	/* FIXME: see if there is a more optimized way todo this */
	if(	(info.status->song != info.song && info.song != -1) || 
			(info.state != info.status->state &&  
			 info.status->state != MPD_STATUS_STATE_PAUSE && 
			 info.state != MPD_STATUS_STATE_PAUSE))
	{
		GtkTreeIter iter;
		GtkTreeModel *model = GTK_TREE_MODEL(pl2_store);
		int i = 0;
		if(gtk_tree_model_get_iter_first(model, &iter))
			do
			{
				if(info.status->state != MPD_STATUS_STATE_STOP 
						&& info.status->state != MPD_STATUS_STATE_UNKNOWN)
				{
					gtk_tree_model_get(model, &iter, SONG_ID, &i, -1);
					if(i == info.status->songid)
					{
						gtk_list_store_set(pl2_store, &iter,
								WEIGHT_INT,PANGO_WEIGHT_ULTRABOLD, -1);
					}
					else  gtk_list_store_set(pl2_store, &iter,
						WEIGHT_INT,PANGO_WEIGHT_NORMAL, -1);
				}
				else  gtk_list_store_set(pl2_store, &iter, 
						WEIGHT_INT,PANGO_WEIGHT_NORMAL, -1);
			}
			while (gtk_tree_model_iter_next(model, &iter));
	}
}


void set_compare_key(GtkEntry *entry)
{
	gchar *string = g_strdup_printf("*%s*", gtk_entry_get_text(entry));
	if(compare_key != NULL) g_free(compare_key);
	compare_key = g_pattern_spec_new(string);
	g_free(string);
}

int pl2_filter_function(GtkTreeModel *model, GtkTreeIter *iter)
{
	gchar *string;
	if(compare_key == NULL) return TRUE;
	gtk_tree_model_get(model, iter, SONG_TITLE, &string, -1);
	if(string == NULL) return FALSE;
	return g_pattern_match_string(compare_key, string);
}

void pl2_filter_refilter()
{
	g_print("starting filtering\n");
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(pl2_fil));
}


int hide_playlist2(GtkWidget *but)
{
	GtkWidget *win = glade_xml_get_widget(pl2_xml, "playlist_window");
	g_print("hiding playlist2\n");
	gtk_widget_hide(win);
	return TRUE;
}

/* if the user activate a row, grab the songid of that row and play it */
void pl2_row_activated(GtkTreeView *tree, GtkTreePath *path)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter(GTK_TREE_MODEL(pl2_fil), &iter, path))
	{
		gint id=0;
		gtk_tree_model_get(GTK_TREE_MODEL(pl2_fil), &iter, SONG_ID,&id,-1);
		mpd_sendPlayIdCommand(info.connection, id);
		mpd_finishCommand(info.connection);
	}
}

/* show the id3info popup of the selected song
 * trigged on button click 
 */

void pl2_show_song_info()
{
	int i = 0;
	GtkTreeModel *model = GTK_TREE_MODEL(pl2_fil);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(glade_xml_get_widget(pl2_xml, "pl_tree")));
	if((i = gtk_tree_selection_count_selected_rows(selection)) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		list = g_list_last(list);
		do{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter(model, &iter,(GtkTreePath *)list->data);
			gtk_tree_model_get(model, &iter, SONG_POS, &value, -1);
			call_id3_window(value);
		}while((list = g_list_previous(list)));
		/* free list */
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}


/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */

void create_playlist2()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	if(pl2_xml != NULL)
	{
		gtk_widget_show_all(
				glade_xml_get_widget(pl2_xml, "playlist_window"));
		gtk_window_present(GTK_WINDOW(
					glade_xml_get_widget(pl2_xml, "playlist_window")));
		return;
	}
	pl2_xml = glade_xml_new(GLADE_PATH"playlist.glade", "playlist_window",NULL);

	if(pl2_store == NULL)
	{
		/* song id, song title */
		pl2_store = gtk_list_store_new(NROWS, 
				GTK_TYPE_INT, /* song id */
				GTK_TYPE_INT, /* pos id */	
				GTK_TYPE_STRING, /* song title */
				GTK_TYPE_INT, /* color string */
				G_TYPE_BOOLEAN); /* enble color */
	}
	tree = glade_xml_get_widget(pl2_xml, "pl_tree");
	/* set selection mode */
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree))), 
			GTK_SELECTION_MULTIPLE);

	/* set filter */
	pl2_fil = gtk_tree_model_filter_new(GTK_TREE_MODEL(pl2_store), NULL);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),
			GTK_TREE_MODEL(pl2_fil));

	/* draw the column with the songs */
	renderer = gtk_cell_renderer_text_new();
	
	/* insert the column in the tree */
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), 
			-1,"Playlist", renderer, 
			"text", SONG_TITLE,
			"weight", WEIGHT_INT,
			"weight-set", WEIGHT_ENABLE,
			NULL);


	/* set filter function */
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(pl2_fil), 
			(GtkTreeModelFilterVisibleFunc) pl2_filter_function,
			NULL, NULL);

	glade_xml_signal_autoconnect(pl2_xml);
}

