/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

#include <libxml/parser.h>

/** Gtk/glib glade stuff */
#include <gtk/gtk.h>

#include <libmpd/debug_printf.h>
/* header files */
#include "main.h"
#include "playlist3.h"

#include "misc.h"
#include "advanced-search.h"
#include "gmpc_easy_download.h"

#include "setup-assistant.h"

#include "gmpc-mpddata-model-playlist.h"
#include "bug-information.h"

#include "pixbuf-cache.h"
#include "options.h"
#include "preferences.h"

#include "plugin-man.h"
#include "browsers/playlist3-playlist-editor.h"

#ifdef ENABLE_MMKEYS
#include "mm-keys.h"
#endif

#define LOG_DOMAIN "Gmpc"
/**
 * Get revision
 */
#include "revision.h"
#include "ipc.h"

#include "internal-plugins.h"
#include "log.h"
#include "mpd-easy-commands.h"

/**
 * Global objects that give signals
 */
/* gives signal on connection changes, and state changes of mpd.*/
GmpcConnection *gmpcconn = NULL;
/* Implements, and gives signals on profiles */
GmpcProfiles *gmpc_profiles = NULL;
/* Implements, and gives signals on meta_data*/
GmpcMetaWatcher *gmw = NULL;
/* Easy command */
GmpcEasyCommand *gmpc_easy_command = NULL;
/* Playlist3 messages */
Playlist3MessagePlugin *pl3_messages = NULL;

/* The playlist backend */
GtkTreeModel *playlist = NULL;

GObject *paned_size_group = NULL;
/**
 * This flag indicate the requested connection state by the user.
 * If the user presses disconnect,  you don't want to auto-connect anymore.
 **/
int gmpc_connected = FALSE;

static void connection_changed_real(
    GmpcConnection * gmpcconn,
    MpdObj * mi,
    int connect,
    gpointer data);

static void gmpc_status_changed_callback_real(
	GmpcConnection * gmpcconn,
    MpdObj * mi,
    ChangedStatusType what,
    gpointer data);

/**
 * Define some local functions
 */

static void gmpc_easy_command_set_default_entries(void);
static void  gmpc_mmkeys_connect_signals(GObject *keys);
/** handle connection changed */
static void connection_changed(MpdObj * mi, int connect, gpointer data);

/** Error callback */
static int error_callback(MpdObj * mi,
	int error_id,
	char *error_msg,
	gpointer data);

/** init stock icons */
static void init_stock_icons(void);

/*
 * the xml fle pointer to the player window
 */
static GtkBuilder *xml_password_window = NULL;
static int autoconnect_callback(void);

/*
 * the ID of the autoconnect timeout callback
 */
static guint autoconnect_timeout = 0;

/*
 * The Config object
 */
config_obj *config = NULL;

/*
 * The Connection object
 */
MpdObj *connection = NULL;

/* Glade prototypes, these would be static otherwise */
void send_password(void);

static void print_version(void);


/**
 * Forward libxml errors into GLib.log errors with LibXML error domain
 */
static void xml_error_func(void *ctx, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    g_logv("LibXML", G_LOG_LEVEL_DEBUG, msg, ap);
    va_end(ap);
}
/* \todo why is this here? */
static xmlGenericErrorFunc handler = (xmlGenericErrorFunc) xml_error_func;


static gboolean hide_on_start(void)
{
    pl3_hide();
    return FALSE;
}


int main(int argc, char **argv)
{
    #ifdef WIN32
    gchar *packagedir = NULL;
    #endif
    #ifdef ENABLE_MMKEYS
    MmKeys *keys = NULL;
    #endif
    #ifdef HAVE_IPC
    GObject *ipc = NULL;
    #endif

    /* A string used severall times to create a path  */
    gchar *url = NULL;

    INIT_TIC_TAC();

	log_init();

    egg_sm_client_set_mode(EGG_SM_CLIENT_MODE_NO_RESTART);

    /**
     * Setup NLS
     */
    #ifdef ENABLE_NLS
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Setting NLS");
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    #endif

    gtk_set_locale();

    TEC("Setting up locale");

    parse_options(&argc, &argv);

    /* Show the version, if requested */
    if (settings.show_version)
    {
        print_version();
        return EXIT_SUCCESS;
    }
    TEC("Parsing command line options");

	log_set_debug_level(settings.debug_level);
	TEC("Set debug level")
    /* Show the bug-information dialog */
    if (settings.show_bug_information)
    {
        bug_information_file_new(stdout);
        return EXIT_SUCCESS;
    }


    /**
     * Init libxml.
     * Libxml is not used (directly) by gmpc.
     * But via glade and several plugins use it.
     * I need to initialize it before the threading is started.
     * So moved to gmpc.
     *
     * This fixes the plugin crasher bug on windows.
     */
    xmlInitParser();
    initGenericErrorDefaultFunc(&handler);

    /**
     * Check if threading is supported, if so, start it.
     * Don't fail here, stuff like can cause that it is allready initialized.
     */
    if (!g_thread_supported())
        g_thread_init(NULL);

    TEC("Initializing threading");
    /*
     * initialize gtk
     */
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Initializing gtk ");

    #ifdef WIN32
    /**
     * This loads an extra gtk rc file on windows.
     * This is used to re-enable rule-hint in the treeview.
	 * (this is forced off on windows).
     */
    packagedir = g_win32_get_package_installation_directory_of_module(NULL);
    url = g_build_filename(packagedir,
			"share", "gmpc",
			"gmpc-gtk-win32.rc", NULL);
    q_free(packagedir);
    gtk_rc_add_default_file(url);
    g_free(url);
    #endif

    /* initialize gtk */
    gtk_init(&argc, &argv);
    TEC("Gtk init");

	/* Hack to override the icon theme, on recursive zeltak request */
	if(settings.icon_theme != NULL) {
		gtk_settings_set_string_property(gtk_settings_get_default(),
				"gtk-icon-theme-name" , settings.icon_theme,NULL);
	}
    /* connect signal to Session manager to quit */
    g_signal_connect(
		egg_sm_client_get(), "quit",
		G_CALLBACK(main_quit), NULL);
    TEC("EggSmClient");

    /**
     * Call create_gmpc_paths();
     * This function checks if the path needed path are available
	 * and creates them if needed.
     */
    create_gmpc_paths();
    TEC("Check version and create paths");

    /**
     * COMMANDLINE_OPTION:
     * Cleanup the metadata database and quit.
     */
    if (settings.clean_config)
    {
        /* start the metadata system */
        meta_data_init();
        //printf("Cleaning up cover file..\n");
        /* Call the cleanup */
        //metadata_cache_cleanup();
        printf("Done..\n");
        /* Destroy the meta data system and exit. */
        meta_data_destroy();
        TEC("Database cleanup");
        return EXIT_SUCCESS;
    }

    /**
     * Open the config file
     */
    /**
     * Check if the user has forced a different config file location.
     * else set to ~/.gmpc/gmpc.cfg
     */
    if (!settings.config_path)
    {
        url = gmpc_get_user_path("gmpc.cfg");
    } else
    {
        url = g_strdup(settings.config_path);
    }

    /**
     * Open it
     */
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			"Trying to open the config file: %s", url);
    config = cfg_open(url);

   /**
    * Show gtk error message and quit if config failed to open.
    */
    if (config == NULL)
    {
        g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR,
			"Failed to save/load configuration:\n%s\n", url);
        show_error_message(_("Failed to load the configuration system."));
		return EXIT_FAILURE;
    }
    TEC("Opening config file: %s", url);
    q_free(url);

    /**
     * \TODO, Check if version changed, then say something about it
     *
     * Enable this function if we need todo some upgrading on version change.
     * Removal of this current content destroys config conversion from 0.17 and
     * up
     */
    url = cfg_get_single_value_as_string(config, "Default", "version");
    if (url == NULL || strcmp(url, VERSION))
    {
        cfg_set_single_value_as_string(config, "Default", "version", VERSION);
    }
    if (url) q_free(url);
    TEC("New version check");


    #ifdef HAVE_IPC
    if (cfg_get_single_value_as_int_with_default(config,
        "Default",
        "allow-multiple",
        FALSE) == FALSE)
    {
        ipc = gmpc_tools_ipc_new();
        if(gmpc_tools_ipc_is_running(ipc))
        {
            if(settings.quit)
                gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND,"quit");
            else
                gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND,"show");

            cfg_close(config);
            config = NULL;
            TEC("IPC setup and quitting");
            return EXIT_SUCCESS;
        }
    }
    #endif
    if (settings.quit)
    {
        cfg_close(config);
        return EXIT_SUCCESS;
    }

	/* Easy command */
    gmpc_easy_command = gmpc_easy_command_new();
    TEC("Init easy command")
	gmpc_easy_command_set_default_entries();
	TEC("Set easy commands")
    mpd_easy_commands_init();
    TEC("Set MPD Easy commands");
	/* Advanced search */
    advanced_search_init();
    TEC("Init advanced search");

    /* PanedSizeGroup */
    paned_size_group = (GObject *) gmpc_paned_size_group_new();

    gmpc_profiles = gmpc_profiles_new();
    /* If user requested a profile, look it up and set it active */
    if (settings.profile_name)
    {
       gmpc_profiles_set_profile_from_name(gmpc_profiles,
   											settings.profile_name);
    }
    TEC("Setting up gmpc idle,signals and profiles");
    /**
     * Initialize the new metadata subsystem.
     */
    meta_data_init();

    TEC("Initializing metadata system");
    pixbuf_cache_create();
    TEC("Pixbuf cache create()");

    /**
     * stock icons
     */
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Loading stock icons");
    init_stock_icons();
    TEC("Init stock icons");
    /**
     * Create connection object
     */
    connection = mpd_new_default();
    if (connection == NULL)
    {
        /**
         * if failed, print error message
         */
        g_log(LOG_DOMAIN,
			 G_LOG_LEVEL_ERROR,
			 "Failed to create connection object\n");
        show_error_message(_("Failed to setup libmpd"));
        abort();
    }
    TEC("Setting up mpd connection object");
    /**
     * Connect signals to the connection object
     */
    mpd_signal_connect_status_changed(connection,
			GmpcStatusChangedCallback, NULL);
    mpd_signal_connect_error(connection,
			error_callback, NULL);
    mpd_signal_connect_connection_changed(connection,
			connection_changed, NULL);
    /**
     * Just some trick to provide glib signals
     */
    gmpcconn = (GmpcConnection *) gmpc_connection_new();
    g_signal_connect(G_OBJECT(gmpcconn),
			"connection_changed",
			G_CALLBACK(connection_changed_real), NULL);
    g_signal_connect(G_OBJECT(gmpcconn),
			"status_changed",
			G_CALLBACK(gmpc_status_changed_callback_real), NULL);

    TEC("Setting up mpd object signal system");
    /**
     * New Metadata object
     */
    gmw = gmpc_meta_watcher_new();
    TEC("Initializing metadata watcher");



    /** init the error messages */
    pl3_messages = playlist3_message_plugin_new();

    playlist = (GtkTreeModel*)gmpc_mpddata_model_playlist_new(
												gmpcconn,
												connection);
    gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(playlist));

    /**
     * Add the internall plugins
     */
	 plugin_manager_load_internal_plugins();


    /**
     *  load dynamic plugins
     */
    if (!settings.disable_plugins)
    {
		plugin_manager_load_plugins();
    }
    /* time todo some initialisation of plugins */
	plugin_manager_initialize_plugins();

    /**
     * Ask user about added/removed provider plugins
     */
    if (!settings.disable_plugins)
        meta_data_check_plugin_changed();
    TEC("Metadata plugin changed check");

    /**
     * Create the main window
     */
    create_playlist3();
    TEC("Creating playlist window");
    #ifdef HAVE_IPC
    if(ipc)
    {
        gmpc_tools_ipc_watch_window(ipc, GTK_WINDOW(playlist3_get_window()));
        TEC("Setup unique app to watch main window");
    }
    #endif

    /**
     * First run dialog
     *
     * If gmpc is ran for the first time, we want to show a wizard that helps
     * the user getting started.
     */
    if (cfg_get_single_value_as_int_with_default(config,
			"Default",
			"first-run",
			1))
    {
        setup_assistant();
        cfg_set_single_value_as_int(config, "Default", "first-run", 0);
        TEC("Setup first run assistant");
    }

    /**
     * If autoconnect is enabled, tell gmpc that it's in state it should connect
     */
    if (cfg_get_single_value_as_int_with_default(config,
			"connection",
			"autoconnect",
			DEFAULT_AUTOCONNECT))
    {
        gmpc_connected = TRUE;
    }
    /*
     * create timeouts
     * get the status every 1/2 second should be enough, but it's configurable.
     */
    g_timeout_add(cfg_get_single_value_as_int_with_default(config,
        "connection",
        "mpd-update-speed",
        500), (GSourceFunc) update_mpd_status, NULL);
    /**
     * create the autoconnect timeout,
     * if autoconnect enable, it will check every 5 seconds
     * if you are still connected, and reconnects you if not.
     */
    autoconnect_timeout = g_timeout_add_seconds(5,
			(GSourceFunc) autoconnect_callback, NULL);

    /**
     * Call this when entering the main loop,
     *  so you are connected on startup, not 5 seconds later
     */
    gtk_init_add((GSourceFunc) autoconnect_callback, NULL);
    if (settings.fullscreen)
    {
        gtk_init_add((GSourceFunc) pl3_window_fullscreen, NULL);
    }

    /**
     * If the user wants gmpc to be started hidden,
     * call pl3_hide after the mainloop started running
     */
    if (cfg_get_single_value_as_int_with_default(config,
			"Default",
			"start-hidden",
			FALSE) ||
			settings.start_hidden)
    {
        g_timeout_add(250, (GSourceFunc) hide_on_start, NULL);
    }
    TEC("Setting up timers");

    #ifdef ENABLE_MMKEYS
    /**
     * Setup Multimedia Keys
     */
    keys = mmkeys_new();
	gmpc_mmkeys_connect_signals(G_OBJECT(keys));
	TEC("Setting up multimedia keys");
    #endif

    url = gmpc_get_user_path("gmpc.key");
    gtk_accel_map_load(url);
    q_free(url);


    /*
     * run the main loop
     */
    gtk_main();

    /**
     * Shutting Down
     *  cleaning up.
     */
    url = gmpc_get_user_path("gmpc.key");
    gtk_accel_map_save(url);
    q_free(url);

#ifdef HAVE_IPC
    if(ipc != NULL)
    {
        g_object_unref(ipc);
    }
#endif
    /* Quit _all_ downloads */
    gmpc_easy_async_quit();

    /* tell the plugins to save themself. */
	plugin_manager_save_state();

    /* Should fix some possible crashes */
    gtk_tree_view_set_model(playlist3_get_category_tree_view(), NULL);

    /**
     * Clear metadata struct
     */
    meta_data_destroy();

    /* time todo some destruction of plugins */
	plugin_manager_destroy_plugins();

    playlist3_destroy();

    g_object_unref(playlist);
    g_object_unref(G_OBJECT(gmw));

    /* Destroy PanedSizeGroup */
    g_object_unref(paned_size_group);
    /**
     * Close the config file
     */
    TOC("Starting save config");
    cfg_close(config);
    TOC("Saved config");
    g_object_unref(gmpc_profiles);
    g_object_unref(gmpcconn);

    pixbuf_cache_destroy();
    /**
     * This now gets destroyed with the plugins
     */
    advanced_search_destroy();
    /**
     * Destroy the connection object
     */
    mpd_free(connection);

	/* Reset xml error function and cleanup */
    initGenericErrorDefaultFunc((xmlGenericErrorFunc *) NULL);
    xmlCleanupParser();
    /* cleanup */
    gmpc_mpddata_treeview_cleanup();

    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Quit....\n");
    return EXIT_SUCCESS;
}


/**
 * Function to quiet the program
 */
void main_quit(void)
{
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Quiting gmpc....");
    /**
     * close playlist and store size
     */
    pl3_hide();
    /**
     * Remove the autoconnect timeout,
     */
    if (autoconnect_timeout)
        g_source_remove(autoconnect_timeout);

    /**
     * Call the connection changed.
     * so it saves the playlist pos
     */
    mpd_signal_connect_connection_changed(connection, NULL, NULL);

    /**
     * Disconnect when connected
     */
    if (mpd_check_connected(connection))
    {
        if (cfg_get_single_value_as_int_with_default(config,
    				"connection", "stop-on-exit", FALSE))
        {
            mpd_player_stop(connection);
        }
        mpd_disconnect(connection);
    }
    /**
     * Exit main loop
     */
    gtk_main_quit();
}


/**
 * Callback that get's called every 5 seconds,
 * and tries to autoconnect
 * (when enabled)
 */

static int autoconnect_backoff = 0;
static int autoconnect_callback(void)
{
    /* Don't autoconnect while showing the first start assistant */
    if (setup_assistant_is_running())
        return FALSE;

    /* check if there is an connection. */
    if (!mpd_check_connected(connection))
    {
        /* connect when autoconnect is enabled, the user wants to be connected
         */
        if (gmpc_connected
            && cfg_get_single_value_as_int_with_default(config,
        			"connection", "autoconnect", DEFAULT_AUTOCONNECT))
        {
            connect_to_mpd();
        }
    }
    if (autoconnect_backoff < 60)
        autoconnect_backoff += 1;
    /* keep the timeout running */
    if (autoconnect_timeout)
        g_source_remove(autoconnect_timeout);
    autoconnect_timeout = g_timeout_add_seconds(5 + autoconnect_backoff,
			(GSourceFunc) autoconnect_callback, NULL);
    return FALSE;
}


static void init_stock_icons(void)
{
    char *path;

    path = gmpc_get_full_image_path();
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), path);
    q_free(path);

    gtk_window_set_default_icon_name("gmpc");

    #ifdef WIN32
    /* The Windows gtkrc sets this to 0, so images don't work on buttons */
    gtk_settings_set_long_property(gtk_settings_get_default(),
				"gtk-button-images", TRUE, "main");
    #endif

    return;
}


/**
 * Handle status changed callback from the libmpd object
 * This involves propegating the signal
 */
void GmpcStatusChangedCallback(MpdObj * mi,
								ChangedStatusType what,
								void *userdata)
{
    g_signal_emit_by_name(gmpcconn, "status-changed", mi, what);
}


/* The actual handling of the status changed signal */
static void gmpc_status_changed_callback_real(GmpcConnection * conn,
											MpdObj * mi,
											ChangedStatusType what,
											gpointer data)
{
    /* When permission changes, update the advanced search regex */
    if (what & MPD_CST_PERMISSION)
    {
        advanced_search_update_taglist();
    }
    /**
     * Make the plugins recieve the signals
     */
	 plugin_manager_status_changed(mi, what);
}


/*******************************
 * Error handling
 * TODO: Needs to be redone/rethought
 */

static void password_dialog_response(
				GtkWidget * dialog,
				gint response,
				gpointer data)
{
    gchar *path;
    switch (response)
    {
        case 0:
            return;
        case GTK_RESPONSE_OK:
        {
            path = (char *)gtk_entry_get_text(
        				GTK_ENTRY(gtk_builder_get_object(xml_password_window,
        				"pass_entry")));
            mpd_set_password(connection, path);
            if (gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(gtk_builder_get_object(
            		xml_password_window,
            		"ck_save_pass"))))
            {
                connection_set_password(path);
            }
            mpd_send_password(connection);
        }
        break;
        default:
            if (mpd_server_check_command_allowed(connection, "status") !=
        					 MPD_SERVER_COMMAND_ALLOWED)
            {
                playlist3_show_error_message(
            		_("GMPC has insufficient permissions on the mpd server."),
            		ERROR_CRITICAL);
                mpd_disconnect(connection);
            }
            break;
    }
    gtk_widget_destroy(
		(GtkWidget *) gtk_builder_get_object(xml_password_window,
		"password-dialog"));
    g_object_unref(xml_password_window);
    xml_password_window = NULL;
}


static void password_dialog(int failed)
{
    GtkWidget *pl3_win = playlist3_get_window();
    gchar *path = NULL;
    if (xml_password_window)
        return;
    path = gmpc_get_full_glade_path("password-dialog.ui");
    xml_password_window = gtk_builder_new();
    gtk_builder_add_from_file(xml_password_window, path, NULL);
    gtk_window_set_transient_for(GTK_WINDOW
        (gtk_builder_get_object(xml_password_window, "password-dialog")),
        GTK_WINDOW(pl3_win));
    q_free(path);
    if (!xml_password_window)
        return;
    if (failed)
    {
        path = g_strdup_printf(
    		_("Failed to set password on: '%s'\nPlease try again"),
    		mpd_get_hostname(connection));
    } else
    {
        path = g_strdup_printf(
    		_("Please enter your password for: '%s'"),
    		mpd_get_hostname(connection));
    }
    gtk_label_set_text(
		GTK_LABEL(gtk_builder_get_object(xml_password_window, "pass_label")),
		path);
    q_free(path);

    g_signal_connect(G_OBJECT
        (gtk_builder_get_object
        (xml_password_window, "password-dialog")), "response",
        G_CALLBACK(password_dialog_response), xml_password_window);
}


void send_password(void)
{
    password_dialog(FALSE);
}

static void playlist_support_help_button_clicked(GObject *a)
{
	open_help("ghelp:gmpc?ProblemSolving");

}

static int error_callback(MpdObj * mi,
				int error_id,
				char *error_msg,
				gpointer data)
{
	int autoconnect = cfg_get_single_value_as_int_with_default(config,
			"connection",
			"autoconnect",
			DEFAULT_AUTOCONNECT);

	/* if we are not connected we show a reconnect */
	if (!mpd_check_connected(mi))
	{
		GtkWidget *button;
		char *str;
		/* no response? then we just ignore it when autoconnecting. */
		if (error_id == 15 && autoconnect)
			return FALSE;

		str = g_markup_printf_escaped("<b>%s %i: %s</b>",
				_("error code"),
				error_id,
				error_msg);
		playlist3_show_error_message(str, ERROR_CRITICAL);
		button = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
		g_signal_connect(G_OBJECT(button),
				"clicked",
				G_CALLBACK(connect_to_mpd), NULL);
		playlist3_error_add_widget(button);
		g_free(str);
	} else
	{
		if (setup_assistant_is_running()
				&& (error_id == MPD_ACK_ERROR_PERMISSION ||
					error_id == MPD_ACK_ERROR_PASSWORD))
		{
			gchar *str = g_markup_printf_escaped("<b>%s</b>",
					_("Insufficient permission to connect to mpd. Check password"));
			setup_assistant_set_error(str);
			q_free(str);
			return TRUE;
		}
		if(error_id == MPD_ACK_ERROR_SYSTEM || error_id == MPD_ACK_ERROR_NO_EXIST) {
			if(g_regex_match_simple(".*{.*playlist.*}.*", error_msg,
						0,G_REGEX_MATCH_NOTEMPTY))
			{
				GtkWidget *button = NULL;
				if(favorites != NULL) {
					gmpc_favorites_list_set_disable(favorites,TRUE);
				}
				playlist_editor_set_disabled();
				playlist3_show_error_message(
						_("Playlist support in MPD is not working. See the "
						"manual on possible fixes.\n"
						"Playlist editor and favorites are now disabled."
						)
						, ERROR_WARNING);
				
				button = gtk_button_new_from_stock(GTK_STOCK_HELP);
				g_signal_connect(G_OBJECT(button), "clicked",
						G_CALLBACK(playlist_support_help_button_clicked), NULL);
				playlist3_error_add_widget(button);
				return FALSE;
			}
		}
		if (error_id == MPD_ACK_ERROR_PASSWORD)
		{
			password_dialog(TRUE);
		} else if (error_id == MPD_ACK_ERROR_PERMISSION)
		{
			password_dialog(FALSE);
		} else
		{
			gchar *str = g_markup_printf_escaped("<b>%s %i: %s</b>",
					_("error code"), error_id,
					error_msg);
			playlist3_show_error_message(str, ERROR_CRITICAL);
			g_free(str);
		}
	}
	return FALSE;
}


/**
 * handle a connection changed
 */
static void connection_changed(MpdObj * mi, int connected, gpointer data)
{
    /* propagate the signal to the connection object */
    if (mpd_check_connected(mi) != connected)
    {
        g_log(LOG_DOMAIN,
    			G_LOG_LEVEL_ERROR,
    			"Connection state differs from actual state: act: %i\n",
    			!connected);
    }
    /**
     * Check version
     */
    if (connected && !mpd_server_check_version(mi, 0, 13, 0))
    {
        gchar *value = g_markup_printf_escaped("<b>%s</b>",
            _("MPD versions before 0.13.0 are not supported"));
        /* disable user connect ! */
        gmpc_connected = FALSE;
        mpd_disconnect(mi);
        /* Give error */
        playlist3_show_error_message(value, ERROR_CRITICAL);
        g_free(value);
    }
    /* Remove timeout */
    if (connected)
    {
        if (autoconnect_timeout)
            g_source_remove(autoconnect_timeout);
        autoconnect_timeout = 0;
        autoconnect_backoff = 0;
    }
    if (connected)
    {
        advanced_search_update_taglist();
    }
    /**
     * force an update of status, to check password
     */
    if (connected)
    {
        mpd_status_update(mi);
        if (connected != mpd_check_connected(mi))
        {
            g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "State differs, exit");
            /* Probly disconnected when getting status..   exiting */
            return;
        }
    }

    /* remove this when it does not fix it */
    g_signal_emit_by_name(gmpcconn,
				"connection-changed",
				mi,
				mpd_check_connected(mi));
}


static void connection_changed_real(
						GmpcConnection * obj,
						MpdObj * mi,
						int connected,
						gpointer data)
{
    /**
     * propegate signals
     */
    g_log(LOG_DOMAIN,
			G_LOG_LEVEL_DEBUG,
			"Connection changed %i-%i \n",
			connected,
			mpd_check_connected(mi));
	plugin_manager_connection_changed(mi, connected);

    /**
     * force an update of status
     */
    if (connected)
        mpd_status_update(mi);

    if (connected)
    {
        playlist3_show_error_message(_("Connected to mpd"), ERROR_INFO);
    } else
    {
        playlist3_show_error_message(_("Disconnected from mpd"), ERROR_INFO);
    }

    if (!connected)
    {
        if (autoconnect_timeout)
            g_source_remove(autoconnect_timeout);
        autoconnect_timeout = g_timeout_add_seconds(5,
    			(GSourceFunc) autoconnect_callback, NULL);
        autoconnect_backoff = 0;
    }
}


/**
 * Shows an error message.
 */
void show_error_message(const gchar * string)
{
    GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "%s", string);
    gtk_widget_show(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
}

static void print_version(void)
{
    printf("%s\n", _("Gnome Music Player Client"));
    printf(GMPC_COPYRIGHT "\n\n");
    printf("%-25s: %s\n", _("Tagline"), GMPC_TAGLINE);
    printf("%-25s: %i.%i.%i\n", _("Version"),
			GMPC_MAJOR_VERSION,
			GMPC_MINOR_VERSION,
			GMPC_MICRO_VERSION);
	if (revision && revision[0] != '\0')
	{
        printf("%-25s: %s\n", _("Revision"), revision);
    }
}

/**
 * Set a basic set of easycommand handlers.
 */
static void gmpc_easy_command_set_default_entries(void)
{
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command,
			_("quit"), "",
			_("Quit gmpc"),
			(GmpcEasyCommandCallback *) main_quit,
			NULL,GTK_STOCK_QUIT);

	gmpc_easy_command_add_entry(gmpc_easy_command,
			_("hide"), "",
			_("Hide gmpc"),
			(GmpcEasyCommandCallback *) pl3_hide,
			NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command,
			_("show"), "",
			_("Show gmpc"),
			(GmpcEasyCommandCallback *) create_playlist3,
			NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command,
			_("toggle"), "",
			_("Toggle gmpc visibility"),
			(GmpcEasyCommandCallback *) pl3_toggle_hidden,
			NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command,
			_("show notification"), "",
			_("Show trayicon notification"),
			(GmpcEasyCommandCallback *) tray_icon2_create_tooltip,
			NULL);
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command,
			_("preferences"), "",
			_("Show preferences window"),
			(GmpcEasyCommandCallback *) create_preferences_window,
			NULL, GTK_STOCK_PREFERENCES);

	gmpc_easy_command_add_entry(gmpc_easy_command,
			_("bug information"), "",
			_("Show bug information"),
			(GmpcEasyCommandCallback *) bug_information_window_new,
			NULL);

	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command,
			_("url"), "",
			_("Show add url window"),
			(GmpcEasyCommandCallback *) url_start,
			NULL,"add-url");

	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command,
			_("url"), ".*://.*",
			_("Add url <scheme>://<path>"),
			(GmpcEasyCommandCallback *) url_start_easy_command,
			NULL,"add-url");
}

static void  gmpc_mmkeys_connect_signals(GObject *keys)
{
	g_signal_connect(keys,
			"mm_playpause",
			G_CALLBACK(play_song), NULL);

	g_signal_connect(keys,
			"mm_play",
			G_CALLBACK(play_song), NULL);

	g_signal_connect(keys,
			"mm_pause",
			G_CALLBACK(pause_song), NULL);

	g_signal_connect(keys,
			"mm_next",
			G_CALLBACK(next_song), NULL);

	g_signal_connect(keys,
			"mm_prev",
			G_CALLBACK(prev_song), NULL);

	g_signal_connect(keys,
			"mm_stop",
			G_CALLBACK(stop_song), NULL);

	g_signal_connect(keys,
			"mm_fastforward",
			G_CALLBACK(song_fastforward), NULL);

	g_signal_connect(keys,
			"mm_fastbackward",
			G_CALLBACK(song_fastbackward), NULL);

	g_signal_connect(keys,
			"mm_repeat",
			G_CALLBACK(repeat_toggle), NULL);

	g_signal_connect(keys,
			"mm_random",
			G_CALLBACK(random_toggle), NULL);

	g_signal_connect(keys,
			"mm_raise",
			G_CALLBACK(create_playlist3), NULL);

	g_signal_connect(keys,
			"mm_hide",
			G_CALLBACK(pl3_hide), NULL);

	g_signal_connect(keys,
			"mm_toggle_hidden",
			G_CALLBACK(pl3_toggle_hidden), NULL);

	g_signal_connect(keys,
			"mm_volume_up",
			G_CALLBACK(volume_up), NULL);

	g_signal_connect(keys,
			"mm_volume_down",
			G_CALLBACK(volume_down), NULL);

	g_signal_connect(keys,
			"mm_toggle_mute",
			G_CALLBACK(volume_toggle_mute), NULL);

	g_signal_connect(keys,
			"mm_show_notification",
			G_CALLBACK(tray_icon2_create_tooltip), NULL);

	g_signal_connect_swapped(keys,
			"mm_show_easy_command",
			G_CALLBACK(gmpc_easy_command_popup),
			gmpc_easy_command);
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=80: */
