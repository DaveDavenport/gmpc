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
#include "player.h"
#include "playlist3.h"
#include "mpdinteraction.h"

/* the config object */
extern config_obj *config;
/* the plugin list */
extern gmpcPlugin **plugins;
/* num of plugins */
extern int num_plugins;


/* the plugins :D */
extern gmpcPlugin connection_plug;
extern gmpcPlugin about_plug;
extern gmpcPlugin playlist_plug;


#ifdef ENABLE_TRAYICON
extern gmpcPlugin tray_icon_plug;
#endif

/** main.c **/
extern GladeXML *xml_main_window;
extern MpdObj *connection;
typedef struct
{
	/* playlist number this is to check if the playlist changed */
	long long playlist_id;
	int playlist_length;
	int playlist_playtime;
	/* the current song */
	int old_pos;
} internal_data;

enum{
	TIME_FORMAT_ELAPSED,
	TIME_FORMAT_REMAINING,
	TIME_FORMAT_PERCENTAGE
};

extern internal_data info;
extern guint update_timeout;
int update_interface();

/* mpdinteraction.c*/
int update_mpd_status();


void create_preferences_window();


/* id3info.c*/
void call_id3_window(int song);

/* do tray */
int create_tray_icon();

/* main.h*/
void main_trigger_update();


void pl3_highlight_song_change ();
char * edit_song_markup(char *format);
void pl3_reinitialize_tree();

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
void load_plugins_from_dir(gchar *path);
void add_plugin(gmpcPlugin *plug, int plugin);
int plugin_get_pos(int id);
#endif
