#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"

/* the internall data structure */
internal_data info;
/* = {NULL, NULL,NULL, TRUE,-1, -1, -1, -1, 1, NULL, NULL, FALSE, FALSE, "",0,NULL, NULL,NULL, NULL,NULL, NULL,NULL,"/", 0,0,0,FALSE, {1,FALSE,0, NULL,0,0,FALSE}};*/

/* this function doesnt use the start/stop_mpd_action because it the user doesnt want to see that */
int update_mpd_status()
    {
    /* check if locked, then just don't update */
    if(info.conlock) return TRUE;
    /* lock it. */
    info.conlock = TRUE;
    if(info.status != NULL)  mpd_freeStatus(info.status);
    info.status = mpd_getStatus(info.connection);
    /* check for errors */
 	if(check_for_errors())
		{
		return TRUE;
		}
    /* unlock it */
    info.conlock = FALSE;
    return TRUE;
    }

int disconnect_to_mpd()
    {
    if(info.conlock == TRUE) return TRUE;
    info.conlock = TRUE;
    mpd_closeConnection(info.connection);
    
    /* free the server stats */
    if(info.stats != NULL) mpd_freeStats(info.stats);
    info.stats = NULL;
    
    info.connection = NULL;
    msg_set_base("gmpc - Disconnected");
    gtk_timeout_remove(update_timeout);
    update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
    update_interface();
    scroll.exposed = 1;
    info.song = -1;
    gtk_widget_set_sensitive(glade_xml_get_widget(xml_main_window, "pm_button"), FALSE);
    if(info.playlist_running)
	{
	 destroy_playlist(glade_xml_get_widget(xml_playlist_window, "playlist_window"));
	if(debug)g_print("destroying playlist\n");
	 }
    clear_playlist_buffer();
    return FALSE;
    }    
/* the functiont that connects to mpd */
int connect_to_mpd()
    {
    scroll.exposed = 1;
    info.song = -1;    
    if(debug)g_print("timeout = %.2f\n", preferences.timeout);
    info.connection = mpd_newConnection(preferences.host, preferences.port, preferences.timeout);
    if(info.connection == NULL)
	{
	if(debug)g_print("Connection failed\n");
	 return TRUE;
	 }
  /*check for connection errors */
   if(info.connection->error)
	{
	msg_set_base("gmpc - Failed to connect, please check the connection settings.");
	mpd_closeConnection(info.connection);
	info.connection = NULL;	
	return TRUE;
	}
    if(preferences.user_auth == TRUE)
	{
	mpd_sendPasswordCommand(info.connection, preferences.password);
	mpd_finishCommand(info.connection);
	}
	
    info.stats = mpd_getStats(info.connection);
    if(info.stats == NULL)
	{
	GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, 
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						"You don't have enough permission to access mpd.");
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
    gtk_widget_set_sensitive(glade_xml_get_widget(xml_main_window, "pm_button"), TRUE);

    return FALSE;
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
	    mpd_sendPauseCommand(info.connection);
	    mpd_finishCommand(info.connection);
	    break;
	case MPD_STATUS_STATE_PAUSE:
	    mpd_sendPauseCommand(info.connection);
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

/* this function updates the internall dbase of mpd */
void update_mpd_dbase()
    {
    /* check if locked, then just don't update */
    if(info.conlock) return;
    /* lock it. */
    info.conlock = TRUE;
    mpd_sendUpdateCommand(info.connection);
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

