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
GPatternSpec *compare_key = NULL;
int hide_playlist2 (GtkWidget * but);

static GtkTargetEntry drag_types[] = {
  {"text/plain", 0, 100}
};

void load_playlist2 ();
void pl2_delete_selected_songs ();
/* timeout for the search */
guint filter_timeout = 0;
void pl2_filter_refilter ();

/* toggles the playlist on or off */
gboolean toggle_playlist2(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb))
	{
		create_playlist2();
	}
	else
	{
		if(pl2_xml == NULL) return FALSE;
		hide_playlist2(NULL);
	}
	return TRUE;
}

/* catch keybord pressing */
gboolean pl2_key_pressed (GtkWidget * widget, GdkEventKey * event)
{
	  if (event->keyval == GDK_Delete)
	  {
		pl2_delete_selected_songs ();  
	  }	
  /* propagate the event further */
  return FALSE;	
}


void pl2_shuffle_playlist()
{
		if(check_connection_state()) return;
		mpd_sendShuffleCommand(info.connection);
		mpd_finishCommand(info.connection);
}

void pl2_clear_playlist()
{
		if(check_connection_state()) return;
		mpd_sendClearCommand(info.connection);
		mpd_finishCommand(info.connection);
}

void pl2_crop_selected_songs()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW
				     (glade_xml_get_widget
				      (pl2_xml, "pl_tree")));
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl2_store), &iter))
	{
		mpd_sendCommandListBegin (info.connection);
		do{
			GtkTreeIter child;
			gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(pl2_fil), &child, &iter);
			if(!gtk_tree_selection_iter_is_selected(selection, &child))
				{
					gint id;
					gtk_tree_model_get(GTK_TREE_MODEL(pl2_store),
						&iter, SONG_ID, &id, -1);
					mpd_sendDeleteIdCommand(info.connection , id);
				}
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(pl2_store), &iter));
		mpd_sendCommandListEnd(info.connection);
		mpd_finishCommand(info.connection);
	}
	gtk_tree_selection_unselect_all(selection);
}


gboolean
pl2_button_press_event (GtkWidget * widget, GdkEventButton * event)
{
  if (event->button == 3)	/* right mouse button */
    {
      GladeXML *xml = glade_xml_new (GLADE_PATH "playlist.glade", "context_menu", NULL);
      GtkTreeSelection *selection =
	gtk_tree_view_get_selection (GTK_TREE_VIEW
				     (glade_xml_get_widget
				      (pl2_xml, "pl_tree")));
      if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{

	  /* menu with selected items */


	}
      else
	{
	  /* menu withouth selected items */

		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "remove"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "crop"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "show_information"), FALSE);
	}
	glade_xml_signal_autoconnect(xml);
	gtk_widget_show_all(GTK_WIDGET(glade_xml_get_widget(xml, "context_menu")));
	gtk_menu_popup(GTK_MENU(glade_xml_get_widget(xml, "context_menu")),NULL, NULL, NULL, NULL, event->button, event->time );

      /* stop signal */
      return TRUE;
    }

  /* continue signal */
  return FALSE;
}

/* function that gets called when the user clicks on a button in the song-info window */
void
pl2_set_query (const gchar * query)
{
  create_playlist2 ();
  gtk_entry_set_text (GTK_ENTRY
		      (glade_xml_get_widget (pl2_xml, "pl_searchen")), query);
}

/* track data recieved on the open_location button and propegate it to the open-location window */
void
pl2_drag_data_recieved (GtkWidget * window, GdkDragContext * context,
			gint x, gint y, GtkSelectionData * selection_data,
			guint info1, guint time)
{
  /* check if there is a connection */
  if (check_connection_state ())
    return;

  /* check if its the right type */
  if (info1 != 100)
    return;

  /* create (or if it exists do nothing  the open location window */
  ol_create (window);

  /* propagate event */
  ol_drag_data_recieved (window, context, x, y, selection_data, info1, time);
}

/* initialize playlist2, this is now a dependency and should always be called on startup */
void
init_playlist2 ()
{
  /* create initial tree store */
  pl2_store = gtk_list_store_new (NROWS, GTK_TYPE_INT,	/* song id */
				  GTK_TYPE_INT,	/* pos id */
				  GTK_TYPE_STRING,	/* song title */
				  GTK_TYPE_INT,	/* weight int */
				  G_TYPE_BOOLEAN,	/* weight color */
				  GTK_TYPE_STRING);	/* stock-id */
}


/* create a dialog that allows the user to save the current playlist */
void
pl2_save_playlist ()
{
  gchar *str;
  GladeXML *xml = NULL;

  /* check if the connection is up */
  if (check_connection_state ())
    return;

  /* create the interface */
  xml = glade_xml_new (GLADE_PATH "playlist.glade", "save_pl", NULL);

  /* run the interface */
  switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
    {
    case GTK_RESPONSE_OK:
      /* if the users agrees do the following: */
      /* get the song-name */
      str =
	(gchar *)
	gtk_entry_get_text (GTK_ENTRY
			    (glade_xml_get_widget (xml, "pl-entry")));
      /* check if the user entered a name, we can't do withouth */
      /* TODO: disable ok button when nothing is entered */
      /* also check if there is a connection */
      if (strlen (str) != 0 && !check_connection_state ())
	{
	  mpd_sendSaveCommand (info.connection, str);
	  mpd_finishCommand (info.connection);

	  /* if nothing went wrong we can reload the browser */
	  if (!check_for_errors ())
	    sb_reload_file_browser ();
	}
    }
  /* destroy the window */
  gtk_widget_destroy (glade_xml_get_widget (xml, "save_pl"));

  /* unref the gui description */
  g_object_unref (xml);
}

/* this function takes care the right row is highlighted */
void
pl2_highlight_song ()
{
  GtkTreeIter iter;
  gchar *temp;
  /* check if there is a connection */
  if (check_connection_state ())
    return;

  /* unmark the old pos if it exists */
  if (info.old_pos != -1)
    {
      /* create a string so I can get the right iter */
      temp = g_strdup_printf ("%i", info.old_pos);
      if (gtk_tree_model_get_iter_from_string
	  (GTK_TREE_MODEL (pl2_store), &iter, temp))
	{
	  gint song_id = 0;
	  /* check if we have the song we want */
	  gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_ID,
			      &song_id, -1);
	  /* if the old song is the new song (so tags updated) quit */
	  if (song_id == info.status->songid
	      && info.status->state == info.state)
	    {
	      g_free (temp);
	      return;
	    }
	  /* unhighlight the song */
	  gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,
			      PANGO_WEIGHT_NORMAL, -1);
	}
      g_free (temp);
      /* reset old pos */
      info.old_pos = -1;
    }
  /* check if we need to highlight a song */
  if (info.status->state != MPD_STATUS_STATE_STOP &&
      info.status->state != MPD_STATUS_STATE_UNKNOWN &&
      info.status->song != -1 && info.status->playlistLength > 0)
    {
      temp = g_strdup_printf ("%i", info.status->song);
      if (gtk_tree_model_get_iter_from_string
	  (GTK_TREE_MODEL (pl2_store), &iter, temp))
	{
	  gint pos;
	  gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_POS,
			      &pos, -1);
	  /* check if we have the right song, if not, print an error */
	  if (pos != info.status->song)
	    {
	      g_print ("Errror %i %i\n", pos, info.status->song);
	    }
	  gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,
			      PANGO_WEIGHT_ULTRABOLD, -1);
	}
      g_free (temp);
      /* set highlighted position */
      info.old_pos = info.status->song;
    }
}


/* function called on regular bases that allows the playlist to track changes */
void
update_playlist2 ()
{
  /* do nothing if we don't exist */
  if (pl2_store == NULL)
    return;

  /* if the song changed, or the state highlight the right song */
  if (info.status->song != info.song || info.state != info.status->state)
    {
      pl2_highlight_song ();
    }

}

/* this function triggers an refilter of the playlist by the timout function */
gboolean
pl2_auto_search ()
{
  /* so reset timeout id, because it will be removed if this function returns FALSE */
  filter_timeout = 0;
  /* refilter the tree */
  pl2_filter_refilter ();
  /* stop the timeout */
  return FALSE;
}

/* sets the search key from the entry, when changed */
void
set_compare_key (GtkEntry * entry)
{
  /* if there was another timeout counting off, reset that */
  if (filter_timeout != 0)
    {
      g_source_remove (filter_timeout);
      filter_timeout = 0;
    }

  /* 0.5 second after the user is _done_ typeing update the view */
  filter_timeout = g_timeout_add (500, (GSourceFunc) pl2_auto_search, NULL);

  /* if there allready is a compare key free its memory */
  if (compare_key != NULL)
    g_free (compare_key);

  /* check if the users entered a filter query, if not set compare key to NULL */
  if (strlen (gtk_entry_get_text (entry)) == 0)
    {
      compare_key = NULL;
      return;
    }
  else
    {
      /* convert the search query so that it doesnt find an exact match but *<user query>* */
      gchar *string = g_strdup_printf ("*%s*", gtk_entry_get_text (entry));
      /* make it case insensitive */
      gchar *lower = g_utf8_strdown (string, -1);
      /* free memory */
      g_free (string);
      /* create search key */
      compare_key = g_pattern_spec_new (lower);
      /* free lower case string */
      g_free (lower);
    }
}

/* function called by the tree_model that returns FALSE if a row shouldn't apear int he list */
int
pl2_filter_function (GtkTreeModel * model, GtkTreeIter * iter)
{
  gchar *string, *lower;
  /* if there is no key, every row is ok */
  if (compare_key == NULL)
    return TRUE;
  /* grab the row's name */
  gtk_tree_model_get (model, iter, SONG_TITLE, &string, -1);
  /* error check */
  if (string == NULL)
    return FALSE;
  /* lower the rows name */
  lower = g_utf8_strdown (string, -1);
  /* compare, and if matches the row is showed */
  if (g_pattern_match_string (compare_key, lower))
    {
      g_free (lower);
      return TRUE;
    }
  else
    {
      g_free (lower);
      return FALSE;
    }
}

/* trigger a refilter */
void
pl2_filter_refilter ()
{
  /* remove a auto-refilter timeout if running */
  if (filter_timeout != 0)
    {
      g_source_remove (filter_timeout);
      filter_timeout = 0;
    }
  /* tell the model to refilter */
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (pl2_fil));
}

/* hide the playlist 
 * this is called when the closed button is pressed (or keybinding)
 * or if the window recieves a delete event 
 */
int hide_playlist2 (GtkWidget * but)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2")), FALSE);
		return TRUE;
	}
  /* get the window */
  GtkWidget *win = glade_xml_get_widget (pl2_xml, "playlist_window");
  /* hide the window */
  gtk_widget_hide (win);
  /* stop the signal (delete event from window) */
  
  return TRUE;
}

/* if the user activate a row, grab the songid of that row and play it */
void
pl2_row_activated (GtkTreeView * tree, GtkTreePath * path)
{
  GtkTreeIter iter;
  /* check if there is a connection */
  if (check_connection_state ())
    return;
  /* get the iter from the path */
  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (pl2_fil), &iter, path))
    {
      gint id = 0;
      /* get the song id */
      gtk_tree_model_get (GTK_TREE_MODEL (pl2_fil), &iter, SONG_ID, &id, -1);
      /* send mpd the play command */
      mpd_sendPlayIdCommand (info.connection, id);
      mpd_finishCommand (info.connection);
      /* check for errors */
      check_for_errors ();
    }
}


/* delete all selected songs,
 * if no songs select ask the user if he want's to clear the list 
 */
void
pl2_delete_selected_songs ()
{
  /* grab the selection from the tree */
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW
				 (glade_xml_get_widget (pl2_xml, "pl_tree")));
  /* check if where connected */
  if (check_connection_state ())
    return;
  /* see if there is a row selected */
  if (gtk_tree_selection_count_selected_rows (selection) > 0)
    {
      GList *list = NULL, *llist = NULL;
      /* start a command list */
      mpd_sendCommandListBegin (info.connection);
      /* grab the selected songs */
      list = gtk_tree_selection_get_selected_rows (selection, &pl2_fil);
      /* grab the last song that is selected */
      llist = g_list_last (list);
      /* remove every selected song one by one */
      do
	{
	  GtkTreeIter iter;
	  int value;
	  gtk_tree_model_get_iter (pl2_fil, &iter,
				   (GtkTreePath *) llist->data);
	  gtk_tree_model_get (pl2_fil, &iter, SONG_ID, &value, -1);
	  mpd_sendDeleteIdCommand (info.connection, value);
	}
      while ((llist = g_list_previous (llist)));
      /* free list */
      g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
      g_list_free (list);
      /* close the list, so it will be executed */
      mpd_sendCommandListEnd (info.connection);
      mpd_finishCommand (info.connection);
      check_for_errors ();
    }
  else
    {
      /* create a warning message dialog */
      GtkWidget *dialog =
	gtk_message_dialog_new (GTK_WINDOW
				(glade_xml_get_widget
				 (pl2_xml, "playlist_window")),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_NONE,
				_
				("Are you sure you want to clear the playlist?"));
      gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
			      GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
			      GTK_RESPONSE_OK, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				       GTK_RESPONSE_CANCEL);

      switch (gtk_dialog_run (GTK_DIALOG (dialog)))
	{
	case GTK_RESPONSE_OK:
	  /* check if where still connected */
	  if (!check_connection_state ())
	    {
	      /* clear the playlist */
	      mpd_sendClearCommand (info.connection);
	      mpd_finishCommand (info.connection);
	      check_for_errors ();
	    }
	}
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
  /* update everything if where still connected */
    gtk_tree_selection_unselect_all(selection);
  if (!check_connection_state ())
    main_trigger_update ();
  
}



/* show the id3info popup of the selected song
 * trigged on button click 
 */

void
pl2_show_song_info ()
{
  int i = 0;
  GtkTreeModel *model = GTK_TREE_MODEL (pl2_fil);
  /* get the tree selection object */
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW
				 (glade_xml_get_widget (pl2_xml, "pl_tree")));
  /* check if there are selected rows */
  if ((i = gtk_tree_selection_count_selected_rows (selection)) > 0)
    {
      GList *list = NULL;
      list = gtk_tree_selection_get_selected_rows (selection, &model);
      /* iterate over every row */
      list = g_list_last (list);
      do
	{
	  GtkTreeIter iter;
	  int value;
	  gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
	  gtk_tree_model_get (model, &iter, SONG_POS, &value, -1);
	  /* show the info for this song  */
	  call_id3_window (value);
	  /* go to previous song if still connected */
	}
      while ((list = g_list_previous (list)) && !check_connection_state ());
      /* free list */
      g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
      g_list_free (list);
    }
}

/* signal triggered when the row is dropped back onto the playlist */
gboolean
pl2_row_moved (GtkTreeView * tree, GdkDragContext * con, gint x, gint y,
	       guint time)
{
  GtkTreePath *path = NULL;
  GtkTreeViewDropPosition pos;
  GtkTreeIter iter, iter2;
  gint pos1, pos2;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  /* if where disconnected just quit */
  if (check_connection_state ())
    return TRUE;



  /* get drop location */
  gtk_tree_view_get_dest_row_at_pos (tree, x, y, &path, &pos);
  if (path == NULL)
    {
      /* dropped to a place where I can't move to */
      g_print ("Don't know where to move it to\n");
      return TRUE;
    }
  /* grab drop localtion */
  gtk_tree_model_get_iter (GTK_TREE_MODEL (pl2_fil), &iter, path);
  gtk_tree_model_get (GTK_TREE_MODEL (pl2_fil), &iter, SONG_POS, &pos2, -1);
  gtk_tree_path_free (path);

  /* adjust position acording to drop after or for, also take last song in account */
  if (pos == GTK_TREE_VIEW_DROP_AFTER
      && pos2 < info.status->playlistLength - 1)
    {
      pos2 = pos2 + 1;
    }


  /* move every dragged row */
  if (gtk_tree_selection_count_selected_rows (selection) > 0)
    {
      GList *list = NULL;
      list = gtk_tree_selection_get_selected_rows (selection, &pl2_fil);
      list = g_list_last (list);
      int i = 0;
      /* start a command list */
      mpd_sendCommandListBegin (info.connection);
      do
	{
	  int dropl = pos2;
	  gtk_tree_model_get_iter (pl2_fil, &iter2,
				   (GtkTreePath *) list->data);

	  /* get start pos */
	  gtk_tree_model_get (pl2_fil, &iter2, SONG_POS, &pos1, -1);
	  /* compensate for previous moves */
	  /* if we move after the current */
	  if (pos1 < pos2)
	    {
	      pos1 -= i;
	      if (pos == GTK_TREE_VIEW_DROP_BEFORE ||
		  pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
		dropl -= 1;
	    }
	  else if (pos1 > pos2)
	    {
	      pos1 += i;
	    }

	  mpd_sendMoveCommand (info.connection, pos1, dropl);
	  i++;
	}
      while ((list = g_list_previous (list)));
      /* free list */
      g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
      g_list_free (list);

      mpd_sendCommandListEnd (info.connection);
      mpd_finishCommand (info.connection);
      check_for_errors ();
    }
  /* stop the signal */
  g_signal_stop_emission_by_name (G_OBJECT (tree), "drag-drop");
  /* trigger updates if connected */
  if (!check_connection_state ())
    main_trigger_update ();
  /* finish the drag */
  gtk_drag_finish (con, TRUE, FALSE, time);
  /* */
  return TRUE;
}


/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */

void
create_playlist2 ()
{
  GtkCellRenderer *renderer;
  GtkWidget *tree;
  GtkTargetEntry target;
  GtkTreeViewColumn *column = NULL;

  /* if we exists pop the window up */
  if (pl2_xml != NULL)
    {
      gtk_widget_show_all (glade_xml_get_widget (pl2_xml, "playlist_window"));
      gtk_window_present (GTK_WINDOW
			  (glade_xml_get_widget
			   (pl2_xml, "playlist_window")));
      return;
    }
  /* load gui desciption */
  pl2_xml =
    glade_xml_new (GLADE_PATH "playlist.glade", "playlist_window", NULL);

  /* obsolete, but cant hurt either */
  if (pl2_store == NULL)
    {
      /* song id, song title */
      pl2_store = gtk_list_store_new (NROWS, GTK_TYPE_INT,	/* song id */
				      GTK_TYPE_INT,	/* pos id */
				      GTK_TYPE_STRING,	/* song title */
				      GTK_TYPE_INT,	/* color string */
				      G_TYPE_BOOLEAN);	/* enble color */
    }

  tree = glade_xml_get_widget (pl2_xml, "pl_tree");

  /* set selection mode, so the user can select more then one row  */
  gtk_tree_selection_set_mode (GTK_TREE_SELECTION
			       (gtk_tree_view_get_selection
				(GTK_TREE_VIEW (tree))),
			       GTK_SELECTION_MULTIPLE);

  /* set filter */
  pl2_fil = gtk_tree_model_filter_new (GTK_TREE_MODEL (pl2_store), NULL);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (pl2_fil));
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 2);




  /* draw the column with the songs */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column,
				       renderer,
				       "stock-id", SONG_STOCK_ID, NULL);


  renderer = gtk_cell_renderer_text_new ();

  /* insert the column in the tree */
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column,
				       renderer,
				       "text", SONG_TITLE,
				       "weight", WEIGHT_INT,
				       "weight-set", WEIGHT_ENABLE, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  /* set filter function */
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (pl2_fil),
					  (GtkTreeModelFilterVisibleFunc)
					  pl2_filter_function, NULL, NULL);


  /* Dragging */
  target.target = "other";
  target.flags = GTK_TARGET_SAME_WIDGET;
  target.info = 2;

  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (tree),
					  GDK_BUTTON1_MASK, &target, 1,
					  GDK_ACTION_MOVE);
  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (tree), &target, 1,
					GDK_ACTION_MOVE);

  /* allow playlist (links) to be dragged onto the open-location buttn */
  gtk_drag_dest_set (glade_xml_get_widget (pl2_xml, "button7"),
		     GTK_DEST_DEFAULT_ALL, drag_types, 1, GDK_ACTION_COPY);

  g_signal_connect (G_OBJECT (glade_xml_get_widget (pl2_xml, "button7")),
		    "drag_data_received", G_CALLBACK (pl2_drag_data_recieved),
		    NULL);

  /* if where not connected call the disconnect function that will set some highlighting ok */
  if (check_connection_state ())
    {
      pl2_disconnect ();
    }

  /* connect signals that are defined in the gui description */
  glade_xml_signal_autoconnect (pl2_xml);
}


/* set some stuff if where connecting */
void
pl2_connect ()
{
  if (pl2_xml != NULL)
    {
      gtk_widget_set_sensitive (glade_xml_get_widget (pl2_xml, "hb_sens"),
				TRUE);
    }
}

/* set some stuff if where disconnecting */
void
pl2_disconnect ()
{
  /* remove all songs */
  gtk_list_store_clear (pl2_store);
  /* set buttons insensitive */
  gtk_widget_set_sensitive (glade_xml_get_widget (pl2_xml, "hb_sens"), FALSE);
  /* destroy a possible open location window */
  ol_destroy ();
  /* hide the add window */
  sb_close ();
  sb_disconnect ();
}
