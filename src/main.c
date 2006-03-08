/*
 * Copyright (C) 2004-2006 Qball Cow <Qball@qballcow.nl>
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
#include <strings.h>
#include <glade/glade.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <config.h>

#include "plugin.h"
#include "main.h"
#include "misc.h"

/* as internall plugin */
#include "playlist3-tag-browser.h"
#include "playlist3-artist-browser.h"

#ifdef ENABLE_MMKEYS
#include "mm-keys.h"
#endif
void init_playlist_store ();
void connection_changed(MpdObj *mi, int connect, gpointer data);
void   GmpcStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);
extern int debug_level;
void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data);
gmpcPlugin **plugins = NULL;
int num_plugins = 0;

/*
 * the xml fle pointer to the player window
 */

GladeXML *xml_error_window = NULL;
GladeXML *xml_password_window = NULL;
int update_interface ();

/*
 * the ID of the update timeout
 */
guint update_timeout = 0;
void init_stock_icons ();
/*
 * The Config object
 */
config_obj *config = NULL;
/*
 * The Connection object
 */
MpdObj *connection = NULL;

char *gmpc_get_full_image_path(char *filename)
{
	gchar *path;
#ifdef WIN32
	gchar *packagedir;
	packagedir = g_win32_get_package_installation_directory("gmpc", NULL);
	debug_printf(DEBUG_INFO, "Got %s as package installation dir", packagedir);

	path = g_build_filename(packagedir, "data", "images", filename, NULL);

	/* From a certain version of GTK+ this g_free will be needed, but for now it will free
	 * a pointer which is returned on further calls to g_win32_get...
	 * This bug is fixed now (30-10-2005), so it will probably be in glib 2.6.7 and/or 2.8.4
	 */
#if CHECK_GLIB_VERSION(2,8,4)
	  g_free(packagedir);
#endif

#else
	path = g_strdup_printf("%s/%s", PIXMAP_PATH, filename);
#endif
	return path;
}

char *gmpc_get_full_glade_path(char *filename)
{
	gchar *path;
#ifdef WIN32
	gchar *packagedir;
	packagedir = g_win32_get_package_installation_directory("gmpc", NULL);
	debug_printf(DEBUG_INFO, "Got %s as package installation dir", packagedir);

	path = g_build_filename(packagedir, "data", "glade", filename, NULL);

	/* From a certain version of GTK+ this g_free will be needed, but for now it will free
	 * a pointer which is returned on further calls to g_win32_get...
	 * This bug is fixed now (30-10-2005), so it will probably be in glib 2.6.7 and/or 2.8.4
	 */
#if CHECK_GLIB_VERSION(2,8,4)
	 g_free(packagedir);
#endif

#else
	path = g_strdup_printf("%s/%s", GLADE_PATH, filename);
#endif
	return path;
}



int main (int argc, char **argv)
{
	int i;
#ifdef ENABLE_MMKEYS
	MmKeys *keys = NULL;
#endif
	gchar *url = NULL;
	/* debug stuff */
	debug_level = DEBUG_ERROR;
	if(argc > 1)
	{
		int i;
		for(i = 1; i< argc; i++)
		{
			if(!strncasecmp(argv[i], "--debug-level=", 14))
			{
				debug_level = atoi(&argv[i][14]);
				debug_level = (debug_level < 0)? -1:((debug_level > DEBUG_INFO)? DEBUG_INFO:debug_level);
			}
			else if (!strncasecmp(argv[i], "--version", 9))
			{
				printf("Gnome Music Player Client\n");
				printf("Version: %s\n", VERSION);
				exit(0);
			}
		}

	}



#ifdef ENABLE_NLS
	debug_printf(DEBUG_INFO, "Setting NLS");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	/* initialize the settings */
	debug_printf(DEBUG_INFO, "loading default values");


	/*
	 * initialize gtk
	 */
	debug_printf(DEBUG_INFO, "Initializing gtk ");
	gtk_init (&argc, &argv);

	/* initialize threading */
	debug_printf(DEBUG_INFO,"Initializing threading");
	qthread_init();

	/*
	 * stock icons
	 */
	debug_printf(DEBUG_INFO, "Loading stock icons");
	init_stock_icons ();

	
	/* Check for and create dir if availible */
	url = g_strdup_printf("%s/.gmpc/", g_get_home_dir());
	debug_printf(DEBUG_INFO, "Checking for %s existence",url);
	if(!g_file_test(url, G_FILE_TEST_EXISTS))
	{
		debug_printf(DEBUG_INFO, "Trying to create %s",url);
		if(mkdir(url,0700) < 0)
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
		debug_printf(DEBUG_INFO, "%s exist and is directory",url);
	}
	g_free(url);

	/* this shows the connection preferences */
	plugin_add(&connection_plug,0);
	/* this the server preferences */
	plugin_add(&server_plug,0);
	/* this shows the playlist preferences */
	plugin_add(&playlist_plug,0);
	/* this shows the markup stuff */
	plugin_add(&tag_plug,0);
#ifdef ENABLE_TRAYICON
	/* the tray icon */
	plugin_add(&tray_icon_plug,0);
#endif
	plugin_add(&cover_art_plug,0);
	/* the about windows :D*/
	plugin_add(&about_plug,0);
	


	url = g_strdup_printf("%s/%s",GLADE_PATH, "plugins");
	plugin_load_dir(url);
	g_free(url);
	/* plugins */
	url = g_strdup_printf("%s/.gmpc/plugins/",g_get_home_dir());
	if(g_file_test(url, G_FILE_TEST_IS_DIR))
	{
		plugin_load_dir(url);
	}
	else
	{
		mkdir(url, 0777);
	}
	g_free(url);

	/* OPEN CONFIG FILE */
	url = g_strdup_printf("%s/.gmpc/gmpc.cfg", g_get_home_dir());
	debug_printf(DEBUG_INFO, "Trying to open the config file: %s", url);
	config = cfg_open(url);


	/* test if config open  */
	if(config == NULL)
	{
		debug_printf(DEBUG_ERROR,"Failed to save/load configuration:\n%s\n",url);
		return 1;
	}

	g_free(url);
	cover_art_init();
	/* Create connection object */
	connection = mpd_new_default();
	if(connection == NULL)
	{
		debug_printf(DEBUG_ERROR,"Failed to create connection obj\n");
		return 1;
	}
	/* New Signal */
	mpd_signal_connect_status_changed(connection, GmpcStatusChangedCallback, NULL);
	mpd_signal_connect_error(connection, error_callback, NULL);
	mpd_signal_connect_connection_changed(connection, connection_changed, NULL);


	/* time todo some initialisation of plugins */
	for(i=0; i< num_plugins && plugins[i] != NULL;i++)
	{
		if(plugins[i]->init)
		{
			plugins[i]->init();
		}
	}

	/*
	 * create the main window, This is done before anything else (but after command line check)
	 */
	player_create ();

	/* create the store for the playlist */
	init_playlist_store ();

	/*
	 * create timeouts 
	 * get the status every 1/2 second should be enough, but it's configurable.
	 */
	gtk_timeout_add (cfg_get_single_value_as_int_with_default(config,
				"connection","mpd-update-speed",500),
			(GSourceFunc)update_mpd_status, NULL);
	update_timeout = gtk_timeout_add (5000,(GSourceFunc)update_interface, NULL);

	/* update the interface */
	while(gtk_events_pending())
	{
		gtk_main_iteration();
	}
	update_interface();

#ifdef ENABLE_MMKEYS
	/*
	 * Keys
	 */
	keys = mmkeys_new();
	g_signal_connect(G_OBJECT(keys), "mm_playpause", G_CALLBACK(play_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_next", G_CALLBACK(next_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_prev", G_CALLBACK(prev_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_stop", G_CALLBACK(stop_song), NULL);
#endif
	/*
	 * run the main loop
	 */
	gtk_main ();
	/*
	 * Main Loop ended
	 */
	player_destroy();
	/* cleaning up. */
	mpd_free(connection);
	cfg_close(config);
	g_object_unref(playlist);
	return 0;
}


void main_quit()
{
	/* so it saves the playlist pos */
	pl3_close();
	mpd_signal_connect_connection_changed(connection, NULL, NULL);
	if(mpd_check_connected(connection))
	{
		mpd_disconnect(connection);
	}

	gtk_main_quit();
}

int update_interface ()
{
	/* check if there is an connection.*/
	if (!mpd_check_connected(connection)){
		 /* update the popup  */
		if (cfg_get_single_value_as_int_with_default(config,
					"connection",
					"autoconnect",
					0))
		{
			connect_to_mpd ();
		}
	}
	/* now start updating the rest */
	return TRUE;
}

void init_stock_icons ()
{
	GtkIconFactory *factory;
	GdkPixbuf *pb;
	GtkIconSet *set;
	char *path;
	factory = gtk_icon_factory_new ();

	/*
	 * add media-audiofile
	 */
	path = gmpc_get_full_image_path("media-audiofile.png");
	pb = gdk_pixbuf_new_from_file (path ,NULL);
	g_free(path);

	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-audiofile", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-stream
	 */
	path = gmpc_get_full_image_path("media-stream.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);

	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-stream", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-artist
	 */
	path = gmpc_get_full_image_path("media-artist.png");
	pb = gdk_pixbuf_new_from_file (path,NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-artist", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add media-album
	 */
	path = gmpc_get_full_image_path("media-album.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-album", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-play
	 */
	if(!gtk_icon_factory_lookup_default("gtk-media-play")) {
		path = gmpc_get_full_image_path("media-play.png");
		pb = gdk_pixbuf_new_from_file (path, NULL);
		g_free(path);
		set = gtk_icon_set_new_from_pixbuf (pb);
		gtk_icon_factory_add (factory, "gtk-media-play", set);
		g_object_unref (G_OBJECT (pb));
	}
	
	/*
	 * add media-stop
	 */
	if(!gtk_icon_factory_lookup_default("gtk-media-stop")) {
		path = gmpc_get_full_image_path("media-stop.png");
		pb = gdk_pixbuf_new_from_file (path,NULL);
		g_free(path);
		set = gtk_icon_set_new_from_pixbuf (pb);
		gtk_icon_factory_add (factory, "gtk-media-stop", set);
		g_object_unref (G_OBJECT (pb));
	}
	/*
	 * add media-next
	 */
	if(!gtk_icon_factory_lookup_default("gtk-media-next")) {
		path = gmpc_get_full_image_path("media-next.png");
		pb = gdk_pixbuf_new_from_file (path,NULL);
		g_free(path);
		set = gtk_icon_set_new_from_pixbuf (pb);
		gtk_icon_factory_add (factory, "gtk-media-next", set);
		g_object_unref (G_OBJECT (pb));
	}
	/*
	 * add media-prev
	 */
	if(!gtk_icon_factory_lookup_default("gtk-media-previous")) {
		path = gmpc_get_full_image_path("media-prev.png");
		pb = gdk_pixbuf_new_from_file (path, NULL);
		g_free(path);
		set = gtk_icon_set_new_from_pixbuf (pb);
		gtk_icon_factory_add (factory, "gtk-media-previous", set);
		g_object_unref (G_OBJECT (pb));
	}
	/*
	 * add media-pause
	 */
	if(!gtk_icon_factory_lookup_default("gtk-media-pause")) {
		path = gmpc_get_full_image_path("media-pause.png");
		pb = gdk_pixbuf_new_from_file (path, NULL);
		g_free(path);
		set = gtk_icon_set_new_from_pixbuf (pb);
		gtk_icon_factory_add (factory, "gtk-media-pause", set);
		g_object_unref (G_OBJECT (pb));
	}
	/*
	 * add player-shuffle
	 */
	path = gmpc_get_full_image_path("player-shuffle.png");
	pb = gdk_pixbuf_new_from_file (path,NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-random", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add playerrepeat
	 */
	path = gmpc_get_full_image_path("player-repeat.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-repeat", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add icecast
	 */
	path = gmpc_get_full_image_path("icecast.png");
	pb = gdk_pixbuf_new_from_file (path,NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "icecast", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media playlist
	 */
	path = gmpc_get_full_image_path("media-playlist.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-playlist", set);
	g_object_unref (G_OBJECT (pb));


	/*
	 * add media playlist
	 */
	path = gmpc_get_full_image_path("gmpc.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gmpc", set);
	g_object_unref (G_OBJECT (pb));


	gtk_icon_factory_add_default (factory);
}


void init_playlist_store ()
{

	playlist = (GtkTreeModel *)playlist_list_new();

	playlist_list_set_markup((CustomList *)playlist,cfg_get_single_value_as_string_with_default(config,
                                       			"playlist","markup", DEFAULT_PLAYLIST_MARKUP));

	
}


void   GmpcStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	int i;
	if(what&MPD_CST_SONGPOS)
	{
		playlist_list_set_current_song_pos(PLAYLIST_LIST(playlist), mpd_player_get_current_song_pos(mi));
	}
	if(what&MPD_CST_SONGID)
	{
		pl3_highlight_song_change();
	}
	if(what&MPD_CST_DATABASE)
	{
		pl3_database_changed();
	//	pl3_artist_browser_dbase_updated();
	}
	if(what&MPD_CST_UPDATING)
	{
		pl3_updating_changed(connection, mpd_status_db_is_updating(connection));
	}
	if(what&MPD_CST_STATE)
	{
		if(mpd_player_get_state(mi) == MPD_STATUS_STATE_STOP){
			playlist_list_set_current_song_pos(PLAYLIST_LIST(playlist), -1);
		}
		else if (mpd_player_get_state(mi) == MPD_STATUS_STATE_PLAY) {
			playlist_list_set_current_song_pos(PLAYLIST_LIST(playlist), mpd_player_get_current_song_pos(mi));
		}
	}
	if(what&MPD_CST_PLAYLIST)
	{
		playlist_list_data_update(PLAYLIST_LIST(playlist),mi);
	}

	/* make the player handle signals */
	player_mpd_state_changed(mi,what,userdata);
	id3_status_update();

	for(i=0; i< num_plugins; i++)
	{
		if(plugins[i]->mpd_status_changed!= NULL)
		{
			plugins[i]->mpd_status_changed(mi,what,NULL);
		}
	}
}

void error_window_destroy(GtkWidget *window,int response, gpointer autoconnect)
{

	gtk_widget_destroy(window);
	g_object_unref(xml_error_window);
	xml_error_window = NULL;
	if(response == GTK_RESPONSE_OK)
	{
		cfg_set_single_value_as_int(config, "connection", "autoconnect", GPOINTER_TO_INT(autoconnect));
		connect_to_mpd();
	}
}

void password_dialog(int failed)
{
	gchar *path  = NULL;
	if(xml_password_window) return;
	path = gmpc_get_full_glade_path("gmpc.glade");
	xml_password_window = glade_xml_new(path, "password-dialog",NULL);
	g_free(path);
	if(!xml_password_window) return;
	if(failed)
	{
		path = g_strdup_printf("Failed to set password on: '%s'\nPlease try again",
				cfg_get_single_value_as_string(config, "connection", "hostname"));
	}
	else
	{
		path = g_strdup_printf("Please enter your password for: '%s'",
				cfg_get_single_value_as_string(config, "connection", "hostname"));

	}
	gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_password_window, "pass_label")),path);
	g_free(path);
	switch(gtk_dialog_run(GTK_DIALOG(glade_xml_get_widget(xml_password_window, "password-dialog"))))
	{
		case GTK_RESPONSE_OK:
			{
				path = (char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_password_window, "pass_entry")));
				mpd_set_password(connection, path);
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_password_window, "ck_save_pass"))))
				{
					cfg_set_single_value_as_int(config, "connection", "useauth", TRUE);
					cfg_set_single_value_as_string(config, "connection", "password", path);
				}
				mpd_send_password(connection);
			}
			break;
		default:
			cfg_set_single_value_as_int(config, "connection", "autoconnect", FALSE);
			mpd_disconnect(connection);
			break;


	}
	gtk_widget_destroy(glade_xml_get_widget(xml_password_window, "password-dialog"));
	g_object_unref(xml_password_window);
	xml_password_window = NULL;
}

void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data)
{
	int autoconnect = cfg_get_single_value_as_int_with_default(config, "connection","autoconnect", DEFAULT_AUTOCONNECT);
	/* if we are not connected we show a reconnect */
	if(!mpd_check_connected(mi))
	{
		/* no response? then we just ignore it when autoconnecting. */
		if(error_id == 15 && autoconnect) return;
		cfg_set_single_value_as_int(config, "connection", "autoconnect", 0);
		if (xml_error_window == NULL)
		{
			gchar *str = g_strdup_printf("error code %i: %s", error_id, error_msg);
			gchar *path = gmpc_get_full_glade_path("gmpc.glade");
			xml_error_window = glade_xml_new(path,"error_dialog",NULL);
			g_free(path);
			GtkWidget *dialog = glade_xml_get_widget(xml_error_window, "error_dialog");
			gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_error_window,"em_label")), str);
			gtk_widget_show_all(dialog);
			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(error_window_destroy), GINT_TO_POINTER(autoconnect));
			msg_set_base(_("Gnome Music Player Client"));
			g_free(str);
		}
		else
		{
			gchar *str = g_strdup_printf("error code %i: %s", error_id, error_msg);
			gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_error_window,"em_label")), str);
			g_free(str);
		}
	}
	else
	{
		if(error_id == MPD_ACK_ERROR_PASSWORD)
		{
			password_dialog(TRUE);
		}
		else if (error_id == MPD_ACK_ERROR_PERMISSION)
		{
			password_dialog(FALSE);
		}
		else {

			GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					_("Mpd Returned the following error:\n<i>\"%s\"</i>"),
					error_msg);

			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show_all(dialog);
		}
	}
}

void connect_callback(MpdObj *mi)
{
	if(xml_error_window != NULL)
	{
		int autocon = cfg_get_single_value_as_int_with_default(config,
				"connection",
				"autoconnect",
				DEFAULT_AUTOCONNECT);
		error_window_destroy(glade_xml_get_widget(xml_error_window, "error_dialog"),0,
				GINT_TO_POINTER(autocon));
	}
	gtk_timeout_remove (update_timeout);
	update_timeout = gtk_timeout_add (400,(GSourceFunc)update_interface, NULL);
}


void connection_changed(MpdObj *mi, int connect, gpointer data)
{
	int i=0;
	player_connection_changed(mi,connect);
	for(i=0; i< num_plugins; i++)
	{
		if(plugins[i]->mpd_connection_changed!= NULL)
		{
			plugins[i]->mpd_connection_changed(mi,connect,NULL);
		}
	}

}

void show_error_message(gchar *string)
{
	GtkWidget *dialog = NULL;
	dialog = gtk_message_dialog_new_with_markup(NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			string);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_widget_show(dialog);
}
