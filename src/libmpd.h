#ifndef __MPD_LIB__
#define __MPD_LIB__
#include "libmpdclient.h"
#include <regex.h>
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MPD_O_NOT_CONNECTED -2
#define MPD_O_FAILED_STATUS -3
#define MPD_O_LOCK_FAILED -4
#define MPD_O_FAILED_STATS -5
#define MPD_O_ERROR -6
#define MPD_O_PLAYLIST_EXIST -10

#define	MPD_OB_PLAYER_PAUSE 	3
#define	MPD_OB_PLAYER_PLAY 	2
#define	MPD_OB_PLAYER_STOP 	1
#define	MPD_OB_PLAYER_UNKNOWN 	0


typedef struct _MpdQueue MpdQueue;

typedef struct _MpdObj 
{
	/* defines if we are connected */
	/* This should be made true if and only if the connection is up and running */
	short int 	connected;
	/* information needed to connect to mpd */
	char 		*hostname;
	int 		port;
	char 		*password;
	float 		connection_timeout;

	/* mpd's structures */
	mpd_Connection 	*connection;
	mpd_Status 	*status;
	mpd_Stats 	*stats;
	mpd_Song 	*CurrentSong;

	/* information needed to detect changes on mpd's side */
	long long 	playlistid;
	int 		songid;
	int 		state;
	unsigned long	dbUpdateTime;	
	int updatingDb;
	
	
	/* functions to call */
	void *(* playlist_changed)(struct _MpdObj *mi, int old_playlist_id, int new_playlist_id);	
	/* error signal */
	void *(* error_signal)(struct _MpdObj *mi, int id, char *msg, void *pointer);	
	void *error_signal_pointer;
	/* song change */
	void *(* song_changed)(struct _MpdObj *mi, int old_song_id, int new_song_id, void *pointer);	
	void *song_changed_signal_pointer;                                                     	
	/* song status changed */
	void *(* status_changed)(struct _MpdObj *mi, void *pointer);	
	void *status_changed_signal_pointer;                                                     	
	/* song status changed */
	void *(* state_changed)(struct _MpdObj *mi,int old_state,int new_state, void *pointer);	
	void *state_changed_signal_pointer;                                                     	

	/* disconnect signal */
	void *(* disconnect) (struct _MpdObj *mi, void *pointer);
	void *disconnect_pointer;

	/* connect signal */
	void *(* connect) (struct _MpdObj *mi, void *pointer);
	void *connect_pointer;
	
	/* error message */
	int error;
	char *error_msg;	

	/* song datab update */
	void *(* database_changed)(struct _MpdObj *mi,void *pointer);	
	void *database_changed_signal_pointer;                                                     	



	

	/* internal values */
	/* this "locks" the connections. so we can't have to commands competing with eachother */
	short int connection_lock;

	/* queue */
	MpdQueue *queue;

}MpdObj;


enum {
	MPD_DATA_TYPE_NONE,
	MPD_DATA_TYPE_ARTIST,
	MPD_DATA_TYPE_ALBUM,
	MPD_DATA_TYPE_DIRECTORY,
	MPD_DATA_TYPE_SONG,
	MPD_DATA_TYPE_PLAYLIST,
	MPD_DATA_TYPE_OUTPUT_DEV
} MpdDataType;


typedef struct _MpdData
{
	struct _MpdData *next;
	struct _MpdData *prev;
	struct _MpdData *first;

	/* MpdDataType */
	int type;

	union 
	{
		char *artist;
		char *album;
		char *directory;
		char *playlist; /*is a path*/
		mpd_Song *song;
		mpd_OutputEntity *output_dev; /* from devices */
	}value;
}MpdData;

MpdObj * 	mpd_ob_new_default			();
MpdObj * 	mpd_ob_new				(char *hostname, int port, char *password);
void 		mpd_ob_set_hostname			(MpdObj *mi, char *hostname);
void 		mpd_ob_set_password			(MpdObj *mi, char *hostname);
void 		mpd_ob_set_port				(MpdObj *mi, int port);
void 		mpd_ob_set_connection_timeout		(MpdObj *mi, float timeout);
int 		mpd_ob_connect				(MpdObj *mi);
int 		mpd_ob_disconnect			(MpdObj *mi);
int 		mpd_ob_check_connected			(MpdObj *mi);
int 		mpd_ob_check_error			(MpdObj *mi);
void 		mpd_ob_free				(MpdObj *mi);


/* signals */
void 		mpd_ob_signal_set_playlist_changed	(MpdObj *mi, void *(* playlist_changed)(MpdObj *mi, int old_playlist_id, int new_playlist_id));
void 		mpd_ob_signal_set_error			(MpdObj *mi, void *(* error_signal)(MpdObj *mi, int id, char *msg, void *pointer),void *pointer);
void 		mpd_ob_signal_set_song_changed		(MpdObj *mi, void *(* song_changed)(MpdObj *mi, int old_song_id, int new_song_id,void *pointer), void *pointer);
void 		mpd_ob_signal_set_status_changed	(MpdObj *mi, void *(* status_changed)(MpdObj *mi,void *pointer), void *pointer);
void 		mpd_ob_signal_set_state_changed 	(MpdObj *mi, void *(* state_changed)(MpdObj *mi, int old_state, int new_state, void *pointer),void *pointer);
void 		mpd_ob_signal_set_disconnect		(MpdObj *mi, void *(* disconnect)(MpdObj *mi, void *pointer),void *disconnect_pointer);
void 		mpd_ob_signal_set_connect		(MpdObj *mi, void *(* connect)(MpdObj *mi, void *pointer),void *connect_pointer);
void 		mpd_ob_signal_set_database_changed	(MpdObj *mi, void *(* database_changed)(MpdObj *mi, void *pointer), void *pointer);
/* status commands */
/* To get the function to have the  most recent info you want to call mpd_ob_status_queue_update 
 * In a gui app. you want to call this every 0.x seconds. 
 * mpd_ob_status_queue_update only queue's an update
 * Only when a function is called that needs status, it's fetched from mpd.
 */
int 		mpd_ob_status_queue_update		(MpdObj *mi);
float 		mpd_ob_status_set_volume_as_float	(MpdObj *mi, float fvol);
int 		mpd_ob_status_set_volume		(MpdObj *mi,int volume);
int 		mpd_ob_status_get_volume		(MpdObj *mi);
int		mpd_ob_status_get_total_song_time	(MpdObj *mi);
int		mpd_ob_status_get_elapsed_song_time	(MpdObj *mi);
int		mpd_ob_status_get_crossfade		(MpdObj *mi);
int		mpd_ob_status_set_crossfade		(MpdObj *mi, int crossfade_time);
int		mpd_ob_stats_update			(MpdObj *mi);

/* Player commands */
int 		mpd_ob_player_play			(MpdObj *mi);
int 		mpd_ob_player_play_id			(MpdObj *mi, int id);
int 		mpd_ob_player_stop			(MpdObj *mi);
int 		mpd_ob_player_next			(MpdObj *mi);
int 		mpd_ob_player_prev			(MpdObj *mi);
int 		mpd_ob_player_pause			(MpdObj *mi);
int 		mpd_ob_player_get_state			(MpdObj *mi);
int 		mpd_ob_player_get_current_song_id	(MpdObj *mi);
int 		mpd_ob_player_get_current_song_pos	(MpdObj *mi);
int		mpd_ob_player_get_repeat		(MpdObj *mi);
int		mpd_ob_player_set_repeat		(MpdObj *mi, int repeat);
int		mpd_ob_player_get_random		(MpdObj *mi);
int		mpd_ob_player_set_random		(MpdObj *mi, int random);
int 		mpd_ob_player_seek			(MpdObj *mi, int sec);

/* playlist command */
mpd_Song * 	mpd_ob_playlist_get_song		(MpdObj *mi, int songid);
mpd_Song * 	mpd_ob_playlist_get_current_song	(MpdObj *mi);
int 		mpd_ob_playlist_clear			(MpdObj *mi);
int 		mpd_ob_playlist_shuffle			(MpdObj *mi);
int 		mpd_ob_playlist_save			(MpdObj *mi, char *name);
void 		mpd_ob_playlist_update_dir		(MpdObj *mi, char *path);
void 		mpd_ob_playlist_move_pos		(MpdObj *mi, int old_pos, int new_pos);
MpdData * 	mpd_ob_playlist_get_artists		(MpdObj *mi);
MpdData *	mpd_ob_playlist_get_albums		(MpdObj *mi, char *artist);
MpdData * 	mpd_ob_playlist_get_directory		(MpdObj *mi,char *path);
MpdData * 	mpd_ob_playlist_find			(MpdObj *mi, int table, char *string, int exact);
MpdData * 	mpd_ob_playlist_get_changes		(MpdObj *mi,int old_playlist_id);
int		mpd_ob_playlist_get_playlist_length	(MpdObj *mi);
void		mpd_ob_playlist_add			(MpdObj *mi, char *path);
int 		mpd_ob_playlist_delete			(MpdObj *mi,char *path);
/* client side search function with best "match" option..
 * It splits the search string into tokens. (on the ' ')  every token is then matched using regex.
 * So f.e. Murder Hooker|Davis  matches songs where title/filename/artist/album contains murder and hooker or murder davis in any order.
 * Warning: This function can be slow.
 */
MpdData *	mpd_ob_playlist_token_find		(MpdObj *mi , char *string);
/* MpdData struct functions */
int 		mpd_ob_data_is_last			(MpdData *data);
void 		mpd_ob_free_data_ob			(MpdData *data);
MpdData * 	mpd_ob_data_get_next			(MpdData *data);
/* mpd ob data next will return NULL when there are no more items. it will also call free when called on the last item. */
/* if you don't want this check with mpd_ob_data_is_last before calling get_next */
/* this allows you to make this construction: */
/*	MpdData * mpd_ob_playlist_get_artists(..);
 * 	while(data != NULL)
 * 	{
 *
 *
 *		data = mpd_ob_data_next(data);
 * 	}
 */
 /* withouth leaking memory  */


/* queing stuff */
void 		mpd_ob_playlist_queue_add		(MpdObj *mi,char *path);
void 		mpd_ob_playlist_queue_load		(MpdObj *mi,char *path);
void 		mpd_ob_playlist_queue_delete_id		(MpdObj *mi,int id);
/* use these to commit the changes */
void 		mpd_ob_playlist_queue_commit		(MpdObj *mi);


/* Server Stuff */
MpdData * 	mpd_ob_server_get_output_devices	(MpdObj *mi);
int 		mpd_ob_server_set_output_device		(MpdObj *mi,int device_id,int state);
long unsigned	mpd_ob_server_get_database_update_time	(MpdObj *mi);
int 		mpd_ob_server_check_version		(MpdObj *mi, int major, int minor, int micro);

/* misc */
regex_t** 	mpd_misc_tokenize			(char *string);
void 		mpd_misc_tokens_free			(regex_t ** tokens);
#endif
