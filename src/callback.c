#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"

/* return 0 = good */
/* return 1 = bad */
int start_mpd_action()
    {
    /* check lock, just to be sure */
    if(info.conlock) return 1;
    /* ok its not locked.. let lock it now */
    info.conlock = TRUE;    
    /* do some stuff so the user can't do (or expect to) do wild stuff */
   gtk_widget_set_sensitive(glade_xml_get_widget(xml_main_window, "main_window"), FALSE);
    /* everything is ok.. */
    return 0;
    }

int stop_mpd_action()
    {
    /* check lock, just to be sure */
    if(!info.conlock) return 1;
    /* ok its not locked.. let lock it now */
    info.conlock = FALSE;    
    /* undo the stuff */
      gtk_widget_set_sensitive(glade_xml_get_widget(xml_main_window, "main_window"), TRUE);
    /* everything is ok.. */
    return 0;
    }

int load_playlist()
    {
    mpd_InfoEntity *entity;
    int i =1;
    info.total_number_of_songs = 0;
    info.total_playtime = 0;

    if(info.conlock) return TRUE;
    if(debug)g_print("start updating playlist\n");
    if(info.playlist != NULL)
    {
	    while((info.playlist = g_list_next(info.playlist)))
	    {
		    mpd_freeSong(info.playlist->data);
		    info.playlist->data = NULL;	    
	    }
	    g_list_free(info.playlist);
	    info.playlist = NULL;
    }
    mpd_sendPlaylistInfoCommand(info.connection, -1);
    while((entity = mpd_getNextInfoEntity(info.connection)))
    {
	    /*	if(!(i % rate))
		{
		gchar *buf = g_strdup_printf("Updating %3.1f%% done", ((i*100)/(float)info.status->playlistLength));
		msg_push_popup(buf);
		g_free(buf);
		while (gtk_events_pending ()) gtk_main_iteration();
		}
		*/
	    if(check_for_errors())
	    {
		    /* remove everything so far */
		    while((info.playlist = g_list_next(info.playlist)))
		    {
			    mpd_freeSong(info.playlist->data);
			    info.playlist->data = NULL;	    
		    }
		    g_list_free(info.playlist);
		    info.playlist = NULL;

		    stop_mpd_action();
		    /* this needs to be true, we did stop on error. */
		    info.conlock = TRUE;
		    return TRUE;
	    }
	    if(entity->type == MPD_INFO_ENTITY_TYPE_SONG)
	    {
		    info.playlist = g_list_append(info.playlist, mpd_songDup(entity->info.song));
		    info.total_playtime += entity->info.song->time;
		    info.total_number_of_songs++;
	    }
	    i++;
	    mpd_freeInfoEntity(entity);
    }
    /* force update */
    info.status->song = -1;
    if(debug)g_print("done updating playlist %lu\n", (long unsigned) info.total_playtime);
    msg_pop_popup();
    return FALSE;
    }

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
			if(info.playlist_running)
			{
				destroy_playlist(glade_xml_get_widget(xml_playlist_window, "playlist_window"));
				if(debug)g_print("destroying playlist\n");
			}
			info.state = -1;
			clear_playlist_buffer();

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



