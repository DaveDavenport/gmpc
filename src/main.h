#ifndef __MAIN_H__

#define __MAIN_H__


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

extern GtkTreeModel *playlist;

/* the config object */
extern config_obj *config;
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
extern gmpcPlugin info_plugin;

#ifdef ENABLE_TRAYICON
extern gmpcPlugin tray_icon_plug;
#endif

/** main.c **/
extern GladeXML *xml_main_window;
extern MpdObj *connection;

enum{
	TIME_FORMAT_ELAPSED,
	TIME_FORMAT_REMAINING,
	TIME_FORMAT_PERCENTAGE
};


/* mpdinteraction.c*/
int update_mpd_status();

void create_preferences_window();

/* id3info.c*/
void call_id3_window(int song);

/* main.h*/
void main_trigger_update();

void pl3_highlight_song_change ();
char * edit_song_markup(char *format);

gboolean playlist_filter_func(GtkTreeModel *model, GtkTreeIter *iter);
void id3_status_update();
void call_id3_window_song(mpd_Song *songstr);
void playlist_changed(MpdObj *mi);

void main_quit();

/*
 * functions to get patch to different files.
 * This is needed to make the windows port work.
 */
char *gmpc_get_full_image_path(char *filename);
char *gmpc_get_full_glade_path(char *filename);

/* plugin */
void plugin_load_dir(gchar *path);
void plugin_add(gmpcPlugin *plug, int plugin);
int plugin_get_pos(int id);



void show_error_message(gchar *string, int block);
int cover_art_edit_cover(gchar *artist, gchar *album);


void tray_icon_connection_changed(MpdObj *mi, int connect);




/**
 * Metadata 
 */

void meta_data_add_plugin(gmpcPlugin *plug);
#endif
