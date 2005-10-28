/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */



#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include "main.h"
#include "misc.h"
#include "strfsong.h"
#include "playlist3.h"
#include "config1.h"
extern config_obj *config;

#define TITLE_LENGTH 42
gint DISPLAY_WIDTH = 240;

scrollname scroll = {NULL, NULL, NULL,NULL, 0,0, TRUE,TRUE};
PangoLayout *layout = NULL, *time_layout = NULL;
guint expose_display_id = 0;

guint seek = FALSE;
guint volume = FALSE;

void title_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
	if(allocation->width != 0) DISPLAY_WIDTH = allocation->width;
}

void time_exposed(GtkWidget *window)
{
	gint height, width;
	GtkRequisition req;
	pango_layout_get_size(time_layout, &width, &height);
	width  = width/PANGO_SCALE;
	height = height/PANGO_SCALE;
	gtk_widget_size_request(window, &req);

	
	if(height+6 != req.height || width+6 > req.width)
	{
		gtk_widget_set_size_request(window,width+6,height+6);	
	}                                                       	

	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->base_gc[GTK_STATE_NORMAL],
			TRUE,
			0,0,
			req.width,req.height);
	gdk_draw_layout(GDK_DRAWABLE(window->window),
			window->style->text_gc[GTK_STATE_NORMAL],
			MAX(0,(req.width-width)/2),MAX(0, (req.height-height)/2) ,
			time_layout);

	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->dark_gc[GTK_STATE_NORMAL],
			FALSE,
			0,0,
			req.width-1,req.height-1);

}



void display_exposed(GtkWidget *window)
{
	int width, height;
	GtkRequisition req;
	gtk_widget_size_request(window, &req);
	g_signal_handler_block(G_OBJECT(window), expose_display_id);
	pango_layout_get_size(layout, &width, &height);
	width  = width/PANGO_SCALE;
	height = height/PANGO_SCALE;

	if(height+6 > req.height)
	{
		gtk_widget_set_size_request(window,-1,height+6);
	}

	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->base_gc[GTK_STATE_NORMAL],
			TRUE,
			0,0,
			DISPLAY_WIDTH,req.height);                            
	if(width <= DISPLAY_WIDTH-5)
	{
		gdk_draw_layout(GDK_DRAWABLE(window->window), 
			window->style->text_gc[GTK_STATE_NORMAL], 
			3, MAX(0, (req.height-height)/2),
			layout);
	}
	else{
		if(width-scroll.pos > DISPLAY_WIDTH)
		{
			gdk_draw_layout(GDK_DRAWABLE(window->window), 
					window->style->text_gc[GTK_STATE_NORMAL], 
					-scroll.pos, MAX(0, (req.height-height)/2),
					layout);
		}
		else
		{
			gdk_draw_layout(GDK_DRAWABLE(window->window), 
					window->style->text_gc[GTK_STATE_NORMAL], 
					-scroll.pos, MAX(0, (req.height-height)/2),
					layout);



			gdk_draw_layout(GDK_DRAWABLE(window->window), 
					window->style->text_gc[GTK_STATE_NORMAL], 
					(-scroll.pos+width),MAX(0, (req.height-height)/2),
					layout);
		}

		if((width-scroll.pos) < 0)
		{
			scroll.pos = 0;

		}
	}

	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->dark_gc[GTK_STATE_NORMAL],
			FALSE,
			0,0,
			DISPLAY_WIDTH-1,req.height-1);                              
	g_signal_handler_unblock(G_OBJECT(window), expose_display_id);
}	
gboolean update_msg()
{
	int width;
	/* scroll will be -1 when there is getting stuff updated. hopefully this fixes the nasty segfault in pango*/
	if(scroll.exposed)
	{
		if(scroll.msg != NULL) g_free(scroll.msg);
		/* set the correct message in the msg box. and set posistion on 0 */
		if(scroll.popup_msg != NULL) {
			scroll.msg = g_strdup(scroll.popup_msg);
		} else if(scroll.base_msg != NULL){
			scroll.msg = g_strdup(scroll.base_msg);
		} else {
			scroll.msg = g_strdup(_("Gnome Music Player Client"));
		}
		scroll.pos = 0;
		scroll.up = 0;
		pango_layout_set_text(layout, scroll.msg, -1);
		pango_layout_get_size(layout, &width, NULL);
		width = width/PANGO_SCALE;
		if(width > DISPLAY_WIDTH-5 && scroll.do_scroll)
		{
			char *temp= scroll.msg;
			scroll.msg = g_strdup_printf("%s  ***  ", scroll.msg);
			g_free(temp);
			pango_layout_set_text(layout, scroll.msg, -1);
		}

	}
	/* scroll the song text */
	{
		GtkWidget *window = glade_xml_get_widget(xml_main_window, "entry_image");
		pango_layout_get_size(layout, &width, NULL);

		width = width/PANGO_SCALE;

		if(width > DISPLAY_WIDTH-5)
		{
			if(scroll.do_scroll)
			{
				scroll.pos+=4;
			}
			else
			{
				scroll.pos = -2;

			}

		}
		gtk_widget_queue_draw(window);

		scroll.exposed = FALSE;
	}
	/* return true .. so that the it keeps going */
	return TRUE;
}

void msg_set_base(gchar *msg)
{
	int i =0;
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
	if(!g_utf8_validate(msg, -1, NULL)){
		scroll.base_msg = g_strdup(_("No valid UTF-8. Please check your locale"));
	}
	else {
		scroll.base_msg = g_strdup(msg);
	}

	scroll.do_scroll = TRUE;
	for(;msg[i] != '\0';i++)
	{
		if(msg[i] == '\n')
		{
			scroll.do_scroll =FALSE;
		}
	}

	scroll.exposed = TRUE;
}

void msg_push_popup(gchar *msg)
{
	if(msg == NULL) return;
	if(scroll.queue == NULL)
	{
		scroll.queue = g_queue_new();
	}
	if(!g_utf8_validate(msg, -1, NULL))
	{
		g_queue_push_tail(scroll.queue,g_strdup(_("No valid UTF-8. Please check your locale")));
		scroll.popup_msg = g_queue_peek_tail(scroll.queue);
	}
	else
	{
		g_queue_push_tail(scroll.queue,g_strdup(msg));
		scroll.popup_msg = g_queue_peek_tail(scroll.queue);
	}
	scroll.exposed = TRUE;
}

void msg_pop_popup()
{
	if(scroll.popup_msg != NULL)
	{
		char *msg = g_queue_peek_tail(scroll.queue);

		g_queue_pop_tail(scroll.queue);
		scroll.popup_msg = g_queue_peek_tail(scroll.queue);
		g_free(msg);
	}
	scroll.exposed = TRUE;
}

/* this updates the player.. this is called from the update function */
/* conlock isnt locked at this point.. so If I do decide to get anything lock it */

int update_player()
{
	if(!mpd_check_connected(connection)) return FALSE;
	/* update the volume slider */
	GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
	if((int)gtk_range_get_value(scale) != mpd_status_get_volume(connection))
	{
		gtk_range_set_value(scale, (double)mpd_status_get_volume(connection));
	}    

	/* things that only need to be updated during playing */
	{
		int totalTime = mpd_status_get_total_song_time(connection);
		int elapsedTime = mpd_status_get_elapsed_song_time(connection);		
		/* update the progress bar */
		if(!seek){
			GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
			gdouble  prog = ((double)elapsedTime/(double)totalTime)*100;
			gtk_range_set_value(scale, prog);
		}
		/* update the time box */
		{
			int e_min = (int)(elapsedTime/60);
			int e_sec = elapsedTime - 60*e_min;
			int r_min = (int)((totalTime- elapsedTime)/60);
			int r_sec = totalTime - elapsedTime - r_min*60;
			gchar *buf = NULL;


			if(cfg_get_single_value_as_int_with_default(config, "player", "time-format", TIME_FORMAT_ELAPSED) == TIME_FORMAT_ELAPSED)
			{
				/* if more then 100 minutes player change to hh:mm */
				if(abs(e_min) >= 100)
				{
					e_sec = (e_min % 60);
					e_min = (int)(e_min/60);
				}                                				
				buf = g_strdup_printf("%02i:%02i", abs(e_min), abs(e_sec));
			}
			else if (cfg_get_single_value_as_int(config, "player", "time-format") == TIME_FORMAT_REMAINING)
			{
				/* if more then 100 minutes player change to hh:mm */
				if(abs(r_min) >= 100)
				{                                                               				
					r_sec = (r_min % 60);
					r_min = (int)(r_min/60);
				}                                								
				buf = g_strdup_printf("-%02i:%02i", abs(r_min), abs(r_sec));
			}
			else{
				if(totalTime <= 0)
				{
					buf = g_strdup("n/a");
				}
				else
				{
					buf = g_strdup_printf("%3.1f %%", (double)((double)elapsedTime/(double)totalTime)*100);
				}
			}
			pango_layout_set_text(time_layout, buf, -1);
			gtk_widget_queue_draw(glade_xml_get_widget(xml_main_window, "time_image"));

			g_free(buf);
		}
	}

	/* update random and repeat button */
	/* lock it to stop them from toggling and triggering another toggle*/
	if(mpd_player_get_repeat(connection) != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rep_button"))))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rep_button")), mpd_player_get_repeat(connection));
	}
	if(mpd_player_get_random(connection) != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rand_button"))))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rand_button")), mpd_player_get_random(connection));
	}
	return FALSE;
}


void player_state_changed(int old_state, int state)
{
	GtkWidget *image = glade_xml_get_widget(xml_main_window, "play_button_image");
	if(state == MPD_PLAYER_STOP || state == MPD_PLAYER_UNKNOWN)
	{
		GtkWidget *entry;
		msg_set_base(_("GMPC - Stopped"));
		if(cfg_get_single_value_as_int_with_default(config, "player", "time-format",TIME_FORMAT_ELAPSED) == TIME_FORMAT_ELAPSED)
		{
			pango_layout_set_text(time_layout, "00:00", -1);
		}
		else if(cfg_get_single_value_as_int(config, "player", "time-format") == TIME_FORMAT_REMAINING)
		{
			pango_layout_set_text(time_layout, "-00:00", -1);

		}
		else	
		{
			pango_layout_set_text(time_layout, "0.0 %", -1);
		}
		gtk_widget_queue_draw(glade_xml_get_widget(xml_main_window, "time_image"));
		entry =  glade_xml_get_widget(xml_main_window, "progress_slider");
		gtk_range_set_value(GTK_RANGE(entry), 0);

	}
	if(state == MPD_PLAYER_PLAY)
	{	
		gtk_image_set_from_stock(GTK_IMAGE(image),"gtk-media-pause", GTK_ICON_SIZE_BUTTON);
	}
	else
	{
		gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-media-play", GTK_ICON_SIZE_BUTTON);
	}

	if(state != MPD_PLAYER_PLAY && state != MPD_PLAYER_PAUSE)
	{
		msg_set_base(_("Gnome Music Player Client"));
		if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
		{
			gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), _("Gnome Music Player Client"));	
		}
	}
	else
	{
		gchar buffer[1024];
		char *string =cfg_get_single_value_as_string_with_default(config, "player","display_markup",DEFAULT_PLAYER_MARKUP);
		strfsong(buffer, 1024,string,mpd_playlist_get_current_song(connection));
		cfg_free_string(string);
		msg_set_base(buffer);
		if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
		{
			int i=0,j=0;
			int stripes = 0;
			char *new_buf = NULL;
			/* calculate the number of needed "stripes" */
			for(i=0;buffer[i] != '\0';i++)
			{
				if(buffer[i] == '\n') stripes++; 
			}
			/* malloc a new string.. size of old string + the number of needed "stripes"*2 ('\n' -> ' - ' == +2 chars) + a ending \0*/
			new_buf = (char *)g_malloc0((i+2*stripes+1)*sizeof(char ));
			
			for(i=0;buffer[i] != '\0';i++)
			{

				if(buffer[i] == '\n')
				{
					new_buf[j] = ' ';
					new_buf[j+1] = '-';
					new_buf[j+2] = ' ';
					j+=2;
				}
				else
				{
					new_buf[j] = buffer[i];
				}
				j++;
			}
			

			gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), new_buf);	
			g_free(new_buf);
		}
	}

}


void player_song_changed()
{
	int state = mpd_player_get_state(connection);

	if(state != MPD_PLAYER_PLAY && state != MPD_PLAYER_PAUSE)
	{
		msg_set_base(_("Gnome Music Player Client"));
		if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
		{
			gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), _("Gnome Music Player Client"));	
		}
	}
	else
	{
		gchar buffer[1024];
		char *string = cfg_get_single_value_as_string_with_default(config, "player","display_markup",DEFAULT_PLAYER_MARKUP);
		strfsong(buffer, 1024, string,mpd_playlist_get_current_song(connection));
		cfg_free_string(string);
		msg_set_base(buffer);
		if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
		{
			int i=0,j=0;
			int stripes = 0;
			char *new_buf = NULL;
			/* calculate the number of needed "stripes" */
			for(i=0;buffer[i] != '\0';i++)
			{
				if(buffer[i] == '\n') stripes++; 
			}
			/* malloc a new string.. size of old string + the number of needed "stripes"*2 ('\n' -> ' - ' == +2 chars) + a ending \0*/
			new_buf = (char *)g_malloc0((i+2*stripes+1)*sizeof(char ));
			/* iterate through the string and copy it.. when meeting '\n' insert ' - ' */
			for(i=0;buffer[i] != '\0';i++)
			{

				if(buffer[i] == '\n')
				{
					new_buf[j] = ' ';
					new_buf[j+1] = '-';
					new_buf[j+2] = ' ';
					j+=2;
				}
				else
				{
					new_buf[j] = buffer[i];
				}
				j++;
			}
			gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), new_buf);	
			g_free(new_buf);
		}
	}
}


/* start seeking in the song..  only allow this when you're playing or paused */
/* block it other wise. */
/* everything is blocked until the seek is done. */
/* show time to seek to in entry box */
int progress_seek_start()
{
	if(mpd_player_get_state(connection) != MPD_PLAYER_PLAY && 
			mpd_player_get_state(connection) != MPD_PLAYER_PAUSE)
	{
		return TRUE;
	}
	seek = TRUE;
	msg_push_popup("Seek To:");
	return FALSE;
}

void change_progress_update()
{
	if(seek)
	{
		if(mpd_check_connected(connection))
		{
			GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
			gchar *buf = NULL;
			gdouble value = gtk_range_get_value(scale);
			int totaltime = mpd_status_get_total_song_time(connection);
			int newtime = (int)(totaltime*(double)(value/100));
			if(cfg_get_single_value_as_int_with_default(config, "player", "time-format",TIME_FORMAT_ELAPSED)  == TIME_FORMAT_ELAPSED)
			{
				int min = (int)(newtime/60);
				int sec = newtime - 60*min;
				int t_min = (int)(totaltime/60);
				int t_sec = totaltime - 60*t_min;
				buf = g_strdup_printf(_("Seek to %02i:%02i/%02i:%02i"), min, sec, t_min, t_sec);
			}
			else if (cfg_get_single_value_as_int(config, "player", "time-format") == TIME_FORMAT_REMAINING)
			{
				int t_min = (int)(totaltime/60);
				int t_sec = totaltime - 60*t_min;
				int min = (int)((totaltime -newtime)/60);
				int sec = (totaltime -newtime) - 60*min;
				buf = g_strdup_printf(_("Seek to -%02i:%02i/%02i:%02i"), min, sec, t_min, t_sec);
			}	
			else buf = g_strdup_printf(_("Seek to %3.1f%%"), value);
			msg_pop_popup(buf);
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
	seek = FALSE;
	if(!mpd_check_connected(connection))
	{
		return TRUE;
	}
	else if(mpd_player_get_state(connection) == MPD_PLAYER_PLAY || mpd_player_get_state(connection) == MPD_PLAYER_PAUSE)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
		gdouble value = gtk_range_get_value(scale);
		int change = (int)(mpd_status_get_total_song_time(connection)*(double)(value/100));

		mpd_player_seek(connection, change);
	}
	return FALSE;
}

/* if the volume slider is pressed (mouse button)  it holds the update so I Can display the volume in */
/* the entry box and it doesn't tries to move my volume slider while sliding */
/* also if volume isnt "slidable" block the user from changing it */
int volume_change_start()
{
	if(!mpd_check_connected(connection) || volume) return TRUE;
	if(mpd_status_get_volume(connection) == -1) return TRUE;
	volume = TRUE;
	msg_push_popup("Volume: ");
	return FALSE;
}

/* if the volume changes say it in the entry box.. this looks nice :) */    
void volume_change_update()
{
	if(mpd_check_connected(connection) && volume)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
		gdouble value = gtk_range_get_value(scale);
		gchar *buf = g_strdup_printf(_("Volume %i%%"), (int)value);
		msg_pop_popup();
		msg_push_popup(buf);
		g_free(buf);

		if(mpd_status_set_volume(connection,(int)value) < 0)
		{
			return;
		}

		/* do this so the title gets updated again, even if it doesnt need scrolling */
		/* it does look ugly .. need to find a better way */
		scroll.pos = -1;
	}
	else if(mpd_check_connected(connection))
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
		gdouble value = gtk_range_get_value(scale);
		if(value != mpd_status_get_volume(connection))
		{
			if(mpd_status_set_volume(connection, (int)value) < 0)
			{
				return;
			}                                                 			
		}
	}
}
/* apply changes and give mpd free */
int volume_change_stop()
{
	msg_pop_popup();
	volume = FALSE;
	if(!mpd_check_connected(connection)) return TRUE;
	return FALSE;
}

/* change the time format between elapsing and remaining and percentage */
void time_format_toggle()
{
	int time = cfg_get_single_value_as_int(config, "player", "time-format");
	time++;
	if(time > 2)
	{
		time = 0;
	}
	cfg_set_single_value_as_int(config, "player", "time-format",time);
}


/* the id3 info screen */
void id3_info()
{
	if(!mpd_check_connected(connection)) return;
	if(mpd_player_get_current_song_id(connection) > 0);
	call_id3_window(mpd_player_get_current_song_id(connection));
}

void style_changed(GtkWidget *window, GtkStyle *prev, PangoLayout *lay)
{
	pango_layout_context_changed(lay);
}

int player_key_press(GtkWidget *mw, GdkEventKey *event,gpointer data)
{
	/* go back 10 seconds */
	if(event->keyval == GDK_Left)
	{
		seek_ns(SEEK_STEP);
		return TRUE;	
	}
	/* go forward 10 seconds */
	else if (event->keyval == GDK_Right)
	{
		seek_ps(SEEK_STEP);
		return TRUE;	
	}
	/* volume up */
	else if (event->keyval == GDK_Up)
	{
		mpd_status_set_volume(connection,mpd_status_get_volume(connection) +5);
		return TRUE;
	}
	/* volume down */
	else if (event->keyval == GDK_Down)
	{
		mpd_status_set_volume(connection,mpd_status_get_volume(connection) -5);
		return TRUE;
	}
	else if (event->keyval == GDK_q && event->state == GDK_CONTROL_MASK)
	{
		/*gtk_main_quit();*/
		main_quit();
		return TRUE;

	}
	else if (event->keyval == GDK_j)
	{
		pl3_playlist_search();
	}
	else if (event->keyval == GDK_i)
	{
		id3_info();
	}
	return FALSE;
}


/* create the player and connect signals */
void create_player()
{
	xml_main_window = glade_xml_new(GLADE_PATH"gmpc.glade", "main_window", NULL);
	/* check for errors and axit when there is no gui file */
	if(xml_main_window == NULL)  g_error(_("Couldnt initialize GUI. Please check installation\n"));
	glade_xml_signal_autoconnect(xml_main_window);

	DISPLAY_WIDTH = glade_xml_get_widget(xml_main_window, "entry_image")->allocation.width;
	/* set icons from the custom stock set*/
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(xml_main_window, "prev_im")), "gtk-media-previous", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(xml_main_window, "next_im")), "gtk-media-next", GTK_ICON_SIZE_BUTTON);	
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(xml_main_window, "stop_im")), "gtk-media-stop", GTK_ICON_SIZE_BUTTON);	
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(xml_main_window, "play_button_image")), "gtk-media-play", GTK_ICON_SIZE_BUTTON);	
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(xml_main_window, "rand_im")), "media-random", GTK_ICON_SIZE_BUTTON);			
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(xml_main_window, "rep_im")), "media-repeat", GTK_ICON_SIZE_BUTTON);			


	gtk_widget_set_app_paintable(glade_xml_get_widget(xml_main_window, "entry_image"),TRUE);
	gtk_widget_set_app_paintable(glade_xml_get_widget(xml_main_window, "time_image"),TRUE);
	layout = gtk_widget_create_pango_layout(glade_xml_get_widget(xml_main_window, "entry_image"), "");
	time_layout = gtk_widget_create_pango_layout(glade_xml_get_widget(xml_main_window, "time_image"), "");
	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "entry_image")), 
			"style-set", G_CALLBACK(style_changed), layout);
	expose_display_id = g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "entry_image")), 
			"expose-event", G_CALLBACK(display_exposed), layout);

	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "time_image")), 
			"style-set", G_CALLBACK(style_changed), time_layout);
	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "time_image")), 
			"expose-event", G_CALLBACK(time_exposed), time_layout);

	pango_layout_set_text(time_layout, "00:00", -1);

	g_signal_connect(
			G_OBJECT(glade_xml_get_widget(xml_main_window, "main_window")),		
			"key-press-event",
			G_CALLBACK(player_key_press),
			NULL);
	/* check for errors and axit when there is no gui file */
	gtk_timeout_add(300, (GSourceFunc)update_msg, NULL);
	time_exposed(glade_xml_get_widget(xml_main_window, "time_image"));
	display_exposed(glade_xml_get_widget(xml_main_window, "entry_image"));
}
