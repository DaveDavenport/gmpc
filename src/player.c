#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"
#define TITLE_LENGTH 42
scrollname scroll = {NULL, NULL, NULL, 0,0, TRUE};
/* wrapper functions for the title entry box. */

gboolean update_msg()
	{
	/* scroll will be -1 when there is getting stuff updated. hopefully this fixes the nasty segfault in pango*/
	if(scroll.exposed)
		{
		if(scroll.msg != NULL) g_free(scroll.msg);
		/* set the correct message in the msg box. and set posistion on 0 */
		if(scroll.popup_msg != NULL)
			{
			scroll.msg = g_strdup(scroll.popup_msg);
			}	
		else if(scroll.base_msg != NULL)
			{
			scroll.msg = g_strdup(scroll.base_msg);
			}
		else scroll.msg = g_strdup("Gnome Music Player Client");
		scroll.pos = 0;
		scroll.up = 0;
		}
  	 /* scroll the song text */
	{
	GtkWidget *entry = glade_xml_get_widget(xml_main_window, "title_entry");
	PangoLayout *layout = gtk_entry_get_layout(GTK_ENTRY(entry));
	int width;
	pango_layout_get_size(layout, &width, NULL);
	if(scroll.exposed)gtk_entry_set_text(GTK_ENTRY(entry), scroll.msg);
	if((width/PANGO_SCALE) > (entry->allocation.width - 8))
	    {
	    if(scroll.pos == 0 && scroll.up % 4 );
	    {
	    gtk_entry_set_text(GTK_ENTRY(entry), &scroll.msg[scroll.pos]);
	    scroll.pos++;
	    }
	    scroll.up++;
	    }
	else if(scroll.up < 0 && scroll.up != -4) 
	    {
	    scroll.up--;
	    }
	else {
		if(scroll.pos != 0)
		    {
		    if(scroll.up >= 0) scroll.up = -1;
		    else{
			scroll.up = 1;
		        scroll.pos = 0;
		        gtk_entry_set_text(GTK_ENTRY(entry), &scroll.msg[scroll.pos]);
			}
		    }
		else  gtk_entry_set_text(GTK_ENTRY(entry), &scroll.msg[scroll.pos]);
	     }
		scroll.exposed = FALSE;
	}
	/* return true .. so that the it keeps going */
	return TRUE;
	}

void msg_set_base(gchar *msg)
	{
	if(msg == NULL) return;
	/* don't update when its the same string :) */
	if(scroll.msg != NULL)
		{
		if(!strcmp(scroll.msg, msg)) return;
		}
	if(scroll.base_msg != NULL)
		{
			g_free(scroll.base_msg);
			scroll.base_msg = NULL;
		}
	if(!g_utf8_validate(msg, -1, NULL))
		{
		scroll.base_msg = g_strdup("No valid UTF-8. Please check youre locale");
		}
	else	scroll.base_msg = g_strdup(msg);
	scroll.exposed = TRUE;
	}

void msg_push_popup(gchar *msg)
	{
	if(msg == NULL) return;
	if(scroll.popup_msg != NULL)
		{
			g_free(scroll.popup_msg);
			scroll.popup_msg = NULL;
		}
	if(!g_utf8_validate(msg, -1, NULL))
		{
		scroll.popup_msg = g_strdup("No valid UTF-8. Please check youre locale");
		}
	else	scroll.popup_msg = g_strdup(msg);
	scroll.exposed = TRUE;
	}

void msg_pop_popup()
	{
	if(scroll.popup_msg != NULL)
		{
			g_free(scroll.popup_msg);
			scroll.popup_msg = NULL;
		}
	scroll.exposed = TRUE;
	}

/* this updates the player.. this is called from the update function */
/* conlock isnt locked at this point.. so If I do decide to get anything lock it */

int update_player()
    {
    /* update the volume slider */
    if(info.conlock) return TRUE;
    if(info.volume != info.status->volume)
	{
	GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
	gtk_range_set_value(scale, (double) info.status->volume);
	info.volume = info.status->volume;
	}    
    
    /* things that only need to be updated during playing */
    if(info.status->state == MPD_STATUS_STATE_PLAY)
	{
	/* update the progress bar */
	{
	    GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
    	    gdouble  prog = ((double)info.status->elapsedTime/(double)info.status->totalTime)*100;
    	    gtk_range_set_value(scale, prog);
	}
	/* update the time box */
	{

	GtkWidget *entry = glade_xml_get_widget(xml_main_window, "time_entry");
	    int e_min = (int)(info.status->elapsedTime/60);
	    int e_sec = info.status->elapsedTime - 60*e_min;
	    int r_min = (int)((info.status->totalTime- info.status->elapsedTime)/60);
	    int r_sec = info.status->totalTime - info.status->elapsedTime - r_min*60;
	    gchar *buf = NULL;
	    if(info.time_format == TIME_FORMAT_ELAPSED)
		{
		buf = g_strdup_printf("%02i:%02i", e_min, e_sec);
		}
	    else if (info.time_format == TIME_FORMAT_REMAINING) buf = g_strdup_printf("-%02i:%02i", r_min, r_sec);
	   else buf = g_strdup_printf("%3.1f%%", (double)((double)info.status->elapsedTime/(double)info.status->totalTime)*100);
	    gtk_entry_set_text(GTK_ENTRY(entry), buf);
	
	    g_free(buf);
	}
	}
    /* update the song title */
    
    if(info.song != info.status->song && info.status->state != MPD_STATUS_STATE_STOP)
    {
	    GList *node = g_list_nth(info.playlist, info.status->song);
	    mpd_Song *song;
	    if(node != NULL){
		    song = node->data;
		    /* make a global song */
		    if(info.status->state != MPD_STATUS_STATE_PLAY && info.status->state != MPD_STATUS_STATE_PAUSE)
		    {
			    msg_set_base("Gnome Music Player Client");
		    }
		    else
		    {
			    info.cursong = song;
			    if(song->artist != NULL && song->title != NULL)
			    {
				    gchar *buf = NULL;
				    if(song->title != NULL && song->artist != NULL) buf  = g_strdup_printf("%s - %s", song->title, song->artist);
				    else buf = g_strdup("GMPC - Invalid UTF-8. please check youre locale");
				    msg_set_base(buf);
				    g_free(buf);
			    }
			    else
			    {
				    gchar *buf  = g_path_get_basename(song->file);
				    msg_set_base(buf);
				    g_free(buf);
			    }
		    }
	    }
    }


    /* update if state changes */
    if(info.state != info.status->state)
    {
	    GtkWidget *image = glade_xml_get_widget(xml_main_window, "play_button_image");
	    if(info.status->state == MPD_STATUS_STATE_STOP || info.status->state == MPD_STATUS_STATE_UNKNOWN)
	    {
		    GtkWidget *entry = glade_xml_get_widget(xml_main_window, "time_entry");
		    msg_set_base("GMPC - Stopped");
		    if(info.time_format == TIME_FORMAT_ELAPSED)
		    {
			    gtk_entry_set_text(GTK_ENTRY(entry), "00:00");
		    }
		    else if(info.time_format == TIME_FORMAT_REMAINING) gtk_entry_set_text(GTK_ENTRY(entry), "-00:00");
		    else	 gtk_entry_set_text(GTK_ENTRY(entry), "0.0 %%");
		    entry =  glade_xml_get_widget(xml_main_window, "progress_slider");
		    gtk_range_set_value(GTK_RANGE(entry), 0);
		    
		    info.song = -1;
	    }
	    if(info.status->state == MPD_STATUS_STATE_PLAY) gtk_image_set_from_file(GTK_IMAGE(image), PIXMAP_PATH"/media-pause.png");
	    else gtk_image_set_from_file(GTK_IMAGE(image), PIXMAP_PATH"/media-play.png");

	    info.state = info.status->state;
    }
    /* update random and repeat button */
    /* lock it to stop them from toggling and triggering another toggle*/
    if(info.status->repeat != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rep_button"))))
    {
	    info.conlock = TRUE;
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rep_button")), info.status->repeat);
	    info.conlock = FALSE;
    }
    if(info.status->random != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rand_button"))))
    {
	    info.conlock = TRUE;
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rand_button")), info.status->random);
	    info.conlock = FALSE;
    }
    return FALSE;
    }



/* start seeking in the song..  only allow this when youre playing or paused */
/* block it other wise. */
/* everything is blocked until the seek is done. */
/* show time to seek to in entry box */
int progress_seek_start()
{
	if(info.conlock) return TRUE;
	info.conlock = TRUE;
	if(info.status->state != MPD_STATUS_STATE_PLAY && info.status->state != MPD_STATUS_STATE_PAUSE)
	{
		info.conlock = FALSE;
		return TRUE;
	}
	return FALSE;
}

void change_progress_update()
{
	if(info.conlock)
	{
		if(info.connection != NULL)
		{
			GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
			gchar *buf = NULL;
			gdouble value = gtk_range_get_value(scale);
			int newtime = (int)(info.status->totalTime*(double)(value/100));
			mpd_sendSeekCommand(info.connection,info.status->song,value);
			mpd_finishCommand(info.connection);
			if(check_for_errors()) return;

			if(info.time_format == TIME_FORMAT_ELAPSED)
			{
				int min = (int)(newtime/60);
				int sec = newtime - 60*min;
				int t_min = (int)(info.status->totalTime/60);
				int t_sec = info.status->totalTime - 60*t_min;
				buf = g_strdup_printf("Seek to %02i:%02i/%02i:%02i", min, sec, t_min, t_sec);
			}
			else if (info.time_format == TIME_FORMAT_REMAINING)
			{
				int t_min = (int)(info.status->totalTime/60);
				int t_sec = info.status->totalTime - 60*t_min;
				int min = (int)((info.status->totalTime -newtime)/60);
				int sec = (info.status->totalTime -newtime) - 60*min;
				buf = g_strdup_printf("Seek to -%02i:%02i/%02i:%02i", min, sec, t_min, t_sec);
			}	
			else buf = g_strdup_printf("Seek to %3.1f%%", value);
			msg_push_popup(buf);
			g_free(buf);
		}
		/* do this so the title gets updated again, even if it doesnt need scrolling */
		scroll.pos = -1;
	}
}    

/* apply seek changes */
int progress_seek_stop()
{
	msg_pop_popup();
	if(info.connection == NULL) return TRUE;
	else 
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
		gdouble value = gtk_range_get_value(scale);
		int change = (int)(info.status->totalTime*(double)(value/100));
		mpd_sendSeekCommand(info.connection,info.status->song, change);
		mpd_finishCommand(info.connection);
		if(check_for_errors()) return FALSE;
		info.status->elapsedTime = change;	
		info.conlock = FALSE;
	}
	return FALSE;
}

/* if the volume slider is pressed (mouse button)  it holds the update so I Can display the volume in */
/* the entry box and it doesn't tries to move my volume slider while sliding */
/* also if volume isnt "slidable" block the user from changing it */
int volume_change_start()
{
	if(info.conlock) return TRUE;
	if(info.volume == -1) return TRUE;
	info.conlock = TRUE;
	return FALSE;
}

/* if the volume changes say it in the entry box.. this looks nice :) */    
void volume_change_update()
{
	if(info.connection != NULL && info.conlock)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
		gdouble value = gtk_range_get_value(scale);
		gchar *buf = g_strdup_printf("Volume %i%%", (int)value);
		msg_push_popup(buf);
		g_free(buf);

		mpd_sendSetvolCommand(info.connection, (int)value);
		mpd_finishCommand(info.connection);
		if(check_for_errors()) return;

		/* do this so the title gets updated again, even if it doesnt need scrolling */
		/* it does look ugly .. need to find a better way */
		scroll.pos = -1;
	}
	else if(info.connection != NULL)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
		gdouble value = gtk_range_get_value(scale);
		if(value != info.volume)
		{
			mpd_sendSetvolCommand(info.connection, (int)value);
			mpd_finishCommand(info.connection);        
			if(check_for_errors()) return;
		}
	}
}
/* apply changes and give mpd free */
int volume_change_stop()
{
	msg_pop_popup();
	if(info.connection == NULL) return TRUE;
	else 
	{
		info.conlock = FALSE;    
	}
	return FALSE;
}

/* change the time format between elapsing and remaining and percentage */
void time_format_toggle()
{
	info.time_format++;
	if(info.time_format > 2) info.time_format = 0;
}

/* function to remove the id3 info screen and unref the xml tree */
/*void remove_id3_window(GtkWidget *button)
  {
  GtkWidget *window = gtk_widget_get_toplevel(button);	
  GladeXML *xml_id3_window = glade_get_widget_tree(window);
  gtk_widget_destroy(window);
  if(xml_id3_window != NULL)g_object_unref(xml_id3_window);
  }
  */
/* the id3 info screen */
void id3_info()
{
	GtkWidget *dialog = NULL;
	GladeXML *xml_id3_window;
	GList *node;
	mpd_Song *song;
	call_id3_window(info.status->song);
	return;
	if(info.connection == NULL) return;
	if(info.status->state == MPD_STATUS_STATE_PLAY || info.status->state == MPD_STATUS_STATE_PAUSE);
	else return;

	xml_id3_window = glade_xml_new(GLADE_PATH"gmpc.glade", "id3_info_window", NULL);
	/* check for errors and axit when there is no gui file */
	if(xml_id3_window == NULL)  g_error("Couldnt initialize GUI. Please check installation\n");

	/* set info from struct */
	node = g_list_nth(info.playlist, info.status->song);
	if(node == NULL)
	{
		gtk_widget_destroy(dialog);
		g_object_unref(xml_id3_window);
		xml_id3_window = NULL;
		return;
	}
	song = node->data;
	if(song->artist != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_id3_window, "artist_entry")), song->artist);
	}
	if(song->title != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_id3_window, "title_entry")), song->title);
	}
	if(song->album != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_id3_window, "album_entry")),song->album);
	}
	if(song->track != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_id3_window, "track_entry")), song->track);
	}
	if(song->file != NULL)
	{
		gchar *buf1 = g_path_get_basename(song->file);
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_id3_window, "filename_entry")), buf1);
		g_free(buf1);
	}

	glade_xml_signal_autoconnect(xml_id3_window);
}

/* create the player and connect signals */
void create_player()
{
	xml_main_window = glade_xml_new(GLADE_PATH"gmpc.glade", "main_window", NULL);
	/* check for errors and axit when there is no gui file */
	if(xml_main_window == NULL)  g_error("Couldnt initialize GUI. Please check installation\n");
	glade_xml_signal_autoconnect(xml_main_window);
	gtk_timeout_add(400, (GSourceFunc)update_msg, NULL);
}

