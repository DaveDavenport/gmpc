/** main.c **/
extern int debug;
extern GladeXML *xml_main_window;
typedef struct
{
	gboolean do_popup;
	gboolean show_state;
	gint position;
	gint timeout;
	GdkPixbuf *gmpc_image;
	gint pixbuf_width, pixbuf_height;
	gboolean popup_stay;
}popup_struct;

typedef struct
{
	/* the mpd status struct used  in the whole program */
	mpd_Status *status;
	/* the mpd connection */
	mpd_Connection * connection;
	/* status about the connection */
	mpd_Stats *stats;
	/* connection lock, to prevent to functions to use the connection concurrent */
	gboolean conlock;
	/* playlist number this is to check if the playlist changed */
	int playlist_id;
	/* the state, if the state changes I know I have to update some stuff */
	int state;
	/* the volume if the volume change I also have to update some stuff */
	int volume;
	/* the current song */
	int song;
	/* Elapsed or remaining time */
	int time_format;
	/* The Playlist, only in my memory */
	GList *playlist;
	/* the current song */
	mpd_Song *cursong;
	/* playlist running. */
	gboolean playlist_running;
	/* filter enabled */
	gboolean show_filter;
	gchar filter_entry[256];
	gint  filter_field;
	/* Some internall data..  this to save trafic with mpd daemon */
	GtkListStore *playlist_list;
	GtkListStore *cur_list; /* this one should be empty..  no need to keep that in memory */
	GtkListStore *dir_list;
	GtkListStore *file_list;
	GtkListStore *id3_songs_list;
	GtkListStore *search_list;
	GtkTreeStore *id3_album_list;
	gchar path[1024];
	/* playlists view hidden */
	gboolean playlist_view_hidden;
	guint64 total_number_of_songs;
	guint64	total_playtime; 
	/* tray icon*/
	gboolean do_tray;
	popup_struct popup;
} internal_data;

enum{
	TIME_FORMAT_ELAPSED,
	TIME_FORMAT_REMAINING,
	TIME_FORMAT_PERCENTAGE
};

extern internal_data info;
extern guint update_timeout;
int update_interface();

/* callback.c */
int start_mpd_action();
int stop_mpd_action();
int load_playlist();
int check_for_errors();

/* mpdinteraction.c*/
int update_mpd_status();
int connect_to_mpd();
void play_song();
void stop_song();
void next_song();
void prev_song();
void random_pl();
void repeat_pl();
void update_mpd_dbase();
int disconnect_to_mpd();

/* player.c*/
/* scrolling title in main window */
typedef struct 	{	
	char *msg;
	char *base_msg;
	char *popup_msg;
	int pos;
	int up;
	gboolean exposed;
} scrollname;
extern scrollname scroll;
int update_player();
void create_player();
gboolean update_msg();
void msg_pop_popup();
void msg_push_popup();
void msg_set_base();

/* preferences.c */
typedef struct
{
	char host[256];
	int port;
	gboolean user_auth;
	char password[256];
	float timeout; /* seconds.ms */
	gboolean autoconnect;
} pref_struct;
extern pref_struct preferences;
void create_preferences_window();
void preferences_update();

/* playlist.c*/
void create_player();
void create_playlist();
void load_songs_with_filter();
extern GladeXML *xml_playlist_window;
void update_playlist();
void filter_toggle();
void destroy_playlist(GtkWidget *wid);
void clear_playlist_buffer();
/* id3info.c*/
void call_id3_window(int song);
/* config.c*/
void load_config();
void save_config();

/* do tray */
void update_tray_icon();
int create_tray_icon();

/* popup.c: update_popup() */
void update_popup();
void destroy_tray_icon();

