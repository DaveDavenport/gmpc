#ifndef __MPD_LIB__
#define __MPD_LIB__
#include "libmpdclient.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MPD_O_NOT_CONNECTED -2
#define MPD_O_FAILED_STATUS -3
#define MPD_O_LOCK_FAILED -4

enum {
	MPD_OB_PLAYER_PAUSE,
	MPD_OB_PLAYER_PLAY,
	MPD_OB_PLAYER_STOP,
	MPD_OB_PLAYER_UNKNOWN	
};

typedef struct _MpdObj 
{
	/* defines if we are connected */
	/* This should be made true if and only if the connection is up and running */
	short int connected;
	/* information needed to connect to mpd */
	char *hostname;
	int port;
	char *password;

	/* mpd's structures */
	mpd_Connection *connection;
	mpd_Status *status;
	mpd_Stats *stats;
	mpd_Song *CurrentSong;

	/* information needed to detect changes on mpd's side */
	long long playlistid;
	int songid;

	
	
	
	/* functions to call */
	void *(* playlist_changed)(struct _MpdObj *mi, int old_playlist_id, int new_playlist_id);	
	
	
	/* error message */
	char *error;	
	
	/* internal values */
	/* this "locks" the connections. so we can't have to commands competing with eachother */
	short int connection_lock;
	
}MpdObj;


MpdObj * 	mpd_ob_new_default			();
MpdObj * 	mpd_ob_new				(char *hostname, int port, char *password);
void 		mpd_ob_set_hostname			(MpdObj *mi, char *hostname);
void 		mpd_ob_set_password			(MpdObj *mi, char *hostname);
void 		mpd_ob_set_port				(MpdObj *mi, int port);
void 		mpd_ob_set_connection_timeout		(MpdObj *mi, float timeout);
int 		mpd_ob_connect				(MpdObj *mi);
int 		mpd_ob_disconnect			(MpdObj *mi);
int 		mpd_ob_check_connected			(MpdObj *mi);
/* signals */
void 		mpd_ob_signal_set_playlist_changed	(MpdObj *mi, void *(* playlist_changed)(MpdObj *mi, int old_playlist_id, int new_playlist_id));

/* status commands */
int 		mpd_ob_status_queue_update		(MpdObj *mi);
float 		mpd_ob_status_set_volume_as_float	(MpdObj *mi, float fvol);
int 		mpd_ob_status_set_volume		(MpdObj *mi,int volume);
int 		mpd_ob_status_get_volume		(MpdObj *mi);

/* player commands */
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


/* playlist command */
mpd_Song * 	mpd_ob_playlist_get_song		(MpdObj *mi, int songid);
mpd_Song * 	mpd_ob_playlist_get_current_song	(MpdObj *mi);
int 		mpd_ob_playlist_clear			(MpdObj *mi);
#endif
