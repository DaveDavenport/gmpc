/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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
#include <glib/gstdio.h>

/* header files */
#include "main.h"
#include "playlist3.h"

/** session support */
#include "sm.h"
#include "misc.h"
#include "advanced_search.h"
#include "gmpc_easy_download.h"

#include "setup-assistant.h"
/* as internall plugin */
#include "browsers/playlist3-playlist-editor.h"
#include "browsers/playlist3-file-browser.h"
#include "browsers/playlist3-find2-browser.h"
#include "browsers/playlist3-tag2-browser.h"
#include "browsers/playlist3-current-playlist-browser.h"

/* vala */
#include "vala/gmpc-easy-command.h"
#include "vala/gmpc-test-plugin.h"
#include "vala/gmpc-metadata-browser2.h"
#include "vala/gmpc-metadata-prefetcher.h"

#include "gob/gmpc-mpddata-model-playlist.h"
#include "metadata_cache.h"
#include "bug-information.h"


#define LOG_DOMAIN "Gmpc"
/**
 * Get revision
 */
#include "revision.h"
#ifdef ENABLE_MMKEYS
#include "mm-keys.h"
#endif

#define RESET "\x1b[0m"
#define BOLD  "\x1b[1m"

extern gmpcPlugin connection_plug;
extern gmpcPlugin metadata_plug;
extern gmpcPlugin playlist_plug;
extern gmpcPlugin cover_art_plug;
extern gmpcPlugin tray_icon2_plug;
extern gmpcPlugin proxyplug;
extern gmpcPlugin metab_plugin;
extern gmpcPlugin playlist_editor_plugin;
extern gmpcPlugin statistics_plugin;
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


/**
 * This flag indicate the requested connection state by the user.
 * If the user presses disconnect,  you don't want to auto-connect anymore.
 **/
int gmpc_connected = FALSE;

static void connection_changed_real(GmpcConnection * gmpcconn, MpdObj * mi, int connect, gpointer data);
static void gmpc_status_changed_callback_real(GmpcConnection * gmpcconn,
											  MpdObj * mi, ChangedStatusType what, gpointer data);

/**
 * Define some local functions
 */
/**
 * Error dialog
 */
static GtkWidget *error_dialog = NULL;
static GtkListStore *error_list_store = NULL;

/** handle connection changed */
static void connection_changed(MpdObj * mi, int connect, gpointer data);

/** Error callback */
static int error_callback(MpdObj * mi, int error_id, char *error_msg, gpointer data);

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

/**
 * Set paths
 */
static void create_gmpc_paths(void);

void print_version(void);

#ifndef WIN32
#include "bacon/bacon-message-connection.h"
static BaconMessageConnection *bacon_connection = NULL;
/**
 * Handle incoming (IPC) messages.
 * GMPC ships a utility called "gmpc-remote" that uses this interface.
 */
 #define LOG_DOMAIN_IPC "IPC"
static void bacon_on_message_received(const char *message, gpointer data)
{

	if (message) {
		g_log(LOG_DOMAIN_IPC, G_LOG_LEVEL_DEBUG, "got message: '%s'\n", message);
		/**
         * Makes mpd quit.
         */
		if (strcmp(message, "QUIT") == 0) {
			printf("I've been told to quit, doing this now\n");
			main_quit();
		}
		/**
         * Gives play,pause command
         */
		else if (strcmp(message, "PLAY") == 0 || strcmp(message, "PAUSE") == 0) {
			play_song();
		}
		/**
         * Give next command
         */
		else if (strcmp(message, "NEXT") == 0) {
			next_song();
		}
		/**
         * Give previous command
         */
		else if (strcmp(message, "PREV") == 0) {
			prev_song();
		}
		/**
         * Stop playback
         */
		else if (strcmp(message, "STOP") == 0) {
			stop_song();
		} else if (strcmp(message, "TOGGLE_VIEW") == 0) {
			pl3_toggle_hidden();
		} else if (strcmp(message, "HIDE_VIEW") == 0) {
			pl3_hide();
		} else if (strcmp(message, "SHOW_VIEW") == 0) {
			create_playlist3();
		}
		else if (strcmp(message, "CONNECT") == 0) {
			connect_to_mpd();
		}
		/**
         * pass gmpc an url to parse with the url_parser.
         */
		else if (strncmp(message, "STREAM ", 7) == 0) {
			url_start_real(&message[7]);
		} else {
			create_playlist3();
		}
	}
	/**
	 * Bring gmpc to front, as default action.
	 */

}
#endif

static GLogLevelFlags global_log_level = G_LOG_LEVEL_MESSAGE;
static void gmpc_log_func(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	if(log_level <= global_log_level)
	{
		g_log_default_handler(log_domain, log_level, message, user_data);
	}
}
static gboolean set_log_filter(const gchar *option_name, const gchar *value, gpointer data, GError **error)
{
	if(value == NULL || value[0] == 0){
		g_set_error(error, 0, 0, "--log-filter requires a log domain as argument");
		return FALSE;
	}

	g_log_set_handler(value, G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL|G_LOG_FLAG_RECURSION, g_log_default_handler, NULL);
	return TRUE;
}
static gboolean hide_on_start(void)
{
	pl3_hide();
	return FALSE;
}
int main(int argc, char **argv)
{
	int i;

#ifdef WIN32
	gchar *packagedir;
#endif
#ifdef ENABLE_MMKEYS
	MmKeys *keys = NULL;
#endif

	GError			*error					= NULL;
	GOptionContext	*context				= NULL;
	gboolean		show_version			= FALSE;
	gboolean		disable_plugins			= FALSE;
	gboolean		start_hidden			= FALSE;
	gboolean		clean_config			= FALSE;
	gboolean		quit					= FALSE;
	gboolean		replace					= FALSE;
	gboolean		do_debug_updates		= FALSE;
	gboolean		show_bug_information	= FALSE;
	gchar			*config_path			= NULL;
	gint			debug_level				= -1;
	
	GOptionEntry entries[] = 
	{
		{ "version",		'v', 0,G_OPTION_ARG_NONE,
			&show_version,		N_("Show program version and revision"),			NULL},
		{ "quit",			'q', 0,G_OPTION_ARG_NONE,
			&quit,				N_("Quits the running gmpc"),						NULL},
		{ "replace",		'r', 0,G_OPTION_ARG_NONE,
			&replace,			N_("Replace the running gmpc"),						NULL},
		{ "disable-plugins", 0 , 0,G_OPTION_ARG_NONE,
			&disable_plugins,	N_("Don't load the plugins"),						NULL},
		{ "config",			 0 , 0,G_OPTION_ARG_FILENAME,
			&config_path,		N_("Load alternative config file"),				  "Path"},
		{ "debug-level",	'd', 0,G_OPTION_ARG_INT,
			&debug_level,		N_("Set the debug level"),						 "level"},
		{ "start-hidden",	'h', 0,G_OPTION_ARG_NONE,
			&start_hidden,		N_("Start gmpc hidden to tray"),					NULL},
		{ "clean-cover-db",	 0 , 0,G_OPTION_ARG_NONE,		
			&clean_config,		N_("Remove all failed hits from metadata cache"),	NULL},
		{ "debug-updates",	 0 , G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, 
			&do_debug_updates, 	N_("Show redraw events in GTK+"),					NULL},
		{ "bug-information",'b', 0,G_OPTION_ARG_NONE,
			&show_bug_information, N_("Show bug information dialog"),				NULL},
		{ "log-filter",		'f', 0,G_OPTION_ARG_CALLBACK, 
			set_log_filter, N_("Shows all output from a certain log domain"),		"<Log domain>"},
			

		{NULL}
	};

	/**
     * A string used severall times to create a path
     */
	gchar *url = NULL;

	INIT_TIC_TAC();


	/* *
	 * Set the default debug level
	 * Depending if it is a git build or not
	 */
	if (revision && revision[0] != '\0') {
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
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Setting NLS");
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif
	gtk_set_locale();

	TEC("Setting up locale");

    context = g_option_context_new (_("Gnome Music Player Client"));
    g_option_context_add_main_entries (context, entries, "gmpc");
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free(context);
	if(error) {
		g_log(NULL, G_LOG_LEVEL_ERROR, "Failed to parse commandline options: %s", error->message);
		g_error_free(error);
	}

	/* Show the version, if requested */
	if(show_version) {
		print_version();
		return EXIT_SUCCESS;
	}

	g_log_set_default_handler(gmpc_log_func, NULL);
	/**
	 * Set debug level, options are
	 * 0 = No debug
	 * 1 = Error messages
	 * 2 = Error + Warning messages
	 * 3 = All messages
	 */
	if (debug_level >=0){
		if(debug_level == 3){
			global_log_level = G_LOG_LEVEL_DEBUG;
		}else if (debug_level == 2){
			global_log_level = G_LOG_LEVEL_INFO;
		}
		debug_set_level(debug_level);
	}
	/* Show the bug-information dialog */
	if(show_bug_information){
		gtk_init(&argc, &argv);
		bug_information_window_new(NULL);
		return EXIT_SUCCESS;
	}
	TEC("Parsing command line options");

	/**
     *  initialize threading
     */
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Initializing threading");

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
     * This is used to re-enable rule-hint in the treeview. (this is forced off on windows).
     */
	packagedir = g_win32_get_package_installation_directory_of_module(NULL);
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Got %s as package installation dir", packagedir);
	url = g_build_filename(packagedir, "share", "gmpc", "gmpc-gtk-win32.rc", NULL);
	q_free(packagedir);
	gtk_rc_add_default_file(url);
	g_free(url);
#endif

	/* initialize gtk */
	gtk_init(&argc, &argv);
	TEC("Initializing gtk");

	gmpc_easy_command = gmpc_easy_command_new();
	/* Add it to the plugin command */
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_easy_command), 0, NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command, _("quit"), "",
			_("Quit gmpc"), (GmpcEasyCommandCallback *) main_quit, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command, _("hide"), "",
			_("Hide gmpc"), (GmpcEasyCommandCallback *) pl3_hide, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command, _("show"), "", 
			_("Show gmpc"), (GmpcEasyCommandCallback *)create_playlist3, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command, _("show notification"),"",
			_("Show trayicon notification"), (GmpcEasyCommandCallback *)tray_icon2_create_tooltip, NULL);

	TEC("Init easy command");

	advanced_search_init();
	TEC("Init advanced search");
	/**
     * Call create_gmpc_paths();
     * This function checks if the path needed path are available, if not, create them
     */
	create_gmpc_paths();
	TEC("Check version and create paths");

	/**
     * COMMANDLINE_OPTION:
     * Cleanup the metadata database and quit.
     */
	if (clean_config) {
		/* start the metadata system */
		meta_data_init();
		printf("Cleaning up cover file..\n");
		/* Call the cleanup */
		metadata_cache_cleanup();
		printf("Done..\n");
		/* Destroy the meta data system and exit. */
		meta_data_destroy();
		TEC("Database cleanup");
		return 1;
	}

	/**
     * Open the config file
     */
	/**
     * Check if the user has forced a different config file location.
     * else set to ~/.gmpc/gmpc.cfg
     */
	if (!config_path) {
		url = gmpc_get_user_path("gmpc.cfg");
	} else {
		url = config_path;
	}
	/**
     * Open it
     */
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Trying to open the config file: %s", url);
	config = cfg_open(url);

	/** test if config opened correct  */
	if (config == NULL) {
	/**
         * Show gtk error message and quit
         */
		g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to save/load configuration:\n%s\n", url);
		show_error_message(_("Failed to load the configuration system."), TRUE);
		/* this is an error so bail out correctly */
		abort();
	}
	TEC("Opening config file: %s", url);
	/**
     * cleanup
     */
	q_free(url);

	/**
     * If requested, output debug info to file
     */
	if (cfg_get_single_value_as_int_with_default(config, "Default", "Debug-log", FALSE)) {
		url = gmpc_get_user_path("debug-info.log");
		if (url) {
			FILE *fp = g_fopen(url, "a");
			if (!fp) {
				g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to open debug-log file: \"%s\"\n", url);
				show_error_message(_("Failed to load debug-log file."), TRUE);
				abort();
			}
			/* Set the output */
			debug_set_output(fp);
			/* Force highest level */
			debug_set_level(DEBUG_INFO);
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "***** Opened debug log file\n");
			q_free(url);
			TEC("Enabled Debug log");
		}
	}
	/**
     * TODO, Check if version changed, then say something about it
     */
	url = cfg_get_single_value_as_string(config, "Default", "version");
	if (url == NULL || strcmp(url, VERSION)) {
		int *new_version = split_version(VERSION);
		if (url) {
			int *old_version = split_version((const char *)url);
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Welcome to a new version of gmpc.\n");
			/* Do possible cleanup of config files and stuff */
			/* old version older then 0.1.15.4.98 */
			if ((old_version[0] <= 0 && old_version[1] <= 15 && old_version[2] <= 4 && old_version[3] <= 98)) {
				conf_mult_obj *iter, *cmo = cfg_get_class_list(config);
				g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Purging old keys from the config file.\n");
				for (iter = cmo; iter; iter = iter->next) {
					if (strstr(iter->key, "colpos")
						|| strstr(iter->key, "colshow")
						|| strstr(iter->key, "colsize")) {
						g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Removing entry: %s\n", iter->key);
						cfg_remove_class(config, iter->key);
					}
				}
				cfg_free_multiple(cmo);

			}
			/* old version older then 0.16.2 */
			if ((old_version[0] <= 0 && old_version[1] <= 16 && old_version[2] <= 2)) {
				/* update old key */
				//printf("** Update of db set, because of new version\n");
				//import_old_db = TRUE;
			}
			/* old version older then 0.17.0-beta1 */
			if ((old_version[0] <= 0 && old_version[1] <= 16 && old_version[2] <= 95)) {
				printf("** Correct icon-size\n");
				cfg_set_single_value_as_int(config, "gmpc-mpddata-model", "icon-size", 32);
			}
			q_free(old_version);
		}
		/* set new */
		cfg_set_single_value_as_string(config, "Default", "version", VERSION);
		q_free(new_version);
	}
	if (url) {
		q_free(url);
	}
	TEC("New version check");

#ifndef WIN32
	/**
     * Start IPC system.
     */
	if (cfg_get_single_value_as_int_with_default(config, "Default", "allow-multiple", FALSE) == FALSE) {
		/**
		 * bacon here we come
         */
		bacon_connection = bacon_message_connection_new("gmpc");
		if (bacon_connection != NULL) {
			if (!bacon_message_connection_get_is_server(bacon_connection)) {
				if (replace || quit) {
					bacon_message_connection_send(bacon_connection, "QUIT");
					while (!bacon_message_connection_get_is_server(bacon_connection)) {
						bacon_message_connection_free(bacon_connection);
						bacon_connection = bacon_message_connection_new("gmpc");
						g_usleep(G_USEC_PER_SEC);
					}
				} else {
					g_log(LOG_DOMAIN_IPC, G_LOG_LEVEL_WARNING, "gmpc is allready running\n");
					bacon_message_connection_send(bacon_connection, "PRESENT");
					bacon_message_connection_free(bacon_connection);
					cfg_close(config);
					config = NULL;
					TEC("IPC setup and quitting");
					exit(0);
				}
			}
			bacon_message_connection_set_callback(bacon_connection, bacon_on_message_received, NULL);
		}
		/* If user requested a quit, quit */
		if (quit) {
			cfg_close(config);
			config = NULL;
			if (bacon_connection)
				bacon_message_connection_free(bacon_connection);

			exit(0);
		}
	}
	TEC("IPC setup");
#endif
	if (quit) {
		cfg_close(config);
		return EXIT_SUCCESS;
	}
	/**
     * Setup session support
     */
#ifdef ENABLE_SM
	smc_connect(argc, argv);
	TEC("Session manager setup");
#endif
	/** Signals */

	gmpc_profiles = gmpc_profiles_new();
	TEC("Setting up gmpc idle,signals and profiles");
	/**
     * Initialize the new metadata subsystem.
     * (Will spawn a new thread, so have to be after the init threading
     */
	meta_data_init();

	TEC("Initializing metadata system");

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
	if (connection == NULL) {
	/**
         * if failed, print error message
         */
		g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to create connection object\n");
		show_error_message(_("Failed to setup libmpd"), TRUE);
		abort();
	}
	TEC("Setting up mpd connection object");
	/**
     * Connect signals to the connection object
     */
	mpd_signal_connect_status_changed(connection, GmpcStatusChangedCallback, NULL);
	mpd_signal_connect_error(connection, error_callback, NULL);
	mpd_signal_connect_connection_changed(connection, connection_changed, NULL);
	/**
     * Just some trick to provide glib signals
     */
	gmpcconn = (GmpcConnection *) gmpc_connection_new();
	g_signal_connect(G_OBJECT(gmpcconn), "connection_changed", G_CALLBACK(connection_changed_real), NULL);
	g_signal_connect(G_OBJECT(gmpcconn), "status_changed", G_CALLBACK(gmpc_status_changed_callback_real), NULL);

	TEC("Setting up mpd object signal system");
	/**
     * New Metadata object
     */
	gmw = gmpc_meta_watcher_new();
	TEC("Initializing metadata watcher");

	/**
     * Add the internall plugins
     */

	/** init the error messages */
	pl3_messages = playlist3_message_plugin_new();


    playlist = (GtkTreeModel *)gmpc_mpddata_model_playlist_new(gmpcconn,connection);
	g_object_ref_sink(playlist);
    gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(playlist));
	/** current playlist */
	plugin_add_new((GmpcPluginBase *)play_queue_plugin_new(), 0,NULL);

	/** file browser */
	plugin_add(&file_browser_plug, 0, NULL);
	/** Find Browser */
	plugin_add(&find2_browser_plug, 0, NULL);
	/* this shows the connection preferences */
	plugin_add(&connection_plug, 0, NULL);
	/* this the server preferences */
	plugin_add(&server_plug, 0, NULL);
	/* this shows the playlist preferences */
	plugin_add(&playlist_plug, 0, NULL);
	/* this shows the markup stuff */
	plugin_add(&tag2_plug, 0, NULL);
#ifdef ENABLE_MMKEYS
	plugin_add(&mmkeys_plug, 0, NULL);
#endif
	/* the tray icon */
	plugin_add(&tray_icon2_plug, 0, NULL);

	/* Info3 data browser */
	/* Meta data browser */
	plugin_add(&metab_plugin, 0, NULL);
	/* Playlist editor */
	plugin_add(&playlist_editor_plugin, 0, NULL);

	plugin_add(&statistics_plugin, 0, NULL);

	plugin_add(&metadata_plug, 0, NULL);
	plugin_add(&proxyplug, 0, NULL);

	TEC("Loading internal plugins");
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_test_plugin_new()), 0, NULL);
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_metadata_browser_new()), 0, NULL);
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_plugin_metadata_prefetcher_new()), 0,NULL);
	TEC("Loading new plugins");
	/**
     *  load dynamic plugins
     */
	if (!disable_plugins) {
#ifdef WIN32
		packagedir = g_win32_get_package_installation_directory_of_module(NULL);
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Got %s as package installation dir", packagedir);
		url = g_build_filename(packagedir, "lib", "gmpc", "plugins", NULL);
		q_free(packagedir);

		plugin_load_dir(url);
		q_free(url);
#else
		/* This is the right location to load gmpc plugins */
		url = g_build_path(G_DIR_SEPARATOR_S, PACKAGE_LIB_DIR, "plugins", NULL);
		plugin_load_dir(url);
		q_free(url);
#endif

		if(g_getenv("PLUGIN_DIR") != NULL) {
			gchar *path = g_build_filename(g_getenv("PLUGIN_DIR"),NULL);
			printf("Loading plugin dir: %s\n", path);
			if (path && g_file_test(path, G_FILE_TEST_IS_DIR)) {
				plugin_load_dir(path);
			}
			if(path) g_free(path);
		}
		/* user space dynamic plugins */
		url = gmpc_get_user_path("plugins");
		/**
		 * if dir exists, try to load the plugins.
		 */
		if (g_file_test(url, G_FILE_TEST_IS_DIR)) {
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Trying to load plugins in: %s", url);
			if (!disable_plugins)
				plugin_load_dir(url);
		}
		TEC("Loading plugins from %s", url);
		q_free(url);
	}
	/* time todo some initialisation of plugins */
	for (i = 0; i < num_plugins && plugins[i] != NULL; i++) {
		TEC("Initializing plugin: %s", gmpc_plugin_get_name(plugins[i]));
		gmpc_plugin_init(plugins[i]);
	}

	/**
     * Ask user about added/removed provider plugins
     */
	if(!disable_plugins)
		meta_data_check_plugin_changed();
	TEC("Metadata plugin changed check");

    /* Set window debug, this is used for developers to visualize redraws */
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Create main window\n");
	gdk_window_set_debug_updates(do_debug_updates);
	/**
     * Create the main window
     */
	create_playlist3();
    /* Initialize the message system */
	//playlist3_message_init()
	plugin_add_new(GMPC_PLUGIN_BASE(pl3_messages), 0, NULL);
	
	TEC("Creating playlist window");

	/**
     * First run dialog
     * 
     * If gmpc is ran for the first time, we want to show a wizard that helps 
     * the user getting started.
     */
	if (cfg_get_single_value_as_int_with_default(config, "Default", "first-run", 1)) {
		setup_assistant();
		cfg_set_single_value_as_int(config, "Default", "first-run", 0);
		TEC("Setup first run assistant");
	}

	/**
     * If autoconnect is enabled, tell gmpc that it's in state it should connect
     */
	if (cfg_get_single_value_as_int_with_default(config, "connection", "autoconnect", DEFAULT_AUTOCONNECT)) {
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
     * create the autoconnect timeout, if autoconnect enable, it will check every 5 seconds
     * if you are still connected, and reconnects you if not.
     */
	autoconnect_timeout = g_timeout_add_seconds(5, (GSourceFunc) autoconnect_callback, NULL);

	/**
     * Call this when entering the main loop, so you are connected on startup, not 5 seconds later
     */
	gtk_init_add((GSourceFunc) autoconnect_callback, NULL);

    /**
     * If the user wants gmpc to be started hidden, call pl3_hide after the mainloop started running
     */
	if (cfg_get_single_value_as_int_with_default(config, "Default", "start-hidden", FALSE) || start_hidden) {
		g_timeout_add(250, (GSourceFunc)hide_on_start, NULL);
	}
	TEC("Setting up timers");

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
	g_signal_connect(G_OBJECT(keys), "mm_raise", G_CALLBACK(create_playlist3), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_hide", G_CALLBACK(pl3_hide), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_toggle_hidden", G_CALLBACK(pl3_toggle_hidden), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_volume_up", G_CALLBACK(volume_up), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_volume_down", G_CALLBACK(volume_down), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_toggle_mute", G_CALLBACK(volume_toggle_mute), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_show_notification", G_CALLBACK(tray_icon2_create_tooltip), NULL);
	g_signal_connect_swapped(G_OBJECT(keys), "mm_show_easy_command", G_CALLBACK(gmpc_easy_command_popup), gmpc_easy_command);
	TEC("Setting up multimedia keys");

#endif
	/*
	 * run the main loop
	 */
	gtk_main();

	/**
     * Shutting Down
     *  cleaning up.
     */

#ifndef WIN32
	if (bacon_connection) {
		bacon_message_connection_free(bacon_connection);
		bacon_connection = NULL;
	}
#endif
	/* Quit _all_ downloads */
	gmpc_easy_async_quit();

	/* tell the plugins to save themself. */
	for (i = 0; i < num_plugins && plugins[i] != NULL; i++) {
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Telling '%s' to save itself\n", gmpc_plugin_get_name(plugins[i]));
		gmpc_plugin_save_yourself(plugins[i]);
	}
	/* Should fix some possible crashes */
	gtk_tree_view_set_model(playlist3_get_category_tree_view(), NULL);

	/**
     * Clear metadata struct
     */
	meta_data_destroy();

	/* time todo some destruction of plugins */
	for (i = 0; i < num_plugins && plugins[i] != NULL; i++) {
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Telling '%s' to destroy itself\n", gmpc_plugin_get_name(plugins[i]));
		gmpc_plugin_destroy(plugins[i]);
	}

	//playlist3_message_destroy();
	playlist3_destroy();

	g_object_unref(playlist);
	g_object_unref(G_OBJECT(gmw));

	/**
     * Close the config file
     */
	TOC("Starting save config");
	cfg_close(config);
	TOC("Saved config");
	g_object_unref(gmpc_profiles);
	g_object_unref(gmpcconn);

	/**
	 * This now gets destroyed with the plugins
	 */
	advanced_search_destroy();
	/**
     * Destroy the connection object
     */
	mpd_free(connection);

	xmlCleanupParser();
	/* cleanup */
	gmpc_mpddata_treeview_cleanup();

	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Quit....\n");
	return 0;
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
	if (mpd_check_connected(connection)) {
		if (cfg_get_single_value_as_int_with_default(config, "connection", "stop-on-exit", FALSE)) {
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
static int autoconnect_callback(void)
{
	/* check if there is an connection. */
	if (!mpd_check_connected(connection)) {
		/* connect when autoconnect is enabled, the user wants to be connected
		 */
		if (gmpc_connected
			&& cfg_get_single_value_as_int_with_default(config, "connection", "autoconnect", DEFAULT_AUTOCONNECT)) {
			connect_to_mpd();
		}
	}
	/* keep the timeout running */
	return TRUE;
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
	gtk_settings_set_long_property(gtk_settings_get_default(), "gtk-button-images", TRUE, "main");
#endif

	return;
}

/**
 * Handle status changed callback from the libmpd object
 * This involves propegating the signal
 */
void GmpcStatusChangedCallback(MpdObj * mi, ChangedStatusType what, void *userdata)
{
	gmpc_connection_call_status_changed(gmpcconn, mi, what);
}

/* The actual handling of the status changed signal */
static void gmpc_status_changed_callback_real(GmpcConnection * conn, MpdObj * mi, ChangedStatusType what, gpointer data)
{
	int i;
	if(what&MPD_CST_PERMISSION){
		advanced_search_update_taglist();
	}
	/**
	 * Make the plugins recieve the signals
	 */
	for (i = 0; i < num_plugins; i++) {
		gmpc_plugin_status_changed(plugins[i], mi, what);
	}

}

/*******************************
 * Error handling
 * TODO: Needs to be redone/rethought
 */

static void password_dialog_response(GtkWidget * dialog, gint response, gpointer data)
{
	gchar *path;
	switch (response) {
	case 0:
	{

		return;
	}
	case GTK_RESPONSE_OK:
		{
			path = (char *)
				gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(xml_password_window, "pass_entry")));
			mpd_set_password(connection, path);
			if (gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON(gtk_builder_get_object(xml_password_window, "ck_save_pass")))) {
				connection_set_password(path);
			}
			mpd_send_password(connection);
		}
		break;
	default:
		if (mpd_server_check_command_allowed(connection, "status") != MPD_SERVER_COMMAND_ALLOWED) {
			playlist3_show_error_message(_("GMPC has insufficient permissions on the mpd server."), ERROR_CRITICAL);
			mpd_disconnect(connection);
		}
		break;
	}
	gtk_widget_destroy((GtkWidget *)
					   gtk_builder_get_object(xml_password_window, "password-dialog"));
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
	xml_password_window = gtk_builder_new();	//glade_xml_new(path, "password-dialog",NULL);
	gtk_builder_add_from_file(xml_password_window, path, NULL);
	gtk_window_set_transient_for(GTK_WINDOW
								 (gtk_builder_get_object(xml_password_window, "password-dialog")), GTK_WINDOW(pl3_win));
	q_free(path);
	if (!xml_password_window)
		return;
	if (failed) {
		path = g_strdup_printf(_("Failed to set password on: '%s'\nPlease try again"), mpd_get_hostname(connection));
	} else {
		path = g_strdup_printf(_("Please enter your password for: '%s'"), mpd_get_hostname(connection));
	}
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(xml_password_window, "pass_label")), path);
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

static int error_callback(MpdObj * mi, int error_id, char *error_msg, gpointer data)
{
	int autoconnect = cfg_get_single_value_as_int_with_default(config, "connection",
															   "autoconnect",
															   DEFAULT_AUTOCONNECT);
	/* if we are not connected we show a reconnect */
	if (!mpd_check_connected(mi)) {
		GtkWidget *button;
		char *str;
		/* no response? then we just ignore it when autoconnecting. */
		if (error_id == 15 && autoconnect)
			return FALSE;

		str = g_markup_printf_escaped("<b>%s %i: %s</b>", _("error code"), error_id, error_msg);
		playlist3_show_error_message(str, ERROR_CRITICAL);
		button = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(connect_to_mpd), NULL);
		playlist3_error_add_widget(button);
		g_free(str);
	} else {
		if (setup_assistant_is_running()
			&& (error_id == MPD_ACK_ERROR_PERMISSION || error_id == MPD_ACK_ERROR_PASSWORD)) {
			gchar *str = g_markup_printf_escaped("<b>%s</b>",
												 _("Insufficient permission to connect to mpd. Check password"));
			setup_assistant_set_error(str);
			q_free(str);
			return TRUE;
		}
		if (error_id == MPD_ACK_ERROR_PASSWORD) {
			password_dialog(TRUE);
		} else if (error_id == MPD_ACK_ERROR_PERMISSION) {
			password_dialog(FALSE);
		} else {
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
	if (mpd_check_connected(mi) != connected) {
		g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR,
					 "Connection state differs from actual state: act: %i\n", !connected);
	}
	/**
	 * Check version
	 */
	if (connected && !mpd_server_check_version(mi, 0, 13, 0)) {
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
	if (connected) {
		if (autoconnect_timeout)
			g_source_remove(autoconnect_timeout);
		autoconnect_timeout = 0;
	}
	/**
     * send password, first thing we do, if connected
     */
	/*
	if (connected) {
		if (connection_use_auth()) {
			mpd_send_password(connection);
		}
		advanced_search_update_taglist();
	}
*/
	if(connected){
		advanced_search_update_taglist();
	}
	/**
     * force an update of status, to check password
     */
	if (connected) {
		mpd_status_update(mi);
		if (connected != mpd_check_connected(mi)) {
			g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR, "State differs, exit");
			/* Probly disconnected when getting status..   exiting */
			return;
		}
	}

	/* remove this when it does not fix it */
	gmpc_connection_call_connection_changed(gmpcconn, mi, mpd_check_connected(mi));
}

static void connection_changed_real(GmpcConnection * obj, MpdObj * mi, int connected, gpointer data)
{
	int i = 0;
	INIT_TIC_TAC();

	/**
     * propegate signals
     */
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Connection changed %i-%i \n", connected, mpd_check_connected(mi));
	for (i = 0; i < num_plugins; i++) {
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Connection changed plugin: %s\n", gmpc_plugin_get_name(plugins[i]));
		gmpc_plugin_mpd_connection_changed(plugins[i], mi, connected, NULL);
		TEC("Connection changed plugin: %s", gmpc_plugin_get_name(plugins[i]));

	}

	/**
     * force an update of status
     */
	if (connected)
		mpd_status_update(mi);

	if (connected) {
		playlist3_show_error_message(_("Connected to mpd"), ERROR_INFO);
	} else {
		playlist3_show_error_message(_("Disconnected from mpd"), ERROR_INFO);
	}

	if (!connected) {
		if (autoconnect_timeout)
			g_source_remove(autoconnect_timeout);
		autoconnect_timeout = g_timeout_add_seconds(5, (GSourceFunc) autoconnect_callback, NULL);

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

void show_error_message(const gchar * string, const int block)
{
	GtkTreeIter iter;
	GtkWidget *label = NULL;
	if (!error_dialog) {
		GtkWidget *hbox = NULL, *image;
		GtkWidget *vbox = NULL, *sw = NULL, *tree = NULL;
		GtkWidget *pl3_win = NULL;
		GtkCellRenderer *renderer;
		/* create dialog */
		error_dialog =
			gtk_dialog_new_with_buttons(_
										("Error occurred during operation"),
										NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
		if (pl3_xml) {
			pl3_win = playlist3_get_window();
			gtk_window_set_transient_for(GTK_WINDOW(error_dialog), GTK_WINDOW(pl3_win));
		}
		/** create list store */
		if (!error_list_store) {
			error_list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			/* don't want this destroyed */
			g_object_ref(error_list_store);
		}
		hbox = gtk_hbox_new(FALSE, 6);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(error_dialog)->vbox), hbox);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 9);

		/* Error image */
		image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);
		gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 0);

		vbox = gtk_vbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

		/* Create label */
		label = gtk_label_new(_("The following error(s) occurred:"));
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
		/** Create tree view */
		/* sw */
		sw = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		/* tree */
		tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(error_list_store));
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
		/* add tree to sw */
		gtk_container_add(GTK_CONTAINER(sw), tree);
		/** add cell renderers */
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree),
													-1, _("Error Message"), renderer, "text", 0, NULL);

		/** add sw to vbox */
		gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
	}
	gtk_list_store_append(error_list_store, &iter);
	gtk_list_store_set(error_list_store, &iter, 0, string, -1);
	g_signal_connect(G_OBJECT(error_dialog), "response", G_CALLBACK(error_message_destroy), NULL);
	gtk_widget_show_all(error_dialog);
	if (block) {
		gtk_dialog_run(GTK_DIALOG(error_dialog));
	} else {
		gtk_widget_show(error_dialog);
	}
}

static void create_gmpc_paths(void)
{
	/**
	 * Create needed directories for mpd.
	 */

	/** create path */
	gchar *url = gmpc_get_user_path(NULL);

	/**
	 * Check if ~/.gmpc/ exists
	 * If not try to create it.
	 */
	if (!g_file_test(url, G_FILE_TEST_EXISTS)) {
		if (g_mkdir_with_parents(url, 0700) < 0) {
			g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to create: %s\n", url);
			show_error_message("Failed to create config directory.", TRUE);
			abort();
		}
	}
	/**
	 * if it exists, check if it's a directory
	 */
	if (!g_file_test(url, G_FILE_TEST_IS_DIR)) {
		show_error_message("The config directory is not a directory.", TRUE);
		abort();
	} else {
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s exist and is directory", url);
	}
	/* Free the path */
	q_free(url);
}

void print_version(void)
{
	printf(BOLD "%s\n", ("Gnome Music Player Client"));

	printf(GMPC_COPYRIGHT "\n\n" RESET);
	printf("%-25s: %s\n", ("Tagline"), GMPC_TAGLINE);
	printf("%-25s: %i.%i.%i\n", ("Version"), GMPC_MAJOR_VERSION, GMPC_MINOR_VERSION, GMPC_MICRO_VERSION);
	if (revision && revision[0] != '\0') {
		printf("%-25s: %s\n", ("Revision"), revision);
	}
}
/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
