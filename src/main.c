#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"

GladeXML *xml_main_window = NULL;

int debug = FALSE;
int update_interface();
guint update_timeout = 0;

void set_default_values()
    {
    memset(preferences.host, '\0', 256);
    strcpy(preferences.host, "localhost");
    preferences.port = 2100;
    preferences.user_auth = FALSE;;
    memset(preferences.password, '\0', 256);
    strcpy(preferences.password, "");
    preferences.timeout = 1.0;
    preferences.autoconnect = FALSE;
    
    /* the mpd status struct used  in the whole program */
    info.status = NULL;
	/* the mpd connection */
    info.connection = NULL;
	/* status about the connection */
    info.stats = NULL;
	/* connection lock, to prevent to functions to use the connection concurrent */
    info.conlock= TRUE;
	/* playlist number this is to check if the playlist changed */
    info.playlist_id = -1;
	/* the state, if the state changes I know I have to update some stuff */
    info.state = -1;
	/* the volume if the volume change I also have to update some stuff */
    info.volume = -1;
	/* the current song */
    info.song = -1;
	/* Elapsed or remaining time */
    info.time_format = 1;
	/* The Playlist, only in my memory */
    info.playlist = NULL;
	/* the current song */
    info.cursong = NULL;
	/* playlist running. */
    info.playlist_running = FALSE;
	/* filter enabled */
    info.show_filter = FALSE;
    memset(info.filter_entry, '\0', 256);
    strcpy(info.filter_entry, "");
    info.filter_field = 0;
	/* Some internall data..  this to save trafic with mpd daemon */
    info.playlist_list = NULL;
    info.cur_list = NULL; /* this one should be empty..  no need to keep that in memory */
    info.dir_list = NULL;
    info.file_list = NULL;
    info.id3_songs_list = NULL;
    info.search_list = NULL;
    info.id3_album_list = NULL;
    memset(info.path, '\0', 1024);
    strcpy(info.path, "/");
	/* playlists view hidden */
    info.playlist_view_hidden = FALSE;
    info.total_number_of_songs = 0;
    info.total_playtime = 0; 
	/* tray icon*/
    info.do_tray =FALSE;

    info.popup.do_popup = 1;
    info.popup.show_state = FALSE;
    info.popup.position = 0;
    info.popup.timeout = 5;
    info.popup.gmpc_image = NULL;
    info.popup. pixbuf_width = 0;
    info.popup.pixbuf_height = 0;
    info.popup.popup_stay = FALSE;
    
    }



int main(int argc, char **argv)
    {
    set_default_values();
    /* load config */
    load_config();    
    /* initialize gtk */
    gtk_init(&argc, &argv);    
    
    /* create the main window, This is done before anything else (but after command line check)*/
    create_player();
        
    /* create timeouts */
    /* get the status every 1/2 second should be enough */
    gtk_timeout_add(600, (GSourceFunc)update_mpd_status, NULL);
    update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
	update_interface();
    
    
    if(info.do_tray)  create_tray_icon();
    /* run the main loop */
    gtk_main();
    
    /* save config and quit */
    save_config();
    return 0;
    }


int update_interface()
    {

    /* update the preferences menu, I call this as soon as possible so the preferences menu can detect update */
    preferences_update();
    /* tray update */
    update_tray_icon();
    /* update the popup */
    update_popup();
    /* check if there is an connection. (that is when connection == NULL) */    
    if(info.connection == NULL)
    {
	    if(!preferences.autoconnect) return TRUE;
	    /* connect to mpd if that fails return this function */
	    if(connect_to_mpd())
	    {
		    /* make sure this is all set correct. just a little security */

		    info.connection = NULL;
		    info.conlock = TRUE;
		    return TRUE;
	    }
	    else
	    {
		    info.conlock = FALSE;
		    gtk_timeout_remove(update_timeout);
		    update_timeout =  gtk_timeout_add(400, (GSourceFunc)update_interface, NULL);
	    }
    }
    /* now start updating the rest */
    /* check if busy */
    if(info.conlock) return TRUE;

    /* ok save to update interface, no need to lock (yet)*/

    /* check for new playlist and load it if needed */
    if(info.playlist_id != info.status->playlist)
    {
	    if(load_playlist())
	    {
		    /* oeps error */
		    return TRUE;
	    }
    }
    /* update the playlist */
    update_playlist();

    /* update the player window */
    if(update_player())
    {
	    /* error return */
	    return TRUE;
    }

    /* return (must be true to keep timeout going) */
    /* set these to the good value. So there only updated when changed */
    info.playlist_id = info.status->playlist;
    if(info.status->state != MPD_STATUS_STATE_UNKNOWN) info.song = info.status->song;
    if(info.status->state == MPD_STATUS_STATE_STOP)  info.song = -1;    
    return TRUE;
    }
