#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "mpdinteraction.h"
#include "playlist3.h"
#include "main.h"
#include "config1.h"
#include "debug_printf.h"
extern config_obj *config;
extern GtkListStore *pl2_store;
/* the internall data structure */
internal_data info;

/* this function doesnt use the start/stop_mpd_action because it the user doesnt want to see that */
int update_mpd_status()
{
	if(!mpd_ob_check_connected(connection)) return TRUE;
	mpd_ob_status_queue_update(connection);
/*	mpd_ob_status_update(connection);*/

	/* unlock it */
	return TRUE;
}

int disconnect_to_mpd()
{
	/* disconnect */
	mpd_ob_disconnect(connection);
	return FALSE;
}

void disconnect_callback(MpdObj *mi)
{
	gtk_timeout_remove(update_timeout);
	msg_set_base(_("gmpc - Disconnected"));
	if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
	{
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), _("Gnome Music Player Client"));	
	}

	scroll.exposed = 1;
	info.playlist_id = -1;
	info.playlist_length = -1;
	info.playlist_playtime = 0;
	info.old_pos = -1;

	/* disconnect playlist */
	pl3_disconnect();

	update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
	preferences_update();
	info.updating = FALSE;
	gtk_list_store_clear(pl2_store);
}

/* the functiont that connects to mpd */
int connect_to_mpd()
{
	char *string = NULL;
	scroll.exposed = 1;
	info.playlist_playtime = 0;
	string =cfg_get_single_value_as_string_with_default(config, "connection","hostname","localhost");
	mpd_ob_set_hostname(connection,string);
	cfg_free_string(string);
	mpd_ob_set_port(connection, cfg_get_single_value_as_int_with_default(config,"connection","portnumber", 6600));
	mpd_ob_set_connection_timeout(connection, cfg_get_single_value_as_float_with_default(config,"connection","timeout",1.0));

	if(cfg_get_single_value_as_int_with_default(config, "connection", "useauth",0))
	{
		string = cfg_get_single_value_as_string_with_default(config, "connection","password", "");
		mpd_ob_set_password(connection,string);
		cfg_free_string(string);
	}
	else
	{
		mpd_ob_set_password(connection,"");
	}

	if(mpd_ob_connect(connection) < 0)
	{
		debug_printf(DEBUG_INFO,"Connection failed\n");
		return TRUE;
	}


	update_mpd_status();
	mpd_ob_stats_update(connection);
	/* Set the title */
	msg_set_base(_("GMPC - Connected"));
	if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
	{
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), _("Gnome Music Player Client"));		
	}

	return FALSE;
}

/* DEFAULT FUNCTIONS */

gboolean check_connection_state()
{
	return !mpd_ob_check_connected(connection);
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
	mpd_ob_player_seek(connection, mpd_ob_status_get_elapsed_song_time(connection)+n);
	return FALSE;
}

int seek_ns(int n)
{
	return seek_ps(-n);
}

		
/* returns TRUE when an error */
int check_for_errors()
{
	return mpd_ob_check_error(connection);
}
