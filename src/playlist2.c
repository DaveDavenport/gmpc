/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


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
#include "open-location.h"

GladeXML *pl2_xml = NULL;
GtkListStore *pl2_store = NULL;
GtkTreeModel *pl2_fil = NULL;
GPatternSpec *compare_key = NULL;


/* size */
GtkAllocation pl2_wsize = { 0,0,0,0};





void load_playlist2 ();
void pl2_delete_selected_songs ();
/* timeout for the search */
guint filter_timeout = 0;
void pl2_filter_refilter ();




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
			GTK_TYPE_STRING,	/* stock-id */
			GTK_TYPE_INT);
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
//				if (!check_for_errors ())
//					sb_reload_file_browser ();
			}
	}
	/* destroy the window */
	gtk_widget_destroy (glade_xml_get_widget (xml, "save_pl"));

	/* unref the gui description */
	g_object_unref (xml);
}

/* this function takes care the right row is highlighted */
void pl2_highlight_song ()
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



