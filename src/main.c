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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

/** Gtk/glib glade stuff */
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <glade/glade.h>

/** auto*'s config file */
#include <config.h>

/** session support */
#include "sm.h"
/* header files */
#include "plugin.h"
#include "main.h"

/* as internall plugin */
#include "playlist3-file-browser.h"
#include "playlist3-find-browser.h"
#include "playlist3-tag-browser.h"
#include "playlist3-artist-browser.h"
#include "playlist3-current-playlist-browser.h"

/**
 * Get revision
 */
#include "revision.h"

#ifdef ENABLE_MMKEYS
#include "mm-keys.h"
#endif


#ifdef DEBUG
/* Testing */
#include <sys/time.h>

struct timeval tv_old = {0,0};
#endif

/**
 * Define some local functions
 */

/** Creating the backend */
static void init_playlist_store ();
/** handle connection changed */
void connection_changed(MpdObj *mi, int connect, gpointer data);
/** Handle status changed */
void   GmpcStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);
/** Error callback */
void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data);

/** init stock icons */
void init_stock_icons ();


/*
 * the xml fle pointer to the player window
 */

GladeXML *xml_error_window = NULL;
GladeXML *xml_password_window = NULL;
static int autoconnect_callback ();

/*
 * the ID of the autoconnect timeout
 */
guint autoconnect_timeout = 0;

/*
 * The Config object
 */
config_obj *config = NULL;
/*
 * The Connection object
 */
MpdObj *connection = NULL;

/**
 * Get's the full path to an image,
 * While this is compile time on linux, windows
 * needs to determine it run-time.
 */
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
#if GLIB_CHECK_VERSION(2,8,4)
	g_free(packagedir);
#endif

#else
	path = g_strdup_printf("%s/%s", PIXMAP_PATH, filename);
#endif
	return path;
}

/** 
 * Get the full path to the glade files.
 * While this is compile time on linux, windows 
 * needs to determine it run-time
 */
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
#if GLIB_CHECK_VERSION(2,8,4)
	g_free(packagedir);
#endif

#else
	path = g_strdup_printf("%s/%s", GLADE_PATH, filename);
#endif
	return path;
}

/**
 * The program :D
 */
int main (int argc, char **argv)
{
	int i;
	char *config_path = NULL;
#ifdef ENABLE_MMKEYS
	MmKeys *keys = NULL;
#endif

	/**
	 * A string used severall times to create a path
	 */
	gchar *url = NULL;
	/** 
	 * Debug level,  DEBUG_ERROR is the default. (only show error messages
	 * FOR RELEASE THIS SHOULD_BE CHANGED TO DEBUG_NO_OUTPUT
	 */
	int db_level = DEBUG_ERROR;
	/* *
	 * Set the debug level
	 */
	debug_set_level(db_level);

	/**
	 * Parse Command line options
	 */
	if(argc > 1)
	{
		int i;
		for(i = 1; i< argc; i++)
		{
			/**
			 * Set debug level, options are 
			 * 0 = No debug
			 * 1 = Error messages
			 * 2 = Error + Warning messages
			 * 3 = All messages
			 */
			if(!strncasecmp(argv[i], "--debug-level=", 14))
			{
				db_level = atoi(&argv[i][14]);
				debug_set_level(db_level);
			}
			/**
			 * Print out version + svn revision
			 */
			else if (!strncasecmp(argv[i], "--version", 9))
			{
				printf("Gnome Music Player Client\n");
				printf("Version: %s\n", VERSION);
				printf("Revision: %s\n",revision);
				exit(0);
			}
			/**
			 * Allow the user to pick another config file 
			 */
			else if (!strncasecmp(argv[i], "--config=", 9))
			{
				config_path = g_strdup(&argv[i][9]);
			}
			/**
			 * Print out help message
			 */
			else if (!strncasecmp(argv[i], "--help",6))
			{
				printf("Gnome Music Player Client\n");
				printf("Options:\n");
				printf("\t--help:\t\t\tThis help message.\n");
				printf("\t--debug-level=<level>\tMake gmpc print out debug information.\n");
				printf("\t\t\t\tLevel:\n");
				printf("\t\t\t\t\t0: No Output\n");
				printf("\t\t\t\t\t1: Error Messages\n");
				printf("\t\t\t\t\t2: Error + Warning Messages\n");
				printf("\t\t\t\t\t3: All messages\n");
				printf("\t--version:\t\tPrint version and svn revision\n");
				printf("\t--config=<file>\t\tSet config file path, default  ~/.gmpc/gmpc.cfg\n");
				exit(0);
			}
		}

	}
	

	/**
	 * Setup NLS
	 */
#ifdef ENABLE_NLS
	debug_printf(DEBUG_INFO, "Setting NLS");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
#endif

	/*
	 * initialize gtk
	 */
	debug_printf(DEBUG_INFO, "Initializing gtk ");
	gtk_init (&argc, &argv);
	
	/**
	 * Setup session support
	 */
#ifdef ENABLE_SM
	smc_connect(argc, argv);
	
#endif	

	/**
	 *  initialize threading 
	 */
	debug_printf(DEBUG_INFO,"Initializing threading");
	/** Check if threading is supported. */	
	if (!g_thread_supported ())
	{
		/** initialize it */
		g_thread_init (NULL);
	}
	else
	{
		/** No Threading availible, show a nice gtk error message and quit */
		show_error_message(_("GMPC requires threading support.\nquiting.."),TRUE);
		abort();
	}
	
	/**
	 * Initialize the new metadata subsystem.
	 * (Will spawn a new thread, so have to be after the init threading 
	 */
	meta_data_init();
	
	/**
	 * stock icons
	 */
	debug_printf(DEBUG_INFO, "Loading stock icons");
	init_stock_icons ();
	


	/**
	 * Create needed directories for mpd.
	 */	

	/** create path */
	url = g_strdup_printf("%s/.gmpc/", g_get_home_dir());
	debug_printf(DEBUG_INFO, "Checking for %s existence",url);

	/**
	 * Check if ~/.gmpc/ exists 
	 * If not try to create it.
	 */
	if(!g_file_test(url, G_FILE_TEST_EXISTS))
	{
		debug_printf(DEBUG_INFO, "Trying to create %s",url);
		if(g_mkdir(url,0700) < 0)
		{
			debug_printf(DEBUG_ERROR, "Failed to create: %s\n", url);
			show_error_message("Failed to create ~/.gmpc/.", TRUE);
			abort();
		}
	}
	/**
	 * if it exists, check if it's a directory 
	 */
	if (!g_file_test(url, G_FILE_TEST_IS_DIR))
	{
		debug_printf(DEBUG_ERROR, "%s isn't a directory.\n", url);
		debug_printf(DEBUG_ERROR, "Quitting.\n");
		show_error_message("~/.gmpc/ isn't a directory.", TRUE);
		abort();
	}
	else
	{
		debug_printf(DEBUG_INFO, "%s exist and is directory",url);
	}
	/* Free the path */
	g_free(url);
	

	/**
	 * Open the config file
	 */	
	/** 
	 * Check if the user has forced a different config file location.
	 * else set to ~/.gmpc/gmpc.cfg
	 */
	if(!config_path)
	{
		url = g_strdup_printf("%s/.gmpc/gmpc.cfg", g_get_home_dir());
	}
	else{
		url = config_path;
	}
	/**
	 * Open it 
	 */
	debug_printf(DEBUG_INFO, "Trying to open the config file: %s", url);
	config = cfg_open(url);

	/** test if config opened correct  */
	if(config == NULL)
	{
		/**
		 * Show gtk error message and quit 
		 */
		debug_printf(DEBUG_ERROR,"Failed to save/load configuration:\n%s\n",url);
		show_error_message(_("Failed to load the configuration system"), TRUE);
		abort();
	}
	/**
	 * cleanup 
	 */
	g_free(url);
	


	
	/**
	 * Create connection object 
	 */
	connection = mpd_new_default();
	if(connection == NULL)
	{
		/**
		 * if failed, print error message
		 */
		debug_printf(DEBUG_ERROR,"Failed to create connection obj\n");
		show_error_message(_("Failed to setup libmpd"), TRUE);
		abort();
	}

	/**
	 * Connect signals to the connection object
	 */
	mpd_signal_connect_status_changed(connection, GmpcStatusChangedCallback, NULL);
	mpd_signal_connect_error(connection, error_callback, NULL);
	mpd_signal_connect_connection_changed(connection, connection_changed, NULL);

	/** 
	 * Add the internall plugins 
	 */
	/** current playlist */
	plugin_add(&current_playlist_plug, 0);
	/** file browser */
	plugin_add(&file_browser_plug, 0);
	/** Artist browser */
	plugin_add(&artist_browser_plug, 0);
	/** File Browser */
	plugin_add(&find_browser_plug, 0);
	/* this shows the connection preferences */
	plugin_add(&connection_plug,0);
	/* this the server preferences */
	plugin_add(&server_plug,0);
	/* this shows the playlist preferences */
	plugin_add(&playlist_plug,0);
	/* The new testing information plugin */
	plugin_add(&info_plugin,0);            	
	/* this shows the markup stuff */
	plugin_add(&tag_plug,0);
#ifdef ENABLE_TRAYICON
	/* the tray icon */
	plugin_add(&tray_icon_plug,0);
#endif

	/** Setup cover art manager, removed for now. it needs a rewrite
	 */
	/*
		plugin_add(&cover_art_plug,0);
	*/
	

	
	/**
	 *  load dynamic plugins 
	 */
	/** Load the global installed plugins */
	url = g_strdup_printf("%s/%s",GLADE_PATH, "plugins");
	plugin_load_dir(url);
	g_free(url);
	/* user space dynamic plugins */
	url = g_strdup_printf("%s/.gmpc/plugins/",g_get_home_dir());
	/**
	 * if dir exists, try to load the plugins.
	 */
	if(g_file_test(url, G_FILE_TEST_IS_DIR))
	{
		plugin_load_dir(url);
	}
	g_free(url);
	

	
	
	/* time todo some initialisation of plugins */
	for(i=0; i< num_plugins && plugins[i] != NULL;i++)
	{
		if(plugins[i]->init)
		{
			plugins[i]->init();
		}
	}
	

	/**
	 * Create the backend store for the current playlist
	 */
	init_playlist_store ();
	

	/**
	 * Create the main window
	 */	
	debug_printf(DEBUG_INFO, "Create main window\n");
	create_playlist3();
	

	/*
	 * create timeouts 
	 * get the status every 1/2 second should be enough, but it's configurable.
	 */
	gtk_timeout_add (cfg_get_single_value_as_int_with_default(config,
				"connection","mpd-update-speed",500),
			(GSourceFunc)update_mpd_status, NULL);
	/**
	 * create the autoconnect timeout, if autoconnect enable, it will check every 5 seconds
	 * if you are still connected, and reconnects you if not.
	 */
	autoconnect_timeout = gtk_timeout_add (5000,(GSourceFunc)autoconnect_callback, NULL);
	/** 
	 * Call this when entering the main loop, so you are connected on startup, not 5 seconds later
	 */
	gtk_init_add((GSourceFunc)autoconnect_callback, NULL);

#ifdef ENABLE_MMKEYS
	/**
	 * Setup Multimedia Keys
	 */
	/**
	 * Create mmkeys object
	 */
	keys = mmkeys_new();
	/**
	 * Connect the wanted key's
	 */
	g_signal_connect(G_OBJECT(keys), "mm_playpause", G_CALLBACK(play_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_next", G_CALLBACK(next_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_prev", G_CALLBACK(prev_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_stop", G_CALLBACK(stop_song), NULL);
	
#endif


	/*
	 * run the main loop
	 */
	gtk_main ();
	
	/**
	 *  cleaning up. 
	 */
	/** 
	 * Destroy the connection object 
	 */
	mpd_free(connection);
	/**
	 * Close the config file
	 */
	cfg_close(config);
	/**
	 * remove (probly allready done) 
	 * the playlist object
	 */
	g_object_unref(playlist);
	return 0;
}

/**
 * Function to quiet the program
 */
void main_quit()
{
	debug_printf(DEBUG_INFO, "Quiting gmpc....");
	/**
	 * Remove the autoconnect timeout,
	 */
	g_source_remove(autoconnect_timeout);
	/**
	 * destroy the current playlist..
	 */
	pl3_current_playlist_destroy();

	/** 
	 * Call the connection changed.
	 * so it saves the playlist pos 
	 */
	mpd_signal_connect_connection_changed(connection, NULL, NULL);

	/**
	 * Disconnect when connected
	 */
	if(mpd_check_connected(connection))
	{
		mpd_disconnect(connection);
	}
	/**
	 * Exit main loop 
	 */
	gtk_main_quit();
}

/**
 * Callback that get's called every 5 seconds, 
 * and trieds to autoconnect
 * (when enabled)
 */
static int autoconnect_callback ()
{
	/* check if there is an connection.*/
	if (!mpd_check_connected(connection)){
		/* update the popup  */
		if (cfg_get_single_value_as_int_with_default(config,
					"connection",
					"autoconnect",
					FALSE))
		{
			connect_to_mpd ();
		}
	}
	/**
	 * keep the timeout running
	 */
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
	 * add player-shuffle
	 */
	path = gmpc_get_full_image_path("player-shuffle.png");
	pb = gdk_pixbuf_new_from_file (path,NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "stock_shuffle", set);
	g_object_unref (G_OBJECT (pb));
	/*
	 * add playerrepeat
	 */
	path = gmpc_get_full_image_path("player-repeat.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "stock_repeat", set);
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
	gtk_window_set_default_icon(pb);
	g_object_unref (G_OBJECT (pb));



	/*
	 * add media playlist
	 */
	path = gmpc_get_full_image_path("gmpc-tray.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gmpc-tray", set);
	g_object_unref (G_OBJECT (pb));

	path = gmpc_get_full_image_path("gmpc-tray-play.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gmpc-tray-play", set);
	g_object_unref (G_OBJECT (pb));

	path = gmpc_get_full_image_path("gmpc-tray-pause.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gmpc-tray-pause", set);
	g_object_unref (G_OBJECT (pb));


	path = gmpc_get_full_image_path("gmpc-tray-disconnected.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "gmpc-tray-disconnected", set);
	g_object_unref (G_OBJECT (pb));

	path = gmpc_get_full_image_path("gmpc-no-cover.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);              	
	gtk_icon_factory_add (factory, "media-no-cover", set);
	g_object_unref (G_OBJECT (pb));


	path = gmpc_get_full_image_path("gmpc-loading-cover.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);              	
	gtk_icon_factory_add (factory, "media-loading-cover", set);  	
	g_object_unref (G_OBJECT (pb));

	path = gmpc_get_full_image_path("stock_volume.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free(path);
	set = gtk_icon_set_new_from_pixbuf (pb);              	
	gtk_icon_factory_add (factory, "gmpc-volume", set);
	g_object_unref (G_OBJECT (pb));



	gtk_icon_factory_add_default (factory);
}

/**
 * Create the "Current" playlist backend.
 * This needs to be created from the start so it keeps in sync.
 */

static void init_playlist_store ()
{
	/**
	 * Create the (custom) playlist widget 
	 */
	playlist = (GtkTreeModel *)playlist_list_new();

	/**
	 * restore the markup
	 */
	playlist_list_set_markup((CustomList *)playlist,cfg_get_single_value_as_string_with_default(config,
				"playlist","markup", DEFAULT_PLAYLIST_MARKUP));
}

/**
 * Handle status changed callback from the libmpd object
 * This mostly involves propegating the signal
 */
void   GmpcStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	int i;
	/**
	 * Propagete to the id3 window, so it can update time
	 */
	id3_status_update();
	/**
	 * Make the plugins recieve the signals 
	 */
	for(i=0; i< num_plugins; i++)
	{
		if(plugins[i]->mpd_status_changed!= NULL)
		{
			plugins[i]->mpd_status_changed(mi,what,NULL);
		}
	}
}


/*******************************
 * Error handling 
 * TODO: Needs to be redone/rethought
 */
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
			if(mpd_server_check_command_allowed(connection, "status") != MPD_SERVER_COMMAND_ALLOWED)
			{
				show_error_message(_("GMPC has insuffient permissions on the mpd server."),
						FALSE);
				cfg_set_single_value_as_int(config, "connection", "autoconnect", FALSE);
				mpd_disconnect(connection);
			}
			break;


	}
	gtk_widget_destroy(glade_xml_get_widget(xml_password_window, "password-dialog"));
	g_object_unref(xml_password_window);
	xml_password_window = NULL;
}

void send_password()
{
	password_dialog(FALSE);
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
}

/**
 * handle a connection changed 
 */
void connection_changed(MpdObj *mi, int connect, gpointer data)
{
	int i=0;
	/**
	 * send password, first thing we do, if connected 
	 */
	if(connect)
	{
		if(cfg_get_single_value_as_int_with_default(config, "connection", "useauth",0))
		{
			mpd_send_password(connection);
		}
	}	

	/**
	 * propegate signals
	 */
	debug_printf(DEBUG_INFO, "Connection changed\n");
	playlist_connection_changed(mi, connect);
#ifdef ENABLE_TRAYICON
	tray_icon_connection_changed(mi, connect);
#endif
	for(i=0; i< num_plugins; i++)
	{
		debug_printf(DEBUG_INFO, "Connection changed plugin: %s\n", plugins[i]->name);
		if(plugins[i]->mpd_connection_changed!= NULL)
		{
			plugins[i]->mpd_connection_changed(mi,connect,NULL);
		}
	}
	/**
	 * force an update of status
	 */
	mpd_status_update(mi);
}


/**
 * Shows an error message.
 * When block enabled, it will run in it's own mainloop.
 */
void show_error_message(gchar *string, int block)
{
	GtkWidget *dialog = NULL;
	dialog = gtk_message_dialog_new_with_markup(NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			string);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
	if(block)
	{
		gtk_dialog_run(GTK_DIALOG(dialog));
	}
	else
	{
		gtk_widget_show(dialog);
	}
}
