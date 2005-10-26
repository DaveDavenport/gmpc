#ifndef __PLAYLIST3_H__
#define __PLAYLIST3_H__

extern GtkTreeStore *pl3_tree;
extern GtkListStore *pl3_store;

void create_playlist3 ();
void pl3_update();
void pl3_push_statusbar_message(char *mesg);
void pl3_push_rsb_message(gchar *string);
void pl3_disconnect();
void pl3_cat_sel_changed();
int pl3_close();
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


/* for the tree in the right pane. */
enum pl3_store_types
{
	PL3_SONG_ID,
	PL3_SONG_POS,
	PL3_SONG_TITLE,
	PL3_WEIGHT_INT,
	PL3_SONG_STOCK_ID,
	PL3_UNKOWN, /* song id for pl3_store */
	PL3_NROWS
} pl3_store_type;

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
	NROWS
};

void pl3_playlist_search();

void pl3_playlist_changed();
void pl3_clear_playlist();
void pl3_show_song_info ();

int pl3_cat_get_selected_browser();

#endif
