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
#include "song-browser.h"
#include "open-location.h"

GladeXML *pl2_xml = NULL;
GtkListStore *pl2_store = NULL;
GtkTreeModel *pl2_fil = NULL;
GPatternSpec *compare_key= NULL;

static GtkTargetEntry drag_types[] = 
{
	{"text/plain", 0, 100}
};

void load_playlist2();

/* timeout for the search */
guint filter_timeout = 0;
void pl2_filter_refilter();


void pl2_drag_data_recieved(GtkWidget *window, GdkDragContext *context,
		gint x, gint y, GtkSelectionData *selection_data,
		guint info1, guint time)

{
	if(info1 != 100)
		return;
	ol_create(window);
	ol_drag_data_recieved(window, context,x, y, selection_data,info1, time);
}


void init_playlist2()
{
	pl2_store = gtk_list_store_new(NROWS, 
			GTK_TYPE_INT, /* song id */
			GTK_TYPE_INT, /* pos id */	 	
			GTK_TYPE_STRING, /* song title */
			GTK_TYPE_INT, /* weight int */
			G_TYPE_BOOLEAN, /* weight color */
			GTK_TYPE_STRING);/* stock-id */

}

void pl2_save_playlist()
{
	gchar *str;
	GladeXML *xml = NULL;
	if(check_connection_state()) return;
	xml = glade_xml_new(GLADE_PATH"playlist.glade", "save_pl",NULL);
	
	switch(gtk_dialog_run(GTK_DIALOG(glade_xml_get_widget(xml, "save_pl"))))
	{
		case GTK_RESPONSE_OK:
		str = (gchar *) gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "pl-entry")));
		if(strlen(str) != 0 && !check_connection_state())
		{
			mpd_sendSaveCommand(info.connection, str);
			mpd_finishCommand(info.connection);
			sb_reload_file_browser();
		}	
	}
	gtk_widget_destroy(glade_xml_get_widget(xml, "save_pl"));
	g_object_unref(xml);
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

}

gboolean pl2_auto_search()
{
	/* I am gonna remove it in a sec */
	filter_timeout = 0;
	pl2_filter_refilter();
	return FALSE;
}

void set_compare_key(GtkEntry *entry)
{
	/* 0.5 second after the user is _done_ typeing update the view */
	if(filter_timeout != 0)
	{
		g_source_remove(filter_timeout);
		filter_timeout = 0;
	}
	filter_timeout = g_timeout_add(500, (GSourceFunc)pl2_auto_search, NULL);
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
	if(filter_timeout != 0)
	{
		g_source_remove(filter_timeout);
		filter_timeout = 0;
	}                                       	
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(pl2_fil));
}


int hide_playlist2(GtkWidget *but)
{
	GtkWidget *win = glade_xml_get_widget(pl2_xml, "playlist_window");
	gtk_widget_hide(win);
	return TRUE;
}

/* if the user activate a row, grab the songid of that row and play it */
void pl2_row_activated(GtkTreeView *tree, GtkTreePath *path)
{
	GtkTreeIter iter;
	if(check_connection_state()) return;
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

void pl2_delete_selected_songs()
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(glade_xml_get_widget(pl2_xml, "pl_tree")));
	if(check_connection_state()) return;
	
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
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(
			GTK_WINDOW(glade_xml_get_widget(pl2_xml, "playlist_window")),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_NONE,
			"Are you sure you want to clear the playlist?");
		gtk_dialog_add_buttons(GTK_DIALOG(dialog), 
				GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
		
		switch(gtk_dialog_run(GTK_DIALOG(dialog)))
		{
			case GTK_RESPONSE_OK:
				mpd_sendClearCommand(info.connection);
				mpd_finishCommand(info.connection);
		}
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}
	/* update everything */
	main_trigger_update();
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


gboolean pl2_row_moved(GtkTreeView *tree ,GdkDragContext *con, gint x, gint y,guint time)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition pos;
	GtkTreeIter iter, iter2;
	gint pos1, pos2;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);

	if(check_connection_state()) return;
	/* get drop location */
	gtk_tree_view_get_dest_row_at_pos(tree, x,y, &path, &pos);

	if(path == NULL)
	{
		g_print("Don't know where to move it to\n");
		return TRUE;
	}
	gtk_tree_model_get_iter(GTK_TREE_MODEL(pl2_fil), &iter,path);
	gtk_tree_model_get(GTK_TREE_MODEL(pl2_fil), &iter, SONG_POS, &pos2,-1);
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
	/* trigger updates */
	main_trigger_update();

	gtk_drag_finish(con, TRUE, FALSE, time);
	/* */
	return TRUE;	
}


/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */

void create_playlist2()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTargetEntry target;
	GtkTreeViewColumn *column = NULL;
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
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, 
			renderer, 
			"stock-id", SONG_STOCK_ID,
			NULL);                      	

	
	renderer = gtk_cell_renderer_text_new();

	/* insert the column in the tree */
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, 
			renderer, 
			"text", SONG_TITLE,
			"weight", WEIGHT_INT,
			"weight-set", WEIGHT_ENABLE,
			NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	/* set filter function */
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(pl2_fil), 
			(GtkTreeModelFilterVisibleFunc) pl2_filter_function,
			NULL, NULL);


	/* Dragging*/ 
	target.target = "other";
	target.flags = GTK_TARGET_SAME_WIDGET;
	target.info = 2;

	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(tree), GDK_BUTTON1_MASK,
			&target, 1, GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(tree), &target, 1, GDK_ACTION_MOVE);

	gtk_drag_dest_set(glade_xml_get_widget(pl2_xml, "button7"), GTK_DEST_DEFAULT_ALL, drag_types, 1, GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (glade_xml_get_widget(pl2_xml, "button7")), "drag_data_received",
			G_CALLBACK (pl2_drag_data_recieved),
			NULL);                                                                          	
	if(check_connection_state())
	{
		pl2_disconnect();
	}

	glade_xml_signal_autoconnect(pl2_xml);
}



void pl2_connect()
{
	gtk_widget_set_sensitive(glade_xml_get_widget(pl2_xml, "hb_sens"), TRUE);	
}

void pl2_disconnect()
{
	/* remove all songs */
	gtk_list_store_clear(pl2_store);
	/* set buttons insensitive */
	gtk_widget_set_sensitive(glade_xml_get_widget(pl2_xml, "hb_sens"), FALSE);	
	/* destroy a possible open location window */
	ol_destroy();
	/* hide the add window */
	sb_close();
	sb_disconnect();
}

