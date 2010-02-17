/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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
#include "misc.h"
#include "advanced-search.h"
#include "gmpc_easy_download.h"

#include "setup-assistant.h"
/* as internall plugin */
#include "browsers/playlist3-playlist-editor.h"
#include "browsers/playlist3-file-browser.h"
#include "browsers/playlist3-find2-browser.h"
#include "browsers/playlist3-tag2-browser.h"
#include "browsers/playlist3-current-playlist-browser.h"

#include "gmpc-mpddata-model-playlist.h"
#include "metadata-cache.h"
#include "bug-information.h"

#include "Widgets/pixbuf-cache.h"
#include <libmpd/debug_printf.h>

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
extern gmpcPlugin playlist_editor_plugin;
extern gmpcPlugin statistics_plugin;

GmpcMetadataBrowser *metadata_browser = NULL;
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

static void connection_changed_real(GmpcConnection * gmpcconn, MpdObj * mi, int connect, gpointer data);
static void gmpc_status_changed_callback_real(GmpcConnection * gmpcconn,
											  MpdObj * mi, ChangedStatusType what, gpointer data);

/**
 * Define some local functions
 */

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
static void move_old_gmpc_data(void);
static void print_version(void);

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
		}
		else if (strncmp(message, "EASYCOMMAND", strlen("EASYCOMMAND")) == 0) {
			gmpc_easy_command_do_query(gmpc_easy_command, (&message[strlen("EASYCOMMAND")]));
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



static void xml_error_func(void * ctx, const char * msg,...)
{
	va_list ap;
	va_start(ap, msg);
	g_logv("LibXML", G_LOG_LEVEL_DEBUG, msg,ap);
	va_end(ap);
}

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
	static xmlGenericErrorFunc handler = (xmlGenericErrorFunc)xml_error_func;
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
	gboolean		fullscreen				= FALSE;
	gchar			*config_path			= NULL;
	gint			debug_level				= -1;
	gchar			*profile_name			= NULL;
	
	GOptionEntry entries[] = 
	{
		{ "fullscreen",		 0,  0,G_OPTION_ARG_NONE,
			&fullscreen,		N_("Start the program in full screen"),			NULL},
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
		{ "profile",		'p', 0,G_OPTION_ARG_STRING, 
			&profile_name , N_("Select a profile"),								"<Profile Name>"},

		{NULL}
	};

	/**
     * A string used severall times to create a path
     */
	gchar *url = NULL;

	INIT_TIC_TAC();


	g_log_set_default_handler(gmpc_log_func, NULL);
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

    context = g_option_context_new (_("Gnome Music Player Client"));
    g_option_context_add_main_entries (context, entries, "gmpc");
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_add_group (context, egg_sm_client_get_option_group()); 
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
	TEC("Gtk init");
	/* connect signal to Session manager to quit */
	g_signal_connect( egg_sm_client_get(),
		"quit",
		G_CALLBACK(main_quit),
		NULL);

	TEC("EggSmClient");
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
	move_old_gmpc_data();
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
		show_error_message(_("Failed to load the configuration system."));
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
				show_error_message(_("Failed to load debug-log file."));
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
	/* PanedSizeGroup */
	paned_size_group = (GObject *)gmpc_paned_size_group_new();
	/** Signals */

	gmpc_profiles = gmpc_profiles_new();
	if(profile_name) {
		GList *iter, *items = gmpc_profiles_get_profiles_ids(gmpc_profiles);
		for(iter = g_list_first(items); iter; iter = g_list_next(iter))
		{
			if(g_utf8_collate(profile_name, gmpc_profiles_get_name(gmpc_profiles, (const gchar *)iter->data)) == 0)
			{
				connection_set_current_profile((const gchar *)iter->data);
				break;
			}
		}
		g_list_foreach(items,(GFunc) g_free, NULL);
		g_list_free(items);

	}
	TEC("Setting up gmpc idle,signals and profiles");
	/**
	 * Initialize the new metadata subsystem.
	 */
	meta_data_init();
	pixbuf_cache_create();

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
		show_error_message(_("Failed to setup libmpd"));
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
	gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(playlist));

	/** file browser */
	plugin_add(&file_browser_plug, 0, NULL);
	/** current playlist */
	plugin_add_new((GmpcPluginBase *)play_queue_plugin_new("current-pl"), 0,NULL);
	plugin_add_new((GmpcPluginBase *)gmpc_provider_music_tree_new(),0,NULL);

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
	/* Playlist editor */
	plugin_add(&playlist_editor_plugin, 0, NULL);

	plugin_add(&statistics_plugin, 0, NULL);
	plugin_add(&metadata_plug, 0, NULL);
	plugin_add(&proxyplug, 0, NULL);

	TEC("Loading internal plugins");
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_test_plugin_new()), 0, NULL);
	metadata_browser = gmpc_metadata_browser_new();
	plugin_add_new(GMPC_PLUGIN_BASE(metadata_browser), 0, NULL);
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_plugin_metadata_prefetcher_new()), 0,NULL);
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_plugin_database_update_tracker_new()), 0,NULL);
	plugin_add_new(GMPC_PLUGIN_BASE(gmpc_plugin_mockup_new()), 0,NULL);
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
		/* Load plugin from $PLUGIN_DIR if set */
		if(g_getenv("PLUGIN_DIR") != NULL) {
			gchar *path = g_build_filename(g_getenv("PLUGIN_DIR"),NULL);
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

	if( (revision != NULL && revision[0] != '\0') && cfg_get_single_value_as_int_with_default(config, "Default", "help-question", 0) < 2 && cfg_get_single_value_as_int_with_default(config, "Default", "first-run", 1)) 
	{
		GtkWidget * dialog = gtk_message_dialog_new_with_markup(NULL, 
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_QUESTION,GTK_BUTTONS_CLOSE,
				_("The GMPC  team is looking for help maintaining and developing GMPC\n"
				  "Help is needed with the following:\n"
				  "* Maintaining and updating the website.\n"
				  "* User support\n"
				  "* Maintaining plugins\n"
				  "* With maintaining GMPC itself\n"
				  "If you are interested in helping out, please join our irc channel (#gmpc on irc.freenode.net)\n"
				  "\nThanks,\n<i>Qball Cow</i>"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		cfg_set_single_value_as_int(config, "Default", "help-question", 2);
	}
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
	if(fullscreen)
		gtk_init_add((GSourceFunc) pl3_window_fullscreen, NULL);

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

	initGenericErrorDefaultFunc((xmlGenericErrorFunc *)NULL);
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

static int autoconnect_backoff = 0;
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
	if(autoconnect_backoff < 60) autoconnect_backoff += 1;
	/* keep the timeout running */
	if(autoconnect_timeout) 
		g_source_remove(autoconnect_timeout);
	autoconnect_timeout = g_timeout_add_seconds(5+autoconnect_backoff, (GSourceFunc) autoconnect_callback, NULL);
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
	g_signal_emit_by_name(gmpcconn, "status-changed", mi, what);
}

/* The actual handling of the status changed signal */
static void gmpc_status_changed_callback_real(GmpcConnection * conn, MpdObj * mi, ChangedStatusType what, gpointer data)
{
	int i;
	/* When permission changes, update the advanced search regex */
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
		return;
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
	xml_password_window = gtk_builder_new();
	gtk_builder_add_from_file(xml_password_window, path, NULL);
	gtk_window_set_transient_for(GTK_WINDOW
			(gtk_builder_get_object(xml_password_window, "password-dialog")), GTK_WINDOW(pl3_win));
	q_free(path);
	if (!xml_password_window)
		return;
	if (failed) {
		path = g_strdup_printf(_("Failed to set password on: '%s'\nPlease try again"),
				mpd_get_hostname(connection));
	} else {
		path = g_strdup_printf(_("Please enter your password for: '%s'"),
				mpd_get_hostname(connection));
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
		autoconnect_backoff = 0;
	}
	if(connected){
		advanced_search_update_taglist();
	}
	/**
	 * force an update of status, to check password
	 */
	if (connected) {
		mpd_status_update(mi);
		if (connected != mpd_check_connected(mi)) {
			g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING,"State differs, exit");
			/* Probly disconnected when getting status..   exiting */
			return;
		}
	}

	/* remove this when it does not fix it */
	g_signal_emit_by_name (gmpcconn, "connection-changed", mi, mpd_check_connected(mi));
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
		"%s",string);
	gtk_widget_show(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
}


static void move_old_gmpc_data(void)
{
	gchar *url;
	const gchar *old = g_get_home_dir();
	gchar *path;

	url =  gmpc_get_user_path(NULL);
	if (!g_file_test(url, G_FILE_TEST_EXISTS)) {
		path = g_build_filename(old, ".gmpc", NULL);
		if(g_file_test(path, G_FILE_TEST_IS_DIR)) {
			GDir *dir;
			const gchar *iter;
			/* Create the directory */
			create_gmpc_paths();
			dir = g_dir_open(path, 0, NULL);
			if(dir){
				while((iter = g_dir_read_name(dir)) != NULL)
				{
					gchar *dest_path = g_build_filename(url, iter, NULL);
					gchar *src_path = g_build_filename(path, iter, NULL);
					printf("move %s %s\n", src_path, dest_path);
					g_rename(src_path, dest_path);
					g_free(src_path); g_free(dest_path);
				}
				g_dir_close(dir);
			}
		}
		g_free(path);
	}
	g_free(url);

	url =  gmpc_get_covers_path("covers.sql");
	if (!g_file_test(url, G_FILE_TEST_EXISTS)) {
		path = g_build_filename(old, ".covers","covers.sql", NULL);
		if(g_file_test(path, G_FILE_TEST_EXISTS)) {
			/* Create the directory */
			create_gmpc_paths();
			printf("move %s %s\n",path,url);
			g_rename(path,url);
		}
		g_free(path);
	}
	g_free(url);
}

static void create_directory(gchar *url)
{
	/**
	 * Check if ~/.gmpc/ exists
	 * If not try to create it.
	 */
	if (!g_file_test(url, G_FILE_TEST_EXISTS)) {
		if (g_mkdir_with_parents(url, 0700) < 0) {
			g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to create: %s\n", url);
			show_error_message("Failed to create config directory.");
			abort();
		}
	}
	/**
	 * if it exists, check if it's a directory
	 */
	if (!g_file_test(url, G_FILE_TEST_IS_DIR)) {
		show_error_message("The config directory is not a directory.");
		abort();
	} else {
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s exist and is directory", url);
	}
}

static void create_gmpc_paths(void)
{
	/**
	 * Create needed directories for mpd.
	 */

	/** create path */
	gchar *url = gmpc_get_user_path(NULL);
	create_directory(url);
	q_free(url);

	url = gmpc_get_covers_path(NULL);
	create_directory(url);
	/* Free the path */
	q_free(url);
}

static void print_version(void)
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
