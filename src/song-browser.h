enum{
	SB_FPATH,
	SB_DPATH,
	SB_TYPE, /* row_types*/
	SB_PIXBUF,
	SB_NROWS
} sb_tree_store;

enum {
	BROWSE_FILE,
	BROWSE_TAG,
	BROWSE_SEARCH,
	BROWSE_N	
} browse_types;

enum{
	ROW_SONG,
	ROW_CLOSE,
	ROW_ALBUM,
	ROW_OPEN,
	ROW_PLAYLIST,
	ROW_N	
}row_types;


void song_browser_create();
void update_song_browser();
void sb_reload_file_browser();
void sb_disconnect();
void sb_close();
