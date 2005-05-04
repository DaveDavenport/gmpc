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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>
#include <time.h>
#include "debug_printf.h"
#include "libmpdclient.h"
#include "config1.h"
#include "playlist3.h"
#include "main.h"
#include "strfsong.h"
#include "misc.h"
#include "mm-keys.h"
extern int debug_level;
void playlist_changed(MpdObj *mi, int old_playlist_id, int new_playlist_id);
void init_playlist ();
void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data);
void song_changed(MpdObj *mi, int oldsong, int newsong);
void state_callback(MpdObj *mi, int old_state, int new_state, gpointer data);
void status_callback(MpdObj *mi);
void connect_callback();
void database_changed();
/*
 * the xml fle pointer to the player window 
 */
GladeXML *xml_main_window = NULL;
GladeXML *xml_error_window = NULL;

int update_interface ();

/*
 * the ID of the update timeout
 */
guint update_timeout = 0;
void init_stock_icons ();
extern GtkListStore *pl2_store;
/*
 * The Config object
 */
config_obj *config = NULL;
/* 
 * The Connection object
 */
MpdObj *connection = NULL;


void main_trigger_update ()
{
	if (check_connection_state(info))
	{
		update_mpd_status ();
		update_interface ();
	}
}


/*
 * sets default values in the main struct's 
 */
	void
set_default_values ()
{
	/*
	 * playlist number this is to check if the playlist changed 
	 */
	info.playlist_id = -1;
	info.playlist_length = 0;
	info.playlist_playtime = 0;
	/*
	 * the current song 
	 */
	info.old_pos = -1;
	/*
	 * tray icon 
	 */
	info.hidden = FALSE;

	/* */
	info.sb_hidden = FALSE;

	/*
	 * updating 
	 */
	info.updating = FALSE;
}



int main (int argc, char **argv)
{
	MmKeys *keys = NULL;
	gchar *url = NULL;
	/* debug stuff */

	if(argc > 1)
	{
		int i;
		for(i = 1; i< argc; i++)
		{
			if(!strncasecmp(argv[i], "--enable-debug=", 15))
			{
				debug_level = atoi(&argv[i][15]);
				debug_level = (debug_level < 0)? 0:((debug_level > DEBUG_INFO)? DEBUG_INFO:debug_level);
			}
		}

	}


	
#ifdef ENABLE_NLS
	debug_printf(DEBUG_INFO, "main.c: Setting NLS");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	/* initialize the settings */
	debug_printf(DEBUG_INFO, "main.c: loading default values");
	set_default_values ();

	/* Check for and create dir if availible */
	url = g_strdup_printf("%s/.gmpc/", g_getenv("HOME"));
	debug_printf(DEBUG_INFO, "main.c: Checking for %s existence",url);
	if(!g_file_test(url, G_FILE_TEST_EXISTS))
	{
		debug_printf(DEBUG_INFO, "main.c: Trying to create %s",url);
		if(mkdir(url,0777) < 0)
		{
			debug_printf(DEBUG_ERROR, "Failed to create: %s\n", url);
			return 1;
		}
	}
	else if (!g_file_test(url, G_FILE_TEST_IS_DIR))
	{
		debug_printf(DEBUG_ERROR, "%s isn't a directory.\n", url);
		return 1;
	}
	else
	{
		debug_printf(DEBUG_INFO, "main.c: %s exist and is directory",url);
	}
	g_free(url);

	/* OPEN CONFIG FILE */
	url = g_strdup_printf("%s/.gmpc/gmpc.xml", g_getenv("HOME"));
	debug_printf(DEBUG_INFO, "main.c: Trying to open the config file: %s", url);
	config = cfg_open(url);

	
	/* test if config open  */
	if(config == NULL)
	{
		debug_printf(DEBUG_ERROR,"main.c: Failed to save/load configuration:\n%s\n",url);
		return 1;
	}

	g_free(url);

	/* Create connection object */
	connection = mpd_ob_new_default();
	if(connection == NULL)
	{
		debug_printf(DEBUG_ERROR,"main.c: Failed to create connection obj\n");
		return 1;
	}
	/* connect signals */
	mpd_ob_signal_set_playlist_changed(connection, (void *)playlist_changed);
	mpd_ob_signal_set_error(connection, (void *)error_callback, NULL);
	mpd_ob_signal_set_song_changed(connection, (void *)song_changed, NULL);
	mpd_ob_signal_set_state_changed(connection, (void *)state_callback, NULL);
	mpd_ob_signal_set_status_changed(connection, (void *)status_callback, NULL);
	mpd_ob_signal_set_disconnect(connection, (void *)disconnect_callback, NULL);	
	mpd_ob_signal_set_connect(connection, (void *)connect_callback, NULL);
	mpd_ob_signal_set_database_changed(connection, (void *)database_changed, NULL);
	/*
	 * initialize gtk 
	 */
	gtk_init (&argc, &argv);
	gnome_vfs_init ();

	/*
	 * stock icons 
	 */
	init_stock_icons ();


	/*
	 * create the main window, This is done before anything else (but after command line check) 
	 */
	create_player ();
	if(cfg_get_single_value_as_int_with_default(config,"tray-icon", "enable", DEFAULT_TRAY_ICON_ENABLE) &&  
			cfg_get_single_value_as_int_with_default(config,"player", "hide-startup", DEFAULT_HIDE_ON_STARTUP))
	{
		gtk_widget_hide(GTK_WIDGET(glade_xml_get_widget(xml_main_window, "main_window")));
		info.hidden = TRUE;
	}
	else
	{

	}

	/* create the store for the playlist */
	init_playlist ();

	/*
	 * create timeouts 
	 */
	/*
	 * get the status every 1/2 second should be enough 
	 */
	gtk_timeout_add (cfg_get_single_value_as_int_with_default(config, "connection","mpd-update-speed",500),
			(GSourceFunc)update_mpd_status, NULL);
	update_timeout = gtk_timeout_add (5000,(GSourceFunc)update_interface, NULL);


	/* create a tray icon */
	if (cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable",1))
	{
		create_tray_icon();
	}
	/* update the interface */
	while(gtk_events_pending())
	{
		gtk_main_iteration();
	}
	update_interface();
	
	/*
	 * Keys
	 */
	keys = mmkeys_new();
	g_signal_connect(G_OBJECT(keys), "mm_playpause", G_CALLBACK(play_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_next", G_CALLBACK(next_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_prev", G_CALLBACK(prev_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_stop", G_CALLBACK(stop_song), NULL);

	/*
	 * run the main loop 
	 */

	gtk_main ();
	/* cleaning up. */
	/* this is "slow" mostly because of gtk_list_store_clear */
/*	if(mpd_ob_check_connected(connection))
	{
		mpd_ob_disconnect(connection);
	}

	mpd_ob_free(connection);	
*/	config_close(config);
	return 0;
}


int update_interface ()
{
	/*
	 * update the preferences menu, I call this as soon as possible so the preferences menu can detect update 
	 */
	preferences_update ();

	/*
	 * check if there is an connection. (that is when connection == NULL) 
	 */
	if (!mpd_ob_check_connected(connection))
	{
		/*
		 * update the popup 
		 */
		if (!cfg_get_single_value_as_int_with_default(config, "connection", "autoconnect", 0))
		{
			return TRUE;
		}
		/*
		 * connect to mpd if that fails return this function 
		 */
		if (connect_to_mpd ())
		{
			return TRUE;
		}
		/* connected succesfull */
		else
		{
			gtk_timeout_remove (update_timeout);
			update_timeout = gtk_timeout_add (400,(GSourceFunc)update_interface, NULL);
		}
	}
	/*
	 * now start updating the rest 
	 */
	/*
	 * check if busy 
	 */
	if(!mpd_ob_check_connected(connection)) return TRUE;
	update_player();

	/*
	 * set these to the good value. So there only updated when changed 
	 */
	return TRUE;
}


void playlist_changed(MpdObj *mi, int old_playlist_id, int new_playlist_id)
{
	MpdData *data = NULL;

	/*
	 * so I don't have to check all the time 
	 */
	gint old_length = 0;
	GtkTreeIter iter;
	gchar buffer[1024];
	debug_printf(DEBUG_INFO, "playlist_changed_callback: playlist changed\n");
	old_length = info.playlist_length;
	char *string = cfg_get_single_value_as_string_with_default(config, "playlist","markup", DEFAULT_PLAYLIST_MARKUP);

	data = mpd_ob_playlist_get_changes(mi,info.playlist_id);

	if(data == NULL)
	{
		debug_printf(DEBUG_ERROR, "playlist_changed_callback: what is this, stupid error\n");
		return;
	}
	while(data != NULL)
	{
		/*
		 * decide wether to update or to add 
		 */
		if(data->value.song->pos < old_length)
		{
			/*
			 * needed for getting the row 
			 */
			gchar *path = g_strdup_printf ("%i", data->value.song->pos);	
			if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (pl2_store), &iter, path))
			{
				/* overwriting existing entry */
				gint weight = PANGO_WEIGHT_NORMAL;
				gint time=0;
				if (data->value.song->id == mpd_ob_player_get_current_song_id(connection))
				{
					weight = PANGO_WEIGHT_ULTRABOLD;
				}
				/* get old time */
				gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_TIME, &time, -1);
				if(time != MPD_SONG_NO_TIME)
				{
					info.playlist_playtime -= time;
				}
				if(data->value.song->time != MPD_SONG_NO_TIME)
				{
					info.playlist_playtime += data->value.song->time;
				}
				strfsong (buffer, 1024,
						string,
						data->value.song);						
		
				gtk_list_store_set (pl2_store, &iter,
						SONG_ID,data->value.song->id, 
						SONG_POS,data->value.song->pos, 					
						SONG_TITLE, buffer,
						WEIGHT_ENABLE, TRUE, 
						WEIGHT_INT, weight,
						SONG_STOCK_ID,(strstr(data->value.song->file,"://") == NULL) ?"media-audiofile"	: "media-stream",
						SONG_TIME,data->value.song->time,
						STOCK_ALIGN, 0.0,
						-1);
			}
			g_free(path);
		}
		else
		{
			int weight = PANGO_WEIGHT_NORMAL;
			if(data->value.song->time != MPD_SONG_NO_TIME)
			{
				info.playlist_playtime += data->value.song->time;
			}
			if (data->value.song->id == mpd_ob_player_get_current_song_id(connection))
			{
				weight = PANGO_WEIGHT_ULTRABOLD;                                  			
			}

			strfsong (buffer, 1024,	string,	data->value.song);
			gtk_list_store_append (pl2_store, &iter);
			gtk_list_store_set (pl2_store, &iter,
					SONG_ID,data->value.song->id, 
					SONG_POS,data->value.song->pos, 					
					SONG_TITLE, buffer,
					WEIGHT_ENABLE, TRUE, 
					WEIGHT_INT, weight,
					SONG_STOCK_ID,(strstr(data->value.song->file,"://") == NULL) ?"media-audiofile"	: "media-stream",
					SONG_TIME,data->value.song->time,
					STOCK_ALIGN,0.0,
					-1);

		}
		data= mpd_ob_data_get_next(data);		
	}

	if(connection->status != NULL)
	{
		while (connection->status->playlistLength < old_length)
		{
			gchar *path = g_strdup_printf ("%i", old_length - 1);
			if (gtk_tree_model_get_iter_from_string
					(GTK_TREE_MODEL (pl2_store), &iter, path))
			{
				gint time = 0;
				gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_TIME, &time, -1);
				if(time != MPD_SONG_NO_TIME)
				{
					info.playlist_playtime -= time;
				}                                                      				
				gtk_list_store_remove (pl2_store, &iter);
			}
			g_free (path);
			old_length--;
		}
	}


	pl3_highlight_song_change ();
	cfg_free_string(string);
	info.playlist_id = new_playlist_id;
	info.playlist_length = mpd_ob_playlist_get_playlist_length(connection);

	pl3_playlist_changed();

}

void init_stock_icons ()
{
	GtkIconFactory *factory;
	GdkPixbuf *pb;
	GtkIconSet *set;
	factory = gtk_icon_factory_new ();

	/*
	 * add media-audiofile 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-audiofile.png",
			NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-audiofile", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-stream 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-stream.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-stream", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-artist 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-artist.svg", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-artist", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-album 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-album.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-album", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-play 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-play.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gtk-media-play", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-stop 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-stop.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gtk-media-stop", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-next 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-next.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gtk-media-next", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-prev 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-prev.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gtk-media-previous", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-pause 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-pause.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gtk-media-pause", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add player-shuffle 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "player-shuffle.png",
			NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-random", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add playerrepeat 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "player-repeat.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-repeat", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add icecast
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "icecast.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb); 
	gtk_icon_factory_add (factory, "icecast", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media playlist
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-playlist.svg", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);                        	
	gtk_icon_factory_add (factory, "media-playlist", set);
	g_object_unref (G_OBJECT (pb));                

	gtk_icon_factory_add_default (factory);
}


void init_playlist ()
{
	/* create initial tree store */
	pl2_store = gtk_list_store_new (NROWS, GTK_TYPE_INT,	/* song id */
			GTK_TYPE_INT,	/* pos id */
			GTK_TYPE_STRING,	/* song title */
			GTK_TYPE_INT,	/* weight int */
			G_TYPE_BOOLEAN,	/* weight color */
			GTK_TYPE_STRING,	/* stock-id */
			GTK_TYPE_INT,
			GTK_TYPE_FLOAT);

}

void song_changed(MpdObj *mi, int oldsong, int newsong)
{
	/* player changed */
	player_song_changed(oldsong, newsong);
	tray_icon_song_change();
	pl3_highlight_song_change();
}

void error_window_destroy(GtkWidget *window)
{
	gtk_widget_destroy(window);
	g_object_unref(xml_error_window);
	xml_error_window = NULL;
}



void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data)
{
	if(error_id == 15 && cfg_get_single_value_as_int_with_default(config, "connection", "autoconnect", 0)) return;
	cfg_set_single_value_as_int(config, "connection", "autoconnect", 0);
	if (xml_error_window == NULL)
	{
		gchar *str = g_strdup_printf("error code %i: %s", error_id, error_msg);
		xml_error_window = glade_xml_new(GLADE_PATH"gmpc.glade", "error_dialog",NULL);
		GtkWidget *dialog = glade_xml_get_widget(xml_error_window, "error_dialog");
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_error_window,"em_label")), str); 
		gtk_widget_show_all(dialog);
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(error_window_destroy), NULL);
		msg_set_base(_("Gnome Music Player Client"));
	}
	else
	{
		gchar *str = g_strdup_printf("error code %i: %s", error_id, error_msg);
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_error_window,"em_label")), str); 

	}
}
void connect_callback()
{
	if(xml_error_window != NULL)
	{
		error_window_destroy(glade_xml_get_widget(xml_error_window, "error_dialog"));
	}
}
void status_callback(MpdObj *mi)
{
	id3_status_update();

}


void state_callback(MpdObj *mi, int old_state, int new_state, gpointer data)
{
	player_state_changed(old_state, new_state);
	tray_icon_state_change();
	pl3_highlight_state_change(old_state,new_state);
	/* make */
}

void database_changed()
{
	pl3_reinitialize_tree();
}

