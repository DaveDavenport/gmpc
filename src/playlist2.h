void pl2_set_query(const gchar *query);
void create_playlist2();
void update_playlist2();
void init_playlist2();
void pl2_highlight_song();

void pl2_disconnect();
void pl2_connect();
int hide_playlist2 ();
gboolean toggle_playlist2(GtkToggleButton *tb);

void mw_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n);
gboolean mw_motion_cb (GtkWidget *tv, GdkEventMotion *event, gpointer null);

enum store_types
{
	SONG_ID,
	SONG_POS,
	SONG_TITLE,
	WEIGHT_INT,
	WEIGHT_ENABLE,
	SONG_STOCK_ID,
	NROWS
};
