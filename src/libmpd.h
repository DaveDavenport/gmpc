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

typedef struct _MpdInt 
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

	
	
	
	/* functions to call */
	void *(* playlist_changed)(struct _MpdInt *mi, int old_playlist_id, int new_playlist_id);	
	
	
	/* error message */
	char *error;	
	
	/* internal values */
	/* this "locks" the connections. so we can't have to commands competing with eachother */
	short int connection_lock;
	
}MpdInt;


MpdInt * 	mpd_ob_new_default			();
MpdInt * 	mpd_ob_new				(char *hostname, int port, char *password);
void 		mpd_ob_set_hostname			(MpdInt *mi, char *hostname);
void 		mpd_ob_set_password			(MpdInt *mi, char *hostname);
void 		mpd_ob_set_port				(MpdInt *mi, int port);
void 		mpd_ob_set_connection_timeout		(MpdInt *mi, float timeout);
int 		mpd_ob_connect				(MpdInt *mi);
int 		mpd_ob_disconnect			(MpdInt *mi);
int 		mpd_ob_check_connected			(MpdInt *mi);
/* signals */
void 		mpd_ob_signal_set_playlist_changed	(MpdInt *mi, void *(* playlist_changed)(MpdInt *mi, int old_playlist_id, int new_playlist_id));

/* status commands */
int 		mpd_ob_status_queue_update		(MpdInt *mi);
float 		mpd_ob_status_set_volume_as_float	(MpdInt *mi, float fvol);

/* player commands */
int 		mpd_ob_player_play			(MpdInt *mi);
int 		mpd_ob_player_stop			(MpdInt *mi);
int 		mpd_ob_player_next			(MpdInt *mi);
int 		mpd_ob_player_prev			(MpdInt *mi);
int 		mpd_ob_player_pause			(MpdInt *mi);
int 		mpd_ob_player_get_state			(MpdInt *mi);
int 		mpd_ob_player_get_current_song_id	(MpdInt *mi);
int 		mpd_ob_player_get_current_song_pos	(MpdInt *mi);
/* playlist command */
mpd_Song * 	mpd_ob_playlist_get_song		(MpdInt *mi, int songid);
mpd_Song * 	mpd_ob_playlist_get_current_song	(MpdInt *mi);

#endif
