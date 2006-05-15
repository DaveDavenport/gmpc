#ifndef __PLAYLIST3_H__
#define __PLAYLIST3_H__

#include <libmpd/libmpd.h>

extern GtkTreeStore *pl3_tree;
extern GtkListStore *pl3_store;

void create_playlist3 ();
void pl3_update();
void pl3_push_statusbar_message(char *mesg);
void pl3_push_rsb_message(gchar *string);
void pl3_disconnect();
void pl3_cat_sel_changed();
int pl3_close();
int pl3_hide();
int pl3_window_key_press_event(GtkWidget *mw, GdkEventKey *event);

enum{
	PL3_CURRENT_PLAYLIST,
	PL3_BOOKMARKS,
	PL3_BROWSE_FILE,
	PL3_BROWSE_ARTIST,
	PL3_BROWSE_ALBUM,
	PL3_BROWSE_XIPH,
	PL3_BROWSE_CUSTOM_STREAM,
	PL3_FIND,
	PL3_BROWSE_CUSTOM_TAG,
	PL3_NTYPES
	/* more space for options, like shoutcast */
} tree_type;


gboolean toggle_playlist3(GtkToggleButton *tb);


void pl3_highlight_song_change();

enum store_types
{
	SONG_ID,
	SONG_POS,
	SONG_TITLE,
	WEIGHT_INT,
	SONG_STOCK_ID,
	SONG_TIME,
	SONG_TYPE, /* 0 = file, 1 = stream */
	SONG_PATH,
	NROWS
};

void pl3_playlist_search();

void pl3_playlist_changed();
void pl3_clear_playlist();
void pl3_show_song_info ();

int pl3_cat_get_selected_browser();
void pl3_updating_changed(MpdObj *mi, int updating);
void pl3_database_changed();
void playlist_connection_changed(MpdObj *mi, int connect);

enum PlaylistColums {
	PL_COLUMN_ICON,
	PL_COLUMN_MARKUP,
	PL_COLUMN_ARTIST,
	PL_COLUMN_TRACK,
	PL_COLUMN_TITLEFILE,
	PL_COLUMN_ALBUM,	
	PL_COLUMN_GENRE,
	PL_COLUMN_COMPOSER,
	PL_COLUMN_LENGTH,
	PL_COLUMN_DISC,
	PL_COLUMN_COMMENT,
	PL_COLUMN_SONGPOS,
	PL_COLUMN_TOTAL
};

void pl3_update_go_menu();
#endif
