#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"

GladeXML *xml_main_window = NULL;

int debug = FALSE;
int update_interface();
guint update_timeout = 0;

int main(int argc, char **argv)
    {
    /* check command line options */
    
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
