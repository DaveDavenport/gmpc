#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "mpdinteraction.h"
#include "playlist3.h"
#include "main.h"
#include "config1.h"
extern config_obj *config;
extern GtkListStore *pl2_store;
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

	mpd_ob_status_queue_update(connection);
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
	/* disconnect */
	mpd_ob_disconnect(connection);
	
	gtk_timeout_remove(update_timeout);

	/* free the server stats */
	if(info.stats != NULL) mpd_freeStats(info.stats);
	info.stats = NULL;

	info.connection = NULL;
	msg_set_base(_("gmpc - Disconnected"));

	scroll.exposed = 1;
//	info.song = -1;
	info.playlist_id = -1;
	info.playlist_length = -1;
	info.playlist_playtime = 0;
	info.old_pos = -1;
	
	/* disconnect playlist */
	pl3_disconnect();

	update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
	update_interface();
	info.updating = FALSE;
	gtk_list_store_clear(pl2_store);
	return FALSE;
}

/* the functiont that connects to mpd */
int connect_to_mpd()
{
	info.conlock = TRUE;
	scroll.exposed = 1;
//	info.song = -1;    
	info.playlist_playtime = 0;

	mpd_ob_set_hostname(connection,cfg_get_single_value_as_string_with_default(config, "connection","hostname","localhost"));
	mpd_ob_set_port(connection, cfg_get_single_value_as_int_with_default(config,"connection","portnumber", 6600));
//			cfg_get_single_value_as_float_with_default(config,"connection","timeout",1.0));

	if(cfg_get_single_value_as_int_with_default(config, "connection", "useauth",0))
	{
		mpd_ob_set_password(connection, cfg_get_single_value_as_string_with_default(config, "connection","password", ""));
	}
	else
	{
		mpd_ob_set_password(connection,"");
	}




	if(mpd_ob_connect(connection) < 0)
	{
		if(debug)g_print("Connection failed\n");
		return TRUE;
	}
	info.connection = connection->connection;


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
		cfg_set_single_value_as_int(config, "connection","autoconnect", 0);
		gtk_widget_show_all(dialog);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return TRUE;	
	}
	info.conlock = FALSE;
	update_mpd_status();

	/* Set the title */
	msg_set_base(_("GMPC - Connected"));


	return FALSE;
}

/* DEFAULT FUNCTIONS */
/* returns FALSE when connected */

gboolean check_connection_state()
{
	if(info.connection == NULL)
	{
		return TRUE;
	}
	else return FALSE;
}

gboolean mpd_is_locked()
{
	return info.conlock;
}

gboolean mpd_lock()
{
	if(mpd_is_locked())
	{
		/* database is allready locked */
		return TRUE;
	}
	/* lock */
	info.conlock = TRUE;
	return FALSE;
}

gboolean mpd_unlock()
{
	if(!mpd_is_locked())
	{
		/* database is allready unlocked */
		/* we don't make an error from this..  maybe we should ? */
		return FALSE;
	}

	/* unlock */
	info.conlock = FALSE;
	return FALSE;
}


/******************************************************
 * PLAYER FUNCTIONS 
 */


/* the normal play functions, stop, play, next, prev */
/* returns FALSE when everything went ok */
int next_song()
{
	mpd_ob_player_next(connection);
	return FALSE;
}

int prev_song()
{
	mpd_ob_player_prev(connection);
	return FALSE;
}

int stop_song()
{
	mpd_ob_player_stop(connection);
	return FALSE;
}

int play_song()
{
	int state = mpd_ob_player_get_state(connection);
	if(state == MPD_OB_PLAYER_STOP)
	{
		mpd_ob_player_play(connection);
	}
	else if (state == MPD_OB_PLAYER_PAUSE || state == MPD_OB_PLAYER_PLAY)
	{
		mpd_ob_player_pause(connection);
	}
	return FALSE;	
}

void random_pl(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb) != mpd_ob_player_get_random(connection))
	mpd_ob_player_set_random(connection, !mpd_ob_player_get_random(connection));
}

void repeat_pl(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb) != mpd_ob_player_get_repeat(connection))
		mpd_ob_player_set_repeat(connection, !mpd_ob_player_get_repeat(connection));
}

/* TODO: Changed return Values, check for possible errors */
int seek_ps(int n)
{
	if(mpd_lock())
	{
		return TRUE;
	}

	if(info.status->state == MPD_STATUS_STATE_PLAY || info.status->state == MPD_STATUS_STATE_PAUSE)
	{
		mpd_sendSeekCommand(info.connection, info.status->song, info.status->elapsedTime+n);
		mpd_finishCommand(info.connection);
		if(check_for_errors())
		{
			return TRUE;
		}
	}
	if(mpd_unlock())
	{
		return TRUE;
	}
	return FALSE;
}

int seek_ns(int n)
{
	return seek_ps(-n);
}

int volume_change(int diff)
{
	if(mpd_lock())
	{
		return TRUE;
	}
	mpd_sendVolumeCommand(info.connection,diff);
	mpd_finishCommand(info.connection);
	if(check_for_errors())
	{
		return TRUE;
	}
	if(mpd_unlock())
	{
		return TRUE;
	}
	return FALSE;
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
