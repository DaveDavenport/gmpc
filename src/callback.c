#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"

/* returns TRUE when an error */

int check_for_errors()
{
	/* check for an error */
	if(info.connection->error)
	{
		if(debug)g_print("**DEBUG**  error: %s\n", info.connection->errorStr);
		/* check for connection errors */
		if(	info.connection->error == MPD_ERROR_TIMEOUT 		||
				info.connection->error == MPD_ERROR_CONNCLOSED 	||
				info.connection->error == MPD_ERROR_UNKHOST 		||
				info.connection->error == MPD_ERROR_CONNPORT 		||
				info.connection->error == MPD_ERROR_NOTMPD		||
				info.connection->error == MPD_ERROR_NORESPONSE	||
				info.connection->error == MPD_ERROR_SENDING)
		{
			msg_set_base("GMPC - Connection Error. Please check youre settings in the preferences menu");
			mpd_closeConnection(info.connection);
			info.conlock = TRUE;
			info.connection = NULL;
			/*Set some stuff right.  */
			scroll.exposed = 1;
			info.song = -1;
			gtk_widget_set_sensitive(glade_xml_get_widget(xml_main_window, "pm_button"), FALSE);
/*			if(info.playlist_running)
			{
				destroy_playlist(glade_xml_get_widget(xml_playlist_window, "playlist_window"));
				if(debug)g_print("destroying playlist\n");
			}
*/			info.state = -1;
//			clear_playlist_buffer();

			/* set update timeout slower.. we dont want it to update every 400 ms */
			gtk_timeout_remove(update_timeout);
			update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
		}
		else info.conlock = FALSE;

		/* clear all error's so it doesnt annoy me later */
		if(info.connection) mpd_clearError(info.connection);
		return TRUE;
	}
	else return FALSE;
}



