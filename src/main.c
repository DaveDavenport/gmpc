/*
 * Copyright (C) 2004-2007 Qball Cow <qball@sarine.nl>
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

#include <curl/curl.h>

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
#include "playlist3-find2-browser.h"
#include "playlist3-find3-browser.h"
#include "playlist3-tag-browser.h"
#include "playlist3-artist-browser.h"
#include "playlist3-current-playlist-browser.h"

#include "id3info.h"

/**
 * Get revision
 */
#include "revision.h"

#ifdef ENABLE_MMKEYS
#include "mm-keys.h"
#endif

/**
 * blub
 */
#ifndef WIN32
#include "bacon-message-connection.h"
static BaconMessageConnection *bacon_connection = NULL;
#endif

#ifdef DEBUG
/* Testing */
#include <sys/time.h>

struct timeval tv_old = {0,0};
#endif

GmpcConnection *gmpcconn = NULL;
int gmpc_connected = FALSE;
int gmpc_failed_tries = 0;


/**
 * Define some local functions
 */
/**
 * Error dialog 
 */
static GtkWidget *error_dialog = NULL;
static GtkListStore *error_list_store = NULL;

/** Creating the backend */
static void init_playlist_store(void);
/** handle connection changed */
static void connection_changed(MpdObj *mi, int connect, gpointer data);


/** Error callback */
static void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data);

/** init stock icons */
static void init_stock_icons(void);

/*
 * the xml fle pointer to the player window
 */

GladeXML *xml_error_window = NULL;
GladeXML *xml_password_window = NULL;
static int autoconnect_callback (void);

/*
 * the ID of the autoconnect timeout
 */
guint autoconnect_timeout = 0;

/*
 * The Config object
 */
config_obj *config = NULL;
config_obj *profiles = NULL;

/*
 * The Connection object
 */
MpdObj *connection = NULL;

void connect_callback(MpdObj *mi);

/* Glade prototypes, these would be static otherwise */
void send_password(void);

/**
 * Set paths
 */
static void create_gmpc_paths(void);


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
     * This bug is fixed now (30-10-2007), so it will probably be in glib 2.6.7 and/or 2.8.4
     */
#if GLIB_CHECK_VERSION(2,8,4)
    q_free(packagedir);
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
     * This bug is fixed now (30-10-2007), so it will probably be in glib 2.6.7 and/or 2.8.4
     */
#if GLIB_CHECK_VERSION(2,8,4)
    q_free(packagedir);
#endif

#else
    path = g_strdup_printf("%s/%s", GLADE_PATH, filename);
#endif
    return path;
}
#ifndef WIN32
static void bacon_on_message_received(const char *message, gpointer data)
{
    pl3_show_window();
}
#endif

int main (int argc, char **argv)
{
    int i;
    int clean_config = FALSE;
    char *config_path = NULL;
#ifdef ENABLE_MMKEYS
    MmKeys *keys = NULL;
#endif

    /**
     * A string used severall times to create a path
     */
    gchar *url = NULL;

    /* *
     * Set the debug level
     */
	if(revision && revision[0] != '\0') {
		/* We run a svn version, so we want more default debug output */
		debug_set_level(DEBUG_ERROR);
	} else {
		/* Ok, release version... no debug */
		debug_set_level(0);
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
            if(!strncasecmp(argv[i], _("--debug-level="),strlen(_("--debug-level="))))
            {
                int db_level = atoi(&argv[i][strlen(_("--debug-level="))]);
                debug_set_level(db_level);
            }
            /**
             * Print out version + svn revision
             */
            else if (!strcasecmp(argv[i], _("--version")))
            {
                printf(("Gnome Music Player Client\n"));
                printf(_("Version: %s\n"), VERSION);
                if(revision && revision[0] != '\0')
                {
                    printf(_("Revision: %s\n"),revision);
                }
                exit(0);
            }
            /**
             * Allow the user to pick another config file 
             */
            else if (!strncasecmp(argv[i], _("--config="), strlen(_("--config="))))
            {
                config_path = g_strdup(&argv[i][strlen(_("--config="))]);
            }
            else if (!strncasecmp(argv[i], _("--clean-cover-db"), strlen(_("--clean-cover-db"))))
            {
                clean_config = TRUE;
            }
            /**
             * Print out help message
             */
            else if (!strncasecmp(argv[i], _("--help"),strlen(_("--help"))))
            {
                printf(_("Gnome Music Player Client\n"\
                            "Options:\n"\
                            "\t--help:\t\t\tThis help message.\n"\
                            "\t--debug-level=<level>\tMake gmpc print out debug information.\n"\
                            "\t\t\t\tLevel:\n"\
                            "\t\t\t\t\t0: No Output\n"\
                            "\t\t\t\t\t1: Error Messages\n"\
                            "\t\t\t\t\t2: Error + Warning Messages\n"\
                            "\t\t\t\t\t3: All messages\n"\
                            "\t--version:\t\tPrint version and svn revision\n"\
                            "\t--config=<file>\t\tSet config file path, default  ~/.gmpc/gmpc.cfg\n"\
                            "\t--clean-cover-db\tCleanup the cover file.\n"
                        ));
                exit(0);
            }
        }

    }

    /*
     * initialize gtk
     */
    debug_printf(DEBUG_INFO, "Initializing gtk ");
    gtk_init (&argc, &argv);
    /** 
     * Check libmpd version runtime
     */
    if(strcmp(libmpd_version, LIBMPD_VERSION))
    {
        debug_printf(DEBUG_ERROR, "Trying to run gmpc compiled against libmpd version '%s' with version '%s'", 
                LIBMPD_VERSION, libmpd_version);	

        show_error_message(_("Trying to run gmpc with a wrong libmpd version."), TRUE);
        exit(1);
    }
    /**
     *  initialize threading 
     */
    debug_printf(DEBUG_INFO,"Initializing threading");


    /** Init before threads are active.. */
    debug_printf(DEBUG_INFO, "Initialize curl_global_init");
    {
        CURLcode result;
        /**
         * Only init the CURL_GLOBAL_WIN32 (should only do something on win32 anyway
         * Because I don't want to load the ssl part.. (that costs me 0.5mb extra memory)
         */
        if((result = curl_global_init(CURL_GLOBAL_WIN32)))
        {
            debug_printf(DEBUG_ERROR, "cURL Global init failed: %d\n", result);
            exit(1);
        }	

    }	
    /** Check if threading is supported. */	
    /** initialize it */
    if(!g_thread_supported())g_thread_init (NULL);

    create_gmpc_paths();

    /* do the clean config stuff */
    if(clean_config)
    {
        meta_data_init();
        printf("Cleaning up cover file..\n");
        meta_data_cleanup();
        printf("Done..\n");
        meta_data_destroy();
        return 1;
    }

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
    q_free(url);
    /**
     * Profile file
     */
    url = cfg_get_single_value_as_string(config, "connection", "profile-file");
    if(!url)
    {
        url = g_strdup_printf("%s/.gmpc/profiles.cfg", g_get_home_dir());
        cfg_set_single_value_as_string(config, "connection", "profile-file",url);
    }
    profiles = cfg_open(url);
    if(profiles == NULL)
    {
        /**
         * Show gtk error message and quit 
         */
        debug_printf(DEBUG_ERROR,"Failed to save/load Profile file:\n%s\n",url);
        show_error_message(_("Failed to load the configuration system"), TRUE);
        abort();
    }
    q_free(url);


#ifndef WIN32

    if(cfg_get_single_value_as_int_with_default(config, "Default", "allow-multiple",FALSE) == FALSE)
    {
        /**
         * bacon here we come 
         */
        bacon_connection = bacon_message_connection_new("gmpc");
        if(bacon_connection != NULL)
        {
            if (!bacon_message_connection_get_is_server (bacon_connection)) 
            {
                debug_printf(DEBUG_WARNING, "gmpc is allready running\n");
                bacon_message_connection_send(bacon_connection, "PRESENT");
                exit(0);
            }
            bacon_message_connection_set_callback (bacon_connection,
                    bacon_on_message_received,
                    NULL);
        }
    }


#endif		
    /**
     * Setup session support
     */
#ifdef ENABLE_SM
    smc_connect(argc, argv);
#endif	
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
     * Just some trick to provide glib signals
     */
    gmpcconn = (GmpcConnection *)gmpc_connection_new();

    /** 
     * Add the internall plugins 
     */
    /** current playlist */
    plugin_add(&current_playlist_plug, 0);
    plugin_add(&find3_browser_plug, 0);
    /** file browser */
    plugin_add(&file_browser_plug, 0);
    /** Artist browser */
    plugin_add(&artist_browser_plug, 0);
    /** File Browser */
    plugin_add(&find2_browser_plug, 0);
    /* this shows the connection preferences */
    plugin_add(&connection_plug,0);
    /* this the server preferences */
    plugin_add(&server_plug,0);
    /* this shows the playlist preferences */
    plugin_add(&playlist_plug,0);
    /* this shows the markup stuff */
    plugin_add(&tag_plug,0);
#ifdef ENABLE_MMKEYS
    plugin_add(&mmkeys_plug,0);
#endif
    plugin_add(&url_plugin,0);
    /* the tray icon */
    plugin_add(&tray_icon_plug,0);
    /* Info3 data browser */
    plugin_add(&info3_plugin,0);	
    /* Meta data browser */
    plugin_add(&metab_plugin,0);	



    /**
     *  load dynamic plugins 
     */
    /** Load the global installed plugins */
    url = g_strdup_printf("%s/%s",GLADE_PATH, "plugins");
    plugin_load_dir(url);
    q_free(url);
    /* user space dynamic plugins */
    url = g_strdup_printf("%s/.gmpc/plugins/",g_get_home_dir());
    /**
     * if dir exists, try to load the plugins.
     */
    if(g_file_test(url, G_FILE_TEST_IS_DIR))
    {
        plugin_load_dir(url);
    }
    q_free(url);


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
     * Ask user about added/removed provider plugins 
     */
    meta_data_check_plugin_changed();
    /**
     * Create the main window
     */	
    debug_printf(DEBUG_INFO, "Create main window\n");
    create_playlist3();
    /** 
     * If autoconnect is enabled, tell gmpc that it's in state it should connect
     */	
    if(cfg_get_single_value_as_int_with_default(config, "connection","autoconnect", DEFAULT_AUTOCONNECT))
    {
        gmpc_connected = TRUE;
    }
    /*
     * create timeouts 
     * get the status every 1/2 second should be enough, but it's configurable.
     */
    g_timeout_add (cfg_get_single_value_as_int_with_default(config,
                "connection","mpd-update-speed",500),
            (GSourceFunc)update_mpd_status, NULL);
    /**
     * create the autoconnect timeout, if autoconnect enable, it will check every 5 seconds
     * if you are still connected, and reconnects you if not.
     */
    autoconnect_timeout = g_timeout_add (5000,(GSourceFunc)autoconnect_callback, NULL);
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
    g_signal_connect(G_OBJECT(keys), "mm_fastforward", G_CALLBACK(song_fastforward), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_fastbackward", G_CALLBACK(song_fastbackward), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_repeat", G_CALLBACK(repeat_toggle), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_random", G_CALLBACK(random_toggle), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_raise", G_CALLBACK(pl3_show_window), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_hide", G_CALLBACK(pl3_hide), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_toggle_hidden", G_CALLBACK(pl3_toggle_hidden), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_volume_up", G_CALLBACK(volume_up), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_volume_down", G_CALLBACK(volume_down), NULL);
    g_signal_connect(G_OBJECT(keys), "mm_show_notification", G_CALLBACK(tray_notify_popup), NULL );


#endif


    /*
     * run the main loop
     */
    gtk_main ();

    /**
     *  cleaning up. 
     */
#ifndef WIN32
    if(bacon_connection)
    {
        bacon_message_connection_free (bacon_connection);
    }
#endif	
    /** 
     * Destroy the connection object 
     */
    mpd_free(connection);
    /**
     * Close the config file
     */
    cfg_close(config);
    cfg_close(profiles);
    /**
     * Clear metadata struct
     */
    meta_data_destroy();

    /* time todo some initialisation of plugins */
    for(i=0; i< num_plugins && plugins[i] != NULL;i++)
    {
        if(plugins[i]->destroy)
        {
            plugins[i]->destroy();
        }
    }
    /**
     * remove (probly allready done) 
     * the playlist object
     */
    g_object_unref(playlist);

    curl_global_cleanup();
    return 0;
}

/**
 * Function to quiet the program
 */
void main_quit()
{
    debug_printf(DEBUG_INFO, "Quiting gmpc....");
    /**
     * close playlist and store size
     */
    pl3_hide();
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
static int autoconnect_callback(void)
{
    /* check if there is an connection.*/
    if (!mpd_check_connected(connection)){
        /* update the popup  */
        /*
         * connect when autoconnect is enabled, the user wants to be connected, and it hasn't failed 3 times 
         */
        if (gmpc_failed_tries <  cfg_get_single_value_as_int_with_default(config, "connection","number-of-retries", 3) 
            && gmpc_connected && cfg_get_single_value_as_int_with_default(config,"connection","autoconnect",FALSE))
        {
            /** updated failed time, if it doesn't fail it will be set to 0
             * later 
             */
            gmpc_failed_tries++;
            connect_to_mpd ();

        }
    }
    /**
     * keep the timeout running
     */
    return TRUE;
}

static void init_stock_icons()
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
    q_free(path);

    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "media-audiofile", set);
    g_object_unref (G_OBJECT (pb));
    /*
     * add media-stream
     */
    path = gmpc_get_full_image_path("media-stream.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);

    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "media-stream", set);
    g_object_unref (G_OBJECT (pb));
    /*
     * add media-artist
     */
    path = gmpc_get_full_image_path("media-artist.png");
    pb = gdk_pixbuf_new_from_file (path,NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "media-artist", set);
    g_object_unref (G_OBJECT (pb));
    /*
     * add media-album
     */
    path = gmpc_get_full_image_path("media-album.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "media-album", set);
    g_object_unref (G_OBJECT (pb));

    /*
     * add player-shuffle
     */
    path = gmpc_get_full_image_path("player-shuffle.png");
    pb = gdk_pixbuf_new_from_file (path,NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "stock_shuffle", set);
    g_object_unref (G_OBJECT (pb));
    /*
     * add player-repeat
     */
    path = gmpc_get_full_image_path("player-repeat.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "stock_repeat", set);
    g_object_unref (G_OBJECT (pb));

    /*
     * add media playlist
     */
    path = gmpc_get_full_image_path("media-playlist.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "media-playlist", set);
    g_object_unref (G_OBJECT (pb));


    /*
     * add media playlist
     */
    path = gmpc_get_full_image_path("gmpc.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "gmpc", set);
    gtk_window_set_default_icon(pb);
    g_object_unref (G_OBJECT (pb));

    /*
     * add media playlist
     */
    path = gmpc_get_full_image_path("gmpc-tray.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "gmpc-tray", set);
    g_object_unref (G_OBJECT (pb));

    path = gmpc_get_full_image_path("gmpc-tray-play.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "gmpc-tray-play", set);
    g_object_unref (G_OBJECT (pb));

    path = gmpc_get_full_image_path("gmpc-tray-pause.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "gmpc-tray-pause", set);
    g_object_unref (G_OBJECT (pb));


    path = gmpc_get_full_image_path("gmpc-tray-disconnected.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);
    gtk_icon_factory_add (factory, "gmpc-tray-disconnected", set);
    g_object_unref (G_OBJECT (pb));

    path = gmpc_get_full_image_path("gmpc-no-cover.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);              	
    gtk_icon_factory_add (factory, "media-no-cover", set);
    g_object_unref (G_OBJECT (pb));


    path = gmpc_get_full_image_path("gmpc-loading-cover.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);              	
    gtk_icon_factory_add (factory, "media-loading-cover", set);  	
    g_object_unref (G_OBJECT (pb));

    path = gmpc_get_full_image_path("stock_volume.png");
    pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);              	
    gtk_icon_factory_add (factory, "gmpc-volume", set);
    g_object_unref (G_OBJECT (pb));

	path = gmpc_get_full_image_path("gmpc-add-url.png");
	pb = gdk_pixbuf_new_from_file (path, NULL);
    q_free(path);
    set = gtk_icon_set_new_from_pixbuf (pb);              	
    gtk_icon_factory_add (factory, "gmpc-add-url", set);
    g_object_unref (G_OBJECT (pb));

    gtk_icon_factory_add_default (factory);
}

/**
 * Create the "Current" playlist backend.
 * This needs to be created from the start so it keeps in sync.
 */

static void init_playlist_store ()
{
    gchar *markup = cfg_get_single_value_as_string_with_default(config,"playlist","markup", DEFAULT_PLAYLIST_MARKUP);
    /**
     * Create the (custom) playlist widget 
     */
    playlist = (GtkTreeModel *)playlist_list_new();

    /**
     * restore the markup
     */
    playlist_list_set_markup((CustomList *)playlist,markup);
    q_free(markup);
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
    gmpc_connection_status_changed(gmpcconn, mi, what);
}


/*******************************
 * Error handling 
 * TODO: Needs to be redone/rethought
 */
static void error_window_destroy(GtkWidget *window,int response, gpointer autoconnect)
{
    gtk_widget_destroy(window);
    g_object_unref(xml_error_window);
    xml_error_window = NULL;
    if(response == GTK_RESPONSE_OK)
    {
        connect_to_mpd();
    }
}

static void password_dialog(int failed)
{
    gchar *path  = NULL;
    if(xml_password_window) return;
    path = gmpc_get_full_glade_path("gmpc.glade");
    xml_password_window = glade_xml_new(path, "password-dialog",NULL);
    q_free(path);
    if(!xml_password_window) return;
    if(failed)
    {
        path = g_strdup_printf(_("Failed to set password on: '%s'\nPlease try again"),mpd_get_hostname(connection));
    }
    else
    {
        path = g_strdup_printf(_("Please enter your password for: '%s'"),mpd_get_hostname(connection));
    }
    gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_password_window, "pass_label")),path);
    q_free(path);
    switch(gtk_dialog_run(GTK_DIALOG(glade_xml_get_widget(xml_password_window, "password-dialog"))))
    {
        case GTK_RESPONSE_OK:
            {
                path = (char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_password_window, "pass_entry")));
                mpd_set_password(connection, path);
                if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_password_window, "ck_save_pass"))))
                {
                    connection_set_password(path);				
                }
                mpd_send_password(connection);
            }
            break;
        default:
            if(mpd_server_check_command_allowed(connection, "status") != MPD_SERVER_COMMAND_ALLOWED)
            {
                show_error_message(_("GMPC has insuffient permissions on the mpd server."),FALSE);
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
static void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data)
{
    int autoconnect = cfg_get_single_value_as_int_with_default(config, "connection","autoconnect", DEFAULT_AUTOCONNECT);
    /* if we are not connected we show a reconnect */
    if(!mpd_check_connected(mi))
    {
        /* no response? then we just ignore it when autoconnecting. */
        if(error_id == 15 && autoconnect) return;
        if (xml_error_window == NULL)
        {
            gchar *str = g_strdup_printf(_("error code %i: %s"), error_id, error_msg);
            gchar *path = gmpc_get_full_glade_path("gmpc.glade");
            xml_error_window = glade_xml_new(path,"error_dialog",NULL);
            q_free(path);
            GtkWidget *dialog = glade_xml_get_widget(xml_error_window, "error_dialog");
            gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_error_window,"em_label")), str);
            gtk_widget_show_all(dialog);
            g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(error_window_destroy), GINT_TO_POINTER(autoconnect));
            q_free(str);
        }
        else
        {
            gchar *str = g_strdup_printf(_("error code %i: %s"), error_id, error_msg);
            gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_error_window,"em_label")), str);
            q_free(str);
        }
    }
    else
    {
		if(error_id == MPD_ACK_ERROR_NO_EXIST)
		{
			/* quick hack so a small playlist out of sync is handled nicer */
			return;
		}
        else if(error_id == MPD_ACK_ERROR_PASSWORD)
        {
            password_dialog(TRUE);
        }
        else if (error_id == MPD_ACK_ERROR_PERMISSION)
        {
            password_dialog(FALSE);
        }
        else {
		gchar *str = g_strdup_printf(_("The following error occured: %i:'%s'"), error_id, error_msg);
		show_error_message(str, FALSE);
		q_free(str);
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
static void connection_changed(MpdObj *mi, int connect, gpointer data)
{
    int i=0;
    /**
     * send password, first thing we do, if connected 
     */
    if(connect)
    {
        /* set failed to 0 */
        gmpc_failed_tries = 0;		
        if(connection_use_auth())
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
    gmpc_connection_connection_changed(gmpcconn, mi, connect);
    /**
     * force an update of status
     */
    mpd_status_update(mi);


    if(connect && cfg_get_single_value_as_int_with_default(config, "connection", "warning", TRUE) &&
            mpd_check_connected(connection))
    {
        if(!mpd_server_check_version(connection, 0,12,0)) {
            GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_WARNING,GTK_BUTTONS_CLOSE,
                    _("Gmpc is currently connected to mpd version lower then 0.12.0.\nThis might work, but is no longer supported."));
            g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
            gtk_widget_show(GTK_WIDGET(dialog));
        }
    }
}


/**
 * Shows an error message.
 * When block enabled, it will run in it's own mainloop.
 */
static void error_message_destroy(void)
{
    gtk_widget_destroy(error_dialog);
    error_dialog = NULL;
    gtk_list_store_clear(error_list_store); 
}
void show_error_message(gchar *string, int block)
{
    GtkTreeIter iter;
    GtkWidget *label = NULL;
    if(!error_dialog)
    {    
        GtkWidget *hbox = NULL, *image;
        GtkWidget *vbox = NULL,*sw = NULL, *tree = NULL;
        GtkCellRenderer *renderer;
        /* create dialog */
        error_dialog = gtk_dialog_new_with_buttons(
                _("Error occured during operation"),
                NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_STOCK_CLOSE,
                GTK_RESPONSE_OK,
                NULL);
        /** create list store */
        if(!error_list_store)
        {
            error_list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
            /* don't want this destroyed */
            g_object_ref(error_list_store);
        }
        hbox = gtk_hbox_new(FALSE,6);
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(error_dialog)->vbox), hbox);       
        gtk_container_set_border_width(GTK_CONTAINER(hbox),9);

        /* Error image */
        image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
        gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 0);

        vbox = gtk_vbox_new(FALSE,6); 
        gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

        /* Create label */
        label = gtk_label_new(_("The following error(s) occured:"));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
        /** Create tree view */
        /* sw*/
        sw = gtk_scrolled_window_new(NULL,NULL);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
        /* tree */
        tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(error_list_store));
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
        gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
        /* add tree to sw */
        gtk_container_add(GTK_CONTAINER(sw), tree); 
        /** add cell renderers */
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, _("Error Message"), renderer, "text", 0, NULL);


        /** add sw to vbox */
        gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    }
    gtk_list_store_append(error_list_store, &iter);
    gtk_list_store_set(error_list_store, &iter, 0, string,-1);
    g_signal_connect(G_OBJECT(error_dialog), "response", G_CALLBACK(error_message_destroy), NULL);
    gtk_widget_show_all(error_dialog);
    if(block)
    {
        gtk_dialog_run(GTK_DIALOG(error_dialog));
    }
    else
    {
        gtk_widget_show(error_dialog);
    }

}


static void create_gmpc_paths(void)
{
    /**
     * Create needed directories for mpd.
     */	

    /** create path */
    gchar *url = g_strdup_printf("%s/.gmpc/", g_get_home_dir());
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
    q_free(url);
}
