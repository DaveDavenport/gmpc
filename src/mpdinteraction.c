#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "playlist2.h"
#include "main.h"

/* the internall data structure */
internal_data info;

/* this function doesnt use the start/stop_mpd_action because it the user doesnt want to see that */
int update_mpd_status()
{
	/* check if locked, then just don't update */
	if(info.conlock) return TRUE;
	/* lock it. */
	info.conlock = TRUE;
	if(info.status != NULL)  mpd_freeStatus(info.status);
	mpd_sendStatusCommand(info.connection);
	info.status = mpd_getStatus(info.connection);
	mpd_finishCommand(info.connection);
	/* check for errors */
	if(check_for_errors())
	{
		return TRUE;
	}
	if(info.stats != NULL) mpd_freeStats(info.stats);
	mpd_sendStatsCommand(info.connection);
	info.stats = mpd_getStats(info.connection);
	mpd_finishCommand(info.connection);
	if(info.stats == NULL)g_print("crap %s\n", info.connection->errorStr);


	/* unlock it */
	info.conlock = FALSE;
	return TRUE;
}

int disconnect_to_mpd()
{
	if(info.conlock == TRUE)
	{
		return TRUE;
	}
	info.conlock = TRUE;
	mpd_closeConnection(info.connection);
	gtk_timeout_remove(update_timeout);

	/* free the server stats */
	if(info.stats != NULL) mpd_freeStats(info.stats);
	info.stats = NULL;

	info.connection = NULL;
	msg_set_base(_("gmpc - Disconnected"));

	scroll.exposed = 1;
	info.song = -1;
	info.playlist_id = -1;
	info.playlist_length = -1;
	info.old_pos = -1;
	
	/* disconnect playlist */
	pl2_disconnect();

	update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
	update_interface();
	info.updating = FALSE;

	return FALSE;
}

/* the functiont that connects to mpd */
int connect_to_mpd()
{
	info.conlock = TRUE;
	scroll.exposed = 1;
	info.song = -1;    
	if(debug)g_print("timeout = %.2f\n", preferences.timeout);
	if(info.connection) mpd_clearError(info.connection);
	info.connection = mpd_newConnection(preferences.host, preferences.port, preferences.timeout);
	if(info.connection == NULL)
	{
		if(debug)g_print("Connection failed\n");
		return TRUE;
	}
	/*check for connection errors */
	if(info.connection->error)
	{
		g_print("Connection failed\n");
		msg_set_base(_("gmpc - Failed to connect, please check the connection settings."));
		mpd_closeConnection(info.connection);
		info.connection = NULL;	
		return TRUE;
	}
	if(preferences.user_auth == TRUE)
	{
		mpd_sendPasswordCommand(info.connection, preferences.password);
		mpd_finishCommand(info.connection);
	}

	mpd_sendStatsCommand(info.connection);
	info.stats = mpd_getStats(info.connection);
	mpd_finishCommand(info.connection);
	if(info.stats == NULL)
	{
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, 
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("You don't have enough permission to access mpd."));
		mpd_closeConnection(info.connection);
		info.connection = NULL;
		preferences.autoconnect = FALSE;
		gtk_widget_show_all(dialog);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return TRUE;	
	}
	info.conlock = FALSE;
	update_mpd_status();

	/* Set the title */
	msg_set_base(_("GMPC - Connected"));

	/* connect playlist2 */
	pl2_connect();


	
	return FALSE;
}

/* returns FALSE when connected */

gboolean check_connection_state()
{
	if(info.connection == NULL)
	{
		return TRUE;
	}
	else return FALSE;
}



/* the normal play functions, stop, play, next, prev */

void next_song()
{
	/* check lock, no need to lock it for this command */
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendNextCommand(info.connection);
	mpd_finishCommand(info.connection);
	/* check for an error */
	if(check_for_errors()) return;
	info.conlock = FALSE;
}

void prev_song()
{
	/* check lock, no need to lock it for this command */
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendPrevCommand(info.connection);
	mpd_finishCommand(info.connection);
	/* check for an error */
	if(check_for_errors()) return;
	info.conlock = FALSE;
}

void stop_song()
{
	/* check lock, no need to lock it for this command */
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendStopCommand(info.connection);
	mpd_finishCommand(info.connection);
	/* check for an error */
	if(check_for_errors()) return;
	info.conlock = FALSE;

}

void play_song()
{
	/* check lock, no need to lock it for this command */
	if(info.conlock) return;
	info.conlock = TRUE;
	switch(info.status->state)
	{
		case MPD_STATUS_STATE_PLAY:
			mpd_sendPauseCommand(info.connection, TRUE);
			mpd_finishCommand(info.connection);
			break;
		case MPD_STATUS_STATE_PAUSE:
			mpd_sendPauseCommand(info.connection,FALSE);
			mpd_finishCommand(info.connection);
			break;
		default:
			mpd_sendPlayCommand(info.connection, -1);
			mpd_finishCommand(info.connection);

	}
	/* check for an error */
	if(check_for_errors()) return;
	info.conlock = FALSE;

}

void random_pl()
{
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendRandomCommand(info.connection, !info.status->random);
	mpd_finishCommand(info.connection);
	info.status->random =  !info.status->random;
	if(check_for_errors()) return;
	info.conlock = FALSE;
}

void repeat_pl()
{
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendRepeatCommand(info.connection, !info.status->repeat);
	mpd_finishCommand(info.connection);
	info.status->repeat =  !info.status->repeat;
	if(check_for_errors()) return;
	info.conlock = FALSE;
}

int seek_ps(int n)
{
	if(info.conlock) return FALSE;

	if(info.status->state == MPD_STATUS_STATE_PLAY || info.status->state == MPD_STATUS_STATE_PAUSE)
	{
		info.conlock = TRUE;
		mpd_sendSeekCommand(info.connection, info.status->song, 
				info.status->elapsedTime+n);
		mpd_finishCommand(info.connection);
		if(check_for_errors()) return FALSE;
		info.conlock = FALSE;
	}
	return FALSE;
}

int seek_ns(int n)
{
	if(info.conlock) return FALSE;

	if(info.status->state == MPD_STATUS_STATE_PLAY || info.status->state == MPD_STATUS_STATE_PAUSE)
	{
		info.conlock = TRUE;
		mpd_sendSeekCommand(info.connection, info.status->song,
				info.status->elapsedTime-n);
		mpd_finishCommand(info.connection);
		if(check_for_errors()) return FALSE;
		info.conlock = FALSE;

	}                                                                                              		
	return FALSE;
}

void volume_change(int diff)
{
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendVolumeCommand(info.connection,diff);
	mpd_finishCommand(info.connection);
	if(check_for_errors()) return;
	info.conlock = FALSE;
}



/* this function updates the internall dbase of mpd */
void update_mpd_dbase()
{
	/* check if locked, then just don't update */
	if(info.conlock) return;
	/* lock it. */
	info.conlock = TRUE;
	mpd_sendUpdateCommand(info.connection, "");
	mpd_finishCommand(info.connection);
	/* check for errors */
	if(check_for_errors())
	{
		return;
	}
	/* unlock it */
	info.conlock = FALSE;
	return;
}
