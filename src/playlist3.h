#ifndef __PLAYLIST3_H__
#define __PLAYLIST3_H__


void create_playlist3 ();
void pl3_update();
void pl3_push_statusbar_message(char *mesg);
void pl3_disconnect();
int pl3_close();


enum{
	PL3_CURRENT_PLAYLIST,
	PL3_BOOKMARKS,
	PL3_BROWSE_FILE,
	PL3_BROWSE_ARTIST,
	PL3_BROWSE_ALBUM,
	PL3_BROWSE_XIPH,
	PL3_BROWSE_CUSTOM_STREAM,
	PL3_FIND,
	PL3_NTYPES
	/* more space for options, like shoutcast */
} tree_type;


enum pl3_cat_store
{
	PL3_CAT_TYPE,
	PL3_CAT_TITLE,
	PL3_CAT_INT_ID,
	PL3_CAT_ICON_ID,
	PL3_CAT_PROC, /* for the lazy tree, if the dir is allready processed */
	PL3_CAT_ICON_SIZE,
	PL3_CAT_NROWS
} pl3_cat_store;

#define PL3_ENTRY_STREAM 2
#define PL3_ENTRY_PLAYLIST 1
#define PL3_ENTRY_SONG 0

/* for the tree in the right pane. */
enum pl3_store_types
{
	PL3_SONG_ID,
	PL3_SONG_POS,
	PL3_SONG_TITLE,
	PL3_WEIGHT_INT,
	PL3_WEIGHT_ENABLE,
	PL3_SONG_STOCK_ID,
	PL3_NROWS
} pl3_store_type;

gboolean toggle_playlist3(GtkToggleButton *tb);


void pl3_highlight_song();

enum store_types
{
	SONG_ID,
	SONG_POS,
	SONG_TITLE,
	WEIGHT_INT,
	WEIGHT_ENABLE,
	SONG_STOCK_ID,
	SONG_TIME,
	NROWS
};

































#endif
