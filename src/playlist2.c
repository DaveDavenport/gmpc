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


void pl2_highlight_song()
{
	GtkTreeIter iter;
	gchar *temp;
	if(info.old_pos != -1)
	{
		temp = g_strdup_printf("%i",info.old_pos);
		if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(pl2_store), &iter, temp))
		{
			gint song_id =  0;
			gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_ID, &song_id, -1);
			if(song_id == info.status->songid && info.status->state  == info.state)
			{
				g_free(temp);
				return;
			}
			gtk_list_store_set(pl2_store, &iter, WEIGHT_INT, PANGO_WEIGHT_NORMAL, -1);
		}
		g_free(temp);
		info.old_pos = -1;
	}

	if(info.status->state != MPD_STATUS_STATE_STOP &&
			info.status->state != MPD_STATUS_STATE_UNKNOWN &&
			info.status->song != -1 &&
			info.status->playlistLength > 0)
	{
		temp = g_strdup_printf("%i", info.status->song);
		if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(pl2_store), &iter, temp))
		{
			gint pos;
			gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_POS, &pos, -1);
			if(pos != info.status->song)
			{
				g_print("Errror %i %i\n", pos, info.status->song);

			}
			gtk_list_store_set(pl2_store, &iter, WEIGHT_INT, PANGO_WEIGHT_ULTRABOLD, -1);
		}
		g_free(temp);                                                                     	
		info.old_pos = info.status->song;
	}
}

void update_playlist2()
{
	if(pl2_store == NULL) return;



	if(info.status->song != info.song || info.state != info.status->state)
	{
		pl2_highlight_song();
	}


	/* remove if above routine works */

	/* FIXME: see if there is a more optimized way todo this */
	/*	if(	(info.status->song != info.song && info.song != -1) || 
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
		*/




}


void set_compare_key(GtkEntry *entry)
{
	if(compare_key != NULL) g_free(compare_key);
	if(strlen(gtk_entry_get_text(entry)) ==0)
	{
		compare_key = NULL;
		return;
	}
	else
	{
		gchar *string = g_strdup_printf("*%s*", gtk_entry_get_text(entry));
		gchar *lower = g_utf8_strdown(string, -1);
		g_free(string);
		compare_key = g_pattern_spec_new(lower);
		g_free(lower);
	}
}

int pl2_filter_function(GtkTreeModel *model, GtkTreeIter *iter)
{
	gchar *string, *lower;
	if(compare_key == NULL) return TRUE;
	gtk_tree_model_get(model, iter, SONG_TITLE, &string, -1);
	if(string == NULL) return FALSE;
	lower = g_utf8_strdown(string,-1); 
	if(g_pattern_match_string(compare_key, lower))
	{
		g_free(lower);
		return TRUE;
	}
	else
	{
		g_free(lower);
		return FALSE;
	}
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
		g_print("pos_id %i - %s %s, pos\n", id,gtk_tree_model_get_string_from_iter(pl2_fil, &iter), gtk_tree_path_to_string(path));
		mpd_sendPlayIdCommand(info.connection, id);
		mpd_finishCommand(info.connection);
	}
}

/* show the id3info popup of the selected song
 * trigged on button click 
 */

void pl2_delete_selected_songs()
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(glade_xml_get_widget(pl2_xml, "pl_tree")));

	if(gtk_tree_selection_count_selected_rows(selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		mpd_sendCommandListBegin(info.connection);
		list = gtk_tree_selection_get_selected_rows (selection, &pl2_fil);
		llist = g_list_last(list);
		do{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter(pl2_fil, &iter,(GtkTreePath *)llist->data);
			gtk_tree_model_get(pl2_fil, &iter, SONG_ID, &value, -1);
			mpd_sendDeleteIdCommand(info.connection, value);
		}while((llist = g_list_previous(llist)));
		/* free list */
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
		mpd_sendCommandListEnd(info.connection);
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


gboolean pl2_row_moved(GtkTreeView *tree ,GdkDragContext *con, gint x, gint y, guint time)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition pos;
	GtkTreeIter iter, iter2;
	gint pos1, pos2;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
	/* get drop location */
	gtk_tree_view_get_dest_row_at_pos(tree, x,y, &path, &pos);
	if(path == NULL)
	{
		g_print("Don't know where to move it to\n");
		return TRUE;
	}
	gtk_tree_model_get_iter(GTK_TREE_MODEL(pl2_store), &iter,path);
	gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_POS, &pos2,-1);
	gtk_tree_path_free(path);

	if(pos == GTK_TREE_VIEW_DROP_AFTER && pos2 < info.status->playlistLength-1)
	{
		pos2 = pos2+1;
	}



	if(gtk_tree_selection_count_selected_rows(selection) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &pl2_fil);
		list = g_list_last(list);
		int i=0;
		mpd_sendCommandListBegin(info.connection);
		do{
			int dropl = pos2;
			gtk_tree_model_get_iter(pl2_fil, &iter2,(GtkTreePath *)list->data);

			/* get start pos */
			gtk_tree_model_get(pl2_fil, &iter2, SONG_POS, &pos1,-1);
			/* compensate for previous moves */
			/* if we move after the current */
			if(pos1 < pos2)
			{
				pos1 -= i;
				if(pos == GTK_TREE_VIEW_DROP_BEFORE)
					dropl-=1;

			}
			else if(pos1 > pos2)
			{
				pos1 += i;
				//				if(pos == GTK_TREE_VIEW_DROP_AFTER)
				//					dropl+=1;                   				
			}

			mpd_sendMoveCommand(info.connection, pos1,dropl);
			i++;
		}while((list = g_list_previous(list)));
		/* free list */
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		mpd_sendCommandListEnd(info.connection);
		mpd_finishCommand(info.connection);
	}
	g_signal_stop_emission_by_name(G_OBJECT(tree), "drag-drop");
	return TRUE;	
	//	return TRUE;
}


/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */

void create_playlist2()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTargetEntry target;
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


	/* Dragging*/ 
	target.target = "";
	target.flags = GTK_TARGET_SAME_WIDGET;
	target.info = 1;

	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(tree), GDK_BUTTON1_MASK,
			&target, 1, GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(tree), &target, 1, GDK_ACTION_MOVE);




	glade_xml_signal_autoconnect(pl2_xml);
}

