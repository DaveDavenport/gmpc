enum{
	SB_FPATH,
	SB_DPATH,
	SB_TYPE, /* 0 song, 1 folder */
	SB_PIXBUF,
	SB_EXPANDER,
	SB_NROWS
} sb_tree_store;


void song_browser_create();
void update_song_browser();
