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


