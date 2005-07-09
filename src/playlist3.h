#ifndef __PLAYLIST3_H__
#define __PLAYLIST3_H__


void create_playlist3 ();
void pl3_update();
void pl3_push_statusbar_message(char *mesg);
void pl3_disconnect();
void pl3_cat_sel_changed();
int pl3_close();
int refilter_tree();


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


enum pl3_cat_store
{
	PL3_CAT_TYPE,
	PL3_CAT_TITLE,
	PL3_CAT_INT_ID,
	PL3_CAT_ICON_ID,
	PL3_CAT_PROC, /* for the lazy tree, if the dir is allready processed */
	PL3_CAT_ICON_SIZE,
	PL3_CAT_BROWSE_FORMAT,
	PL3_CAT_NROWS
} pl3_cat_store;

#define PL3_CUR_PLAYLIST 3
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
	PL3_UNKOWN, /* song id for pl3_store */
	PL3_YALIGN,
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
	WEIGHT_ENABLE,
	SONG_STOCK_ID,
	SONG_TIME,
	STOCK_ALIGN,
	NROWS
};

void pl3_playlist_search();

void pl3_playlist_changed();
void pl3_custom_tag_browser_add_single(GtkTreeIter *piter, char *title, char *format);
void pl3_custom_tag_browser_list_add(GtkTreeIter *iter);
void pl3_custom_tag_browser_reload();


#endif
