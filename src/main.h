#ifndef __MAIN_H__

#define __MAIN_H__
#include <config.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else 
#define _(String) String
#endif

#include "config-defaults.h"
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/debug_printf.h>
#include <glade/glade.h>
#include "config1.h"
#include "plugin.h"
#include "metadata.h"
#include "playlist3.h"
#include "playlist-list.h"
#include "mpdinteraction.h"
#include "tray-icon.h"
#include "gmpc-profiles.h"

#include "browsers/playlist3-playlist-editor.h"
/**
 * Some gobjects
 */
#include "gmpc-connection.h"
#include "gmpc-metaimage.h"

extern GmpcConnection *gmpcconn;


extern int gmpc_connected;

extern GtkTreeModel *playlist;

/* the config object */
extern config_obj *config;
extern config_obj *profiles;

/* the plugin list */
extern gmpcPlugin **plugins;
/* num of plugins */
extern int num_plugins;

/* the plugins :D */




extern gmpcPlugin connection_plug;
/*
extern gmpcPlugin about_plug;
*/
extern gmpcPlugin playlist_plug;

extern gmpcPlugin cover_art_plug;

extern gmpcPlugin tray_icon_plug;
extern gmpcPlugin tray_icon2_plug;

extern gmpcPlugin info3_plugin;
extern gmpcPlugin metab_plugin;
extern gmpcPlugin url_plugin; 
/** main.c **/
extern GladeXML *xml_main_window;
extern MpdObj *connection;
extern GmpcProfiles *gmpc_profiles;

extern gmpcPlugin playlist_editor_plugin;

enum{
	TIME_FORMAT_ELAPSED,
	TIME_FORMAT_REMAINING,
	TIME_FORMAT_PERCENTAGE
};

/* main.h*/
void main_trigger_update();

void pl3_highlight_song_change ();
char * edit_song_markup(char *format);

gboolean playlist_filter_func(GtkTreeModel *model, GtkTreeIter *iter);
void call_id3_window_song(mpd_Song *songstr);
void playlist_changed(MpdObj *mi);

void main_quit(void);

/*
 * functions to get patch to different files.
 * This is needed to make the windows port work.
 */
char *gmpc_get_full_glade_path(char *filename);

/* plugin */
void plugin_load_dir(gchar *path);
void plugin_add(gmpcPlugin *plug, int plugin);
int plugin_get_pos(int id);

void show_error_message(gchar *string, int block);

/* This is over here because of it's need for xml_error_window, this should probably
 * be moved to mpdinteraction.c
 */
void connect_callback(MpdObj *mi);

/** Handle status changed */
void   GmpcStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);

/**
 * Metadata 
 */

void meta_data_add_plugin(gmpcPlugin *plug);
/**
 * TODO move this 
 */
void url_start_real(const gchar *url);

/*
 * functions to get patch to different files.
 * This is needed to make the windows port work.
 */
char *gmpc_get_full_image_path(char *filename);

void preferences_show_pref_window(int plugin_id);

/**
 * Help functions
 */
#define q_free(a) g_free(a);a=NULL;

/* help */
void info2_activate(void);
void info2_fill_artist_view(char *);
void info2_fill_album_view(char *, char *);
/* tray stuff */
gboolean tray_icon2_get_available(void);
#endif
