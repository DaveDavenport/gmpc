#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "debug_printf.h"
#include "libmpd.h"


enum {
	MPD_QUEUE_ADD,
	MPD_QUEUE_LOAD,
	MPD_QUEUE_DELETE_ID
} MpdQueueType;

typedef struct _MpdQueue
{
	struct _MpdQueue *next;
	struct _MpdQueue *prev;
	struct _MpdQueue *first;

	/* what item to queue, (add/load/remove)*/
	int type;
	/* for adding files/load playlist/adding streams */
	char *path;
	/* for removing */
	int id;
}_MpdQueue;






/*************************************************************************************/
MpdObj * mpd_ob_create()
{
	MpdObj * mi = malloc(sizeof(MpdObj));
	if( mi == NULL )
	{
		/* should never happen on linux */
		return NULL;
	}

	
	/* set default values */
	mi->connected = FALSE;
	mi->port = 6600;
	mi->hostname = strdup("localhost");
	mi->password = NULL;
	mi->connection_timeout = 1.0;
	mi->connection = NULL;
	mi->status = NULL;
	mi->stats = NULL;
	mi->error = 0;
	mi->error_msg = NULL;
	mi->CurrentSong = NULL;
	/* info */
	mi->playlistid = -1;
	mi->songid = -1;
	mi->state = -1;
	mi->dbUpdateTime = 0;
	mi->updatingDb = 0;

	/* signals */
	mi->playlist_changed = NULL;
	/* song */
	mi->song_changed = NULL;
	mi->song_changed_signal_pointer = NULL;
	/* status */
	mi->status_changed = NULL;
	mi->status_changed_signal_pointer = NULL;
	/* state */
	mi->state_changed = NULL;
	mi->state_changed_signal_pointer = NULL;
	/* database changed */
	mi->database_changed = NULL;
	mi->state_changed_signal_pointer = NULL;
	/* disconnect signal */
	mi->disconnect = NULL;
	mi->disconnect_pointer = NULL;
	/* connect signal */
	mi->connect = NULL;
	mi->connect_pointer = NULL;	


	mi->error_signal = NULL;
	/* connection is locked because where not connected */
	mi->connection_lock = TRUE;

	mi->queue = NULL;
	
	return mi;
}

void mpd_ob_free(MpdObj *mi)
{
	debug_printf(DEBUG_INFO, "mpd_ob_free: destroying MpdObj object\n");
	if(mi->connected)
	{
		/* disconnect */
		mpd_ob_disconnect(mi);
		debug_printf(DEBUG_WARNING, "mpd_ob_free: Connection still running, disconnecting\n");
	}
	if(mi->hostname)
	{
		free(mi->hostname);
	}
	if(mi->password)
	{
		free(mi->password);
	}
	if(mi->error_msg)
	{
		free(mi->error_msg);
	}
	if(mi->connection)
	{
		/* obsolete */
		mpd_closeConnection(mi->connection);
	}
	if(mi->status)
	{
		mpd_freeStatus(mi->status);
	}
	if(mi->stats)
	{
		mpd_freeStats(mi->stats);
	}	
	if(mi->CurrentSong)
	{
		mpd_freeSong(mi->CurrentSong);
	}
	free(mi);
}

int mpd_ob_check_error(MpdObj *mi)
{
	if(mi == NULL)
	{
		return FALSE;
	}

	if(mi->error)
	{
		return TRUE;
	}	

	/* this shouldn't happen, ever */
	if(mi->connection == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_check_error: should this happen, mi->connection == NULL?");
		return FALSE;
	}
	if(mi->connection->error)
	{
		/* TODO: map these errors in the future */
		mi->error = mi->connection->error;
		mi->error_msg = strdup(mi->connection->errorStr);	

		debug_printf(DEBUG_ERROR, "mpd_ob_check_error: Following error occured: code: %i msg: %s", mi->error, mi->error_msg);
		mpd_ob_disconnect(mi);
		/* trigger signal for error */
		if(mi->error_signal)
		{
			mi->error_signal(mi, mi->error, mi->error_msg,mi->error_signal_pointer);
		}

		
		return TRUE;
	}

	return FALSE;
}



int mpd_ob_lock_conn(MpdObj *mi)
{
/*	debug_printf(DEBUG_INFO, "mpd_ob_lock_conn: Locking connection\n");
*/
	if(mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_lock_conn: Failed to lock connection, already locked\n");
		return TRUE;
	}
	mi->connection_lock = TRUE;
	return FALSE;
}

int mpd_ob_unlock_conn(MpdObj *mi)
{
/*	debug_printf(DEBUG_INFO, "mpd_ob_unlock_conn: unlocking connection\n");
 */
	if(!mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_unlock_conn: Failed to unlock connection, already unlocked\n");
		return FALSE;
	}
	
	mi->connection_lock = FALSE;

	return mpd_ob_check_error(mi);
}

MpdObj * mpd_ob_new_default()
{
	debug_printf(DEBUG_INFO, "mpd_ob_new_default: creating a new mpdInt object\n");
	return mpd_ob_create();
}

MpdObj *mpd_ob_new(char *hostname,  int port, char *password)
{
	MpdObj *mi = mpd_ob_create();
	if(mi == NULL)
	{
		return NULL;
	}
	if(hostname != NULL)
	{
		mpd_ob_set_hostname(mi, hostname);
	}
	if(port != 0)
	{
		mpd_ob_set_port(mi, port);
	}
	if(password != NULL)
	{
		mpd_ob_set_password(mi, password);
	}
	return mi;
}


void mpd_ob_set_hostname(MpdObj *mi, char *hostname)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_hostname: mi == NULL\n");
		return;
	}

	if(mi->hostname != NULL)
	{
		free(mi->hostname);
	}
	/* possible location todo some post processing of hostname */
	mi->hostname = strdup(hostname);
}

void mpd_ob_set_password(MpdObj *mi, char *password)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_password: mi == NULL\n");
		return;
	}

	if(mi->password != NULL)
	{
		free(mi->password);
	}
	/* possible location todo some post processing of password */
	mi->password = strdup(password);
}

void mpd_ob_set_port(MpdObj *mi, int port)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_port: mi == NULL\n");
		return;
	}
	mi->port = port;
}

void mpd_ob_set_connection_timeout(MpdObj *mi, float timeout)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_connection_timeout: mi == NULL\n");
		return;
	}
	mi->connection_timeout = timeout;
	if(mpd_ob_check_connected(mi))
	{
		/*TODO: set timeout */	
		if(mpd_ob_lock_conn(mi))
		{
			debug_printf(DEBUG_WARNING,"mpd_ob_set_connection_timeout: lock failed\n");
			return;
		}
		mpd_setConnectionTimeout(mi->connection, timeout);
		mpd_finishCommand(mi->connection);

		mpd_ob_unlock_conn(mi);

	}
}


int mpd_ob_disconnect(MpdObj *mi)
{
	/* set disconnect flag */
	mi->connected = 0;
	/* lock */
	mpd_ob_lock_conn(mi);
	debug_printf(DEBUG_INFO, "mpd_ob_disconnect: disconnecting\n");

	if(mi->connection)
	{
		mpd_closeConnection(mi->connection);
		mi->connection = NULL;
	}
	if(mi->status)
	{
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
	if(mi->stats)
	{
		mpd_freeStats(mi->stats);
		mi->stats = NULL;
	}
	if(mi->CurrentSong)
	{
		mpd_freeSong(mi->CurrentSong);
		mi->CurrentSong = NULL;
	}
	mi->playlistid = -1;
	mi->state = -1;
	mi->songid = -1;
	mi->dbUpdateTime = 0;
	mi->updatingDb = 0;

	/*don't reset errors */

	if(mi->disconnect != NULL)
	{                                                                      		
		mi->disconnect(mi, mi->disconnect_pointer);
	}                                                                                           		

	return FALSE;
}

int mpd_ob_connect(MpdObj *mi)
{
	if(mi == NULL)
	{
		/* should return some spiffy error here */
		return -1;
	}
	/* reset errors */
	mi->error = 0;
	if(mi->error_msg != NULL)
	{
		free(mi->error_msg);
	}
	mi->error_msg = NULL;

	debug_printf(DEBUG_INFO, "mpd_ob_connect: connecting\n");

	if(mi->connected)
	{
		/* disconnect */
		mpd_ob_disconnect(mi);
	}

	if(mi->hostname == NULL)
	{
		mpd_ob_set_hostname(mi, "localhost");
	}
	/* make sure this is true */
	if(!mi->connection_lock)
	{
		mpd_ob_lock_conn(mi);
	}
	/* make timeout configurable */
	mi->connection = mpd_newConnection(mi->hostname,mi->port,mi->connection_timeout);
	if(mi->connection == NULL)
	{
		/* again spiffy error here */
		return -1;
	}
	if(mi->password != NULL && strlen(mi->password) > 0)
	{
		mpd_sendPasswordCommand(mi->connection, mi->password);	
		mpd_finishCommand(mi->connection);
		/* TODO: check if succesfull */
	}	


	/* set connected state */
	mi->connected = TRUE;
	if(mpd_ob_unlock_conn(mi))
	{
		return -1;
	}
	if(mi->connect != NULL)
	{                                                                      		
		mi->connect(mi, mi->connect_pointer);
	}                                                                                           		


	return 0;
}

int mpd_ob_check_connected(MpdObj *mi)
{
	if(mi == NULL)
	{
		return FALSE;
	}
	return mi->connected;
}

int mpd_ob_status_queue_update(MpdObj *mi)
{

	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_ob_status_queue_update: Where not connected\n");
		return TRUE;
	}                                       	
	if(mi->status != NULL)
	{                                  	
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
	return FALSE;
}


int mpd_ob_status_update(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_ob_status_update: Where not connected\n");
		return TRUE;
	}
	if(mpd_ob_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_status_set_volume: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}


	if(mi->status != NULL)
	{
		mpd_freeStatus(mi->status);
	}
	mpd_sendStatusCommand(mi->connection);
	mi->status = mpd_getStatus(mi->connection);
	if(mi->status == NULL)
	{
		debug_printf(DEBUG_ERROR,"mpd_ob_status_update: Failed to grab status from mpd\n");
		mpd_ob_unlock_conn(mi);
		return FALSE;
	}
	if(mpd_ob_unlock_conn(mi))
	{
		return TRUE;
	}
	/*
	 * check for changes 
	 */

	if(mi->playlistid != mi->status->playlist)
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "mpd_ob_status_update: Playlist has changed!");

		/* TODO: Call defined functions */
		if(mi->playlist_changed != NULL)
		{
			mi->playlist_changed(mi, mi->playlistid, mi->status->playlist);
		}
		if(!mpd_ob_check_connected(mi))
		{
			return TRUE;

		}


		/* We can't trust the current song anymore. so we remove it */
		/* tags might have been updated */
		if(mi->CurrentSong != NULL)
		{
			mpd_freeSong(mi->CurrentSong);
			mi->CurrentSong = NULL;
		}

		/* save new id */
		mi->playlistid = mi->status->playlist;
	}


	/* playlist change */
	if(mi->state != mi->status->state)
	{
		/* TODO: Call defined functions */
		if(mi->state_changed != NULL)
		{                                                                      		
			mi->state_changed(mi, mi->state,mi->status->state,mi->state_changed_signal_pointer);
		}                                                                                           		

		mi->state = mi->status->state;
	}

	if(mi->songid != mi->status->songid)
	{
		/* print debug message */
		debug_printf(DEBUG_INFO, "mpd_ob_status_update: Song has changed %i %i!", mi->songid, mi->status->songid);

		/* TODO: Call defined functions */
		if(mi->song_changed != NULL)
		{                                                                      		
			mi->song_changed(mi, mi->songid,mi->status->songid,mi->song_changed_signal_pointer);
		}
		/* save new songid */
		mi->songid = mi->status->songid;

	}
	


	if(mi->status_changed != NULL)
	{                                                                      		
		mi->status_changed(mi, mi->status_changed_signal_pointer);		
	}


	if(mi->status->updatingDb != mi->updatingDb)
	{
		if(!mi->status->updatingDb)
		{
			mpd_ob_stats_update(mi);
		}
		mi->updatingDb = mi->status->updatingDb;
	}

	return FALSE;
}

/* returns TRUE when status is availible, when not availible and connected it tries to grab it */
int mpd_ob_status_check(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("not connected\n");
		return FALSE;
	}
	if(mi->status == NULL)
	{
		/* try to update */
		if(mpd_ob_status_update(mi))
		{
			printf("failed to update status\n");
			return FALSE;
		}
	}
	return TRUE;
}


int mpd_ob_status_get_volume(MpdObj *mi)
{
	if(mi == NULL)
	{
		printf("failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}
	return mi->status->volume;
}

int mpd_ob_status_get_total_song_time(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}
	return mi->status->totalTime;
}


int mpd_ob_status_get_elapsed_song_time(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("failed to check mi == NULL\n");
		return -2;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}
	return mi->status->elapsedTime;
}

int mpd_ob_status_set_volume(MpdObj *mi,int volume)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_status_set_volume: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	/* making sure volume is between 0 and 100 */
	volume = (volume < 0)? 0:(volume>100)? 100:volume;

	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_status_set_volume: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	/* send the command */
	mpd_sendSetvolCommand(mi->connection , volume);
	mpd_finishCommand(mi->connection);
	/* check for errors */

	mpd_ob_unlock_conn(mi);
	/* update status, because we changed it */
	mpd_ob_status_queue_update(mi);
	/* return current volume */
	return mpd_ob_status_get_volume(mi);
}

int mpd_ob_status_get_crossfade(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_status_get_crossfade: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("mpd_ob_status_get_crossfade: Failed grabbing status\n");
		return MPD_O_NOT_CONNECTED;
	}
	return mi->status->crossfade;
}

int mpd_ob_status_set_crossfade(MpdObj *mi,int crossfade_time)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_set_crossfade: not connected\n");	
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_set_crossfade: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}
	mpd_sendCrossfadeCommand(mi->connection, crossfade_time);
	mpd_finishCommand(mi->connection);

	mpd_ob_unlock_conn(mi);
	mpd_ob_status_queue_update(mi);
	return FALSE;
}

/*******************************************************************************************
 * PLAYER
 */

int mpd_ob_player_get_state(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_get_state: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{                                        	
		printf("Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}                                        

	switch(mi->status->state)
	{
		case MPD_STATUS_STATE_PLAY:
			return MPD_OB_PLAYER_PLAY;
		case MPD_STATUS_STATE_STOP:
			return MPD_OB_PLAYER_STOP;
		case MPD_STATUS_STATE_PAUSE:
			return MPD_OB_PLAYER_PAUSE;
		default:
			return MPD_OB_PLAYER_UNKNOWN;
	}
	return MPD_OB_PLAYER_UNKNOWN;
}

int mpd_ob_player_get_current_song_id(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_player_get_state: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{                                        	          	
		debug_printf(DEBUG_ERROR,"Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}                
	/* check if in valid state */	
	if(mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PLAY &&
			mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PAUSE)
	{
		return -1;
	}
	/* just to be sure check */	
	if(!mi->status->playlistLength)
	{
		return -1;
	}
	return mi->status->songid;
}

int mpd_ob_player_get_current_song_pos(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_player_get_state: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{                                        	          	
		debug_printf(DEBUG_ERROR,"Failed to get status\n");
		return MPD_O_FAILED_STATUS;
	}                
	/* check if in valid state */	
	if(mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PLAY &&
			mpd_ob_player_get_state(mi) != MPD_OB_PLAYER_PAUSE)
	{
		return -1;
	}
	/* just to be sure check */	
	if(!mi->status->playlistLength)
	{
		return -1;
	}
	return mi->status->song;
}




float mpd_ob_status_set_volume_as_float(MpdObj *mi, float fvol)
{
	int volume = mpd_ob_status_set_volume(mi, (int)(fvol*100.0));
	if(volume > -1)
	{
		return (float)volume/100.0;
	}
	return (float)volume;
}



int mpd_ob_player_play_id(MpdObj *mi, int id)
{
	debug_printf(DEBUG_INFO, "mpd_ob_player_play_id: trying to play id: %i\n", id);
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendPlayIdCommand(mi->connection,id);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_play(MpdObj *mi)
{
	return mpd_ob_player_play_id(mi, -1);
}

int mpd_ob_player_stop(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendStopCommand(mi->connection);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_next(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendNextCommand(mi->connection);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_prev(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendPrevCommand(mi->connection);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}


int mpd_ob_player_pause(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	if(mpd_ob_player_get_state(mi) == MPD_OB_PLAYER_PAUSE)
	{
		mpd_sendPauseCommand(mi->connection,0);
		mpd_finishCommand(mi->connection);
	}
	else if (mpd_ob_player_get_state(mi) == MPD_OB_PLAYER_PLAY)
	{
		mpd_sendPauseCommand(mi->connection,1);
		mpd_finishCommand(mi->connection);
	}


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_seek(MpdObj *mi, int sec)
{
	int cur_song  = mpd_ob_player_get_current_song_pos(mi);
	if( cur_song < 0 )
	{
		printf("mpd_ob_player_seek: failed to get current song pos\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_play: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_play: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	debug_printf(DEBUG_INFO, "mpd_ob_player_seek: seeking in song %i to %i sec\n", cur_song,sec);

	mpd_sendSeekCommand(mi->connection, cur_song,sec);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_get_repeat(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_get_repeat: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("mpd_ob_player_get_repeat: Failed grabbing status\n");
		return MPD_O_NOT_CONNECTED;
	}
	return mi->status->repeat;
}


















































int mpd_ob_player_set_repeat(MpdObj *mi,int repeat)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_set_repeat: not connected\n");	
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_set_repeat: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}
	mpd_sendRepeatCommand(mi->connection, repeat);
	mpd_finishCommand(mi->connection);

	mpd_ob_unlock_conn(mi);
	mpd_ob_status_queue_update(mi);
	return FALSE;
}



int mpd_ob_player_get_random(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_get_random: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("mpd_ob_player_get_random: Failed grabbing status\n");
		return MPD_O_NOT_CONNECTED;
	}
	return mi->status->random;
}


int mpd_ob_player_set_random(MpdObj *mi,int random)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_set_random: not connected\n");	
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_player_set_random: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}
	mpd_sendRandomCommand(mi->connection, random);
	mpd_finishCommand(mi->connection);

	mpd_ob_unlock_conn(mi);
	mpd_ob_status_queue_update(mi);
	return FALSE;
}

int mpd_ob_playlist_get_playlist_length(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_player_get_playlist_length: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_status_check(mi))
	{
		printf("mpd_ob_player_get_playlist_length: Failed grabbing status\n");
		return MPD_O_NOT_CONNECTED;
	}
	return mi->status->playlistLength;
}

void mpd_ob_playlist_add(MpdObj *mi, char *path)
{
	mpd_ob_playlist_queue_add(mi, path);
	mpd_ob_playlist_queue_commit(mi);
}










/*******************************************************************************
 * PLAYLIST 
 */
mpd_Song * mpd_ob_playlist_get_song(MpdObj *mi, int songid)
{
	mpd_Song *song = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_song: Not Connected\n");
		return NULL;
	}

	if(mpd_ob_lock_conn(mi))
	{
		return NULL;
	}
	debug_printf(DEBUG_INFO, "mpd_ob_playlist_get_song: Trying to grab song with id: %i\n", songid);
	mpd_sendPlaylistIdCommand(mi->connection, songid);
	ent = mpd_getNextInfoEntity(mi->connection);
	mpd_finishCommand(mi->connection);

	if(mpd_ob_unlock_conn(mi))
	{
		/*TODO free entity. for now this can never happen */
		return NULL;
	}                         	

	if(ent == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_song: Failed to grab song from mpd\n");
		return NULL;
	}

	if(ent->type != MPD_INFO_ENTITY_TYPE_SONG)
	{
		mpd_freeInfoEntity(ent);
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_song: Failed to grab corect song type from mpd\n");
		return NULL;
	}
	song = mpd_songDup(ent->info.song);
	mpd_freeInfoEntity(ent);

	return song;
}


mpd_Song * mpd_ob_playlist_get_current_song(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_current_song: Not Connected\n");
		return NULL;
	}

	if(!mpd_ob_status_check(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_current_song: Failed to check status\n");
		return NULL; 
	}

	if(mi->CurrentSong != NULL && mi->CurrentSong->id != mi->status->songid)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_playlist_get_current_song: Current song not up2date, updating\n");
		mpd_freeSong(mi->CurrentSong);
		mi->CurrentSong = NULL;
	}

	if(mi->CurrentSong == NULL)
	{
		/* TODO: this to use the geT_current_song_id function */
		mi->CurrentSong = mpd_ob_playlist_get_song(mi, mpd_ob_player_get_current_song_id(mi));
		if(mi->CurrentSong == NULL)
		{
			debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_current_song: Failed to grab song\n");
			return NULL;
		}
	}
	return mi->CurrentSong;
}

int mpd_ob_playlist_delete(MpdObj *mi,char *path)
{
	if(path == NULL)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_playlist_delete: path == NULL");
		return MPD_O_ERROR;
	}
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_delete: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_delete: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendRmCommand(mi->connection,path);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	return FALSE;
}



int mpd_ob_playlist_clear(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_clear: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_clear: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendClearCommand(mi->connection);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	return FALSE;
}

int mpd_ob_playlist_shuffle(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_shuffle: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_shuffle: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendShuffleCommand(mi->connection);
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	return FALSE;

}

int mpd_ob_playlist_save(MpdObj *mi, char *name)
{
	if(name == NULL || !strlen(name))
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_playlist_save: name != NULL  and strlen(name) > 0 failed");
		return MPD_O_ERROR;
	}
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_playlist_save: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"mpd_ob_playlist_save: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	mpd_sendSaveCommand(mi->connection,name);
	mpd_finishCommand(mi->connection);
	if(mi->connection->error == MPD_ERROR_ACK && mi->connection->errorCode == MPD_ACK_ERROR_EXIST)
	{
		mpd_clearError(mi->connection);
		mpd_ob_unlock_conn(mi);	
		return MPD_O_PLAYLIST_EXIST; 

	}

	/* unlock */                                               	
	mpd_ob_unlock_conn(mi);
	return FALSE;
}

void mpd_ob_playlist_update_dir(MpdObj *mi, char *path)
{
	if(path == NULL || !strlen(path))
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_playlist_update_dir: path != NULL  and strlen(path) > 0 failed");
		return;
	}
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_playlist_update_dir: not connected\n");
		return;
	}
	if(mpd_ob_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"mpd_ob_playlist_update_dir: lock failed\n");
		return;
	}

	mpd_sendUpdateCommand(mi->connection,path);
	mpd_finishCommand(mi->connection);

	/* unlock */                                               	
	mpd_ob_unlock_conn(mi);
	return;
}


void mpd_ob_playlist_move_pos(MpdObj *mi, int old_pos, int new_pos)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_playlist_move_pos: not connected\n");
		return;
	}
	if(mpd_ob_lock_conn(mi))
	{
		debug_printf(DEBUG_ERROR,"mpd_ob_playlist_move_pos: lock failed\n");
		return;
	}

	mpd_sendMoveCommand(mi->connection,old_pos, new_pos);
	mpd_finishCommand(mi->connection);

	/* unlock */                                               	
	mpd_ob_unlock_conn(mi);
	return;
}


void mpd_ob_signal_set_connect (MpdObj *mi, void *(* connect)(MpdObj *mi, void *pointer), void *connect_pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_connect: MpdObj *mi == NULL");
		return;
	}
	mi->connect = connect;
	mi->connect_pointer = connect_pointer;
}                                                                                                                                     /* SIGNALS */


void mpd_ob_signal_set_disconnect (MpdObj *mi, void *(* disconnect)(MpdObj *mi, void *pointer), void *disconnect_pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_disconnect: MpdObj *mi == NULL");
		return;
	}
	mi->disconnect = disconnect;
	mi->disconnect_pointer = disconnect_pointer;
}                                                                                                                                     /* SIGNALS */

void mpd_ob_signal_set_playlist_changed (MpdObj *mi, void *(* playlist_changed)(MpdObj *mi, int old_playlist_id, int new_playlist_id))
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_playlist_changed: MpdObj *mi == NULL");
		return;
	}
	mi->playlist_changed = playlist_changed;
}


void mpd_ob_signal_set_song_changed (MpdObj *mi, void *(* song_changed)(MpdObj *mi, int old_song_id, int new_song_id, void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_song_changed: MpdObj *mi == NULL");
		return;
	}
	mi->song_changed = song_changed;
	mi->song_changed_signal_pointer = pointer;
}


void mpd_ob_signal_set_state_changed (MpdObj *mi, void *(* state_changed)(MpdObj *mi, int old_state, int new_state, void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_state_changed: MpdObj *mi == NULL");
		return;
	}
	mi->state_changed = state_changed;
	mi->state_changed_signal_pointer = pointer;
}

void mpd_ob_signal_set_database_changed (MpdObj *mi, void *(* database_changed)(MpdObj *mi, void *pointer), void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_database_changed: MpdObj *mi == NULL");
		return;
	}
	mi->database_changed = database_changed;
	mi->database_changed_signal_pointer = pointer;
}




void mpd_ob_signal_set_status_changed (MpdObj *mi, void *(* status_changed)(MpdObj *mi, void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_state_changed: MpdObj *mi == NULL");
		return;
	}
	mi->status_changed = status_changed;
	mi->status_changed_signal_pointer = pointer;
}


void mpd_ob_signal_set_error (MpdObj *mi, void *(* error_signal)(MpdObj *mi, int id, char *msg,void *pointer),void *pointer)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_signal_set_error: MpdObj *mi == NULL");
		return;
	}
	mi->error_signal = error_signal;
	mi->error_signal_pointer = pointer;
}



/* more playlist */
/* MpdData Part */
MpdData *mpd_ob_new_data_struct()
{
	MpdData* data = malloc(sizeof(MpdData));

	data->type = 0;

	data->value.artist = NULL;
	data->value.album = NULL;
	data->value.song = NULL;

	return data;	
}



MpdData * mpd_ob_playlist_get_artists(MpdObj *mi)
{
	char *string = NULL;
	MpdData *data = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_get_artists: not connected\n");
		return NULL;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_get_artists: lock failed\n");
		return NULL;
	}

	mpd_sendListCommand(mi->connection,MPD_TABLE_ARTIST,NULL);
	while (( string = mpd_getNextArtist(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_ob_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;

		}	
		else
		{
			data->next = mpd_ob_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = MPD_DATA_TYPE_ARTIST; 
		data->value.artist = string;
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}















MpdData * mpd_ob_data_get_next(MpdData *data)
{
	if(data == NULL || data->next != NULL)
	{
		return data->next;
	}
	else if(data->next == NULL)
	{
		mpd_ob_free_data_ob(data);
		return NULL;
	}
	return data;	
}

int mpd_ob_data_is_last(MpdData *data)
{
	if(data == NULL || data->next == NULL)
	{
		return TRUE;
	}
	return FALSE;	
}


/* clean this up.. make one while look */
void mpd_ob_free_data_ob(MpdData *data)
{
	MpdData *temp = NULL;
	if(data == NULL)
	{
		return;
	}
	data = data->first;	
	while(data != NULL)
	{
		temp = data->next;
		if(data->type == MPD_DATA_TYPE_ARTIST)
		{
			free(data->value.artist);
		}
		else if (data->type == MPD_DATA_TYPE_ALBUM)
		{
			free(data->value.album);
		}
		else if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			free(data->value.directory);
		}
		else if (data->type == MPD_DATA_TYPE_SONG)
		{
			mpd_freeSong(data->value.song);
		}
		else if (data->type == MPD_DATA_TYPE_PLAYLIST)
		{
			free(data->value.playlist);
		}
		else if (data->type == MPD_DATA_TYPE_OUTPUT_DEV)
		{
			mpd_freeOutputElement(data->value.output_dev);
		}

		free(data);
		data= temp;
	}
}

/* */

MpdData * mpd_ob_playlist_get_albums(MpdObj *mi,char *artist)
{
	char *string = NULL;
	MpdData *data = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_get_albums: not connected\n");
		return NULL;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_get_albums: lock failed\n");
		return NULL;
	}

	mpd_sendListCommand(mi->connection,MPD_TABLE_ALBUM,artist);
	while (( string = mpd_getNextAlbum(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_ob_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;
		}	
		else
		{
			data->next = mpd_ob_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = MPD_DATA_TYPE_ALBUM;
		data->value.album = string;
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}




MpdData * mpd_ob_playlist_get_directory(MpdObj *mi,char *path)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_get_albums: not connected\n");
		return NULL;
	}
	if(path == NULL)
	{
		path = "/";
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_get_albums: lock failed\n");
		return NULL;
	}

	mpd_sendLsInfoCommand(mi->connection,path);
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_ob_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;
		}	
		else
		{
			data->next = mpd_ob_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->value.directory = strdup(ent->info.directory->path);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;
			data->value.song = mpd_songDup(ent->info.song);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->value.playlist = strdup(ent->info.playlistFile->path);

		}

		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}

MpdData *mpd_ob_playlist_token_find(MpdObj *mi , char *string)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	char ** strdata = NULL;
	char *searchstr = NULL;
	int i=0;
	regex_t pattern;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_find: not connected\n");
		return NULL;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_find: lock failed\n");
		return NULL;
	}

	if(string == NULL || !strlen(string) )
	{
		debug_printf(DEBUG_INFO, "no string found");
		mpd_ob_unlock_conn(mi);
		return NULL;
	}
	else{
		char *pstring, *temp;
		i =1;
		searchstr = pstring = strdup(string);
		do{
			temp = strsep(&pstring, " ");
			strdata = realloc(strdata, (i+1)*sizeof(char *));
			strdata[i-1]= temp;
			strdata[i] = NULL;
			i++;
		}while(pstring != NULL);	
	}	
	if(strdata == NULL)
	{
		mpd_ob_unlock_conn(mi);
		debug_printf(DEBUG_INFO, "no split string found");
		return NULL;
	}

	mpd_sendListallInfoCommand(mi->connection, "/");
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{	
		if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			int i = 0;
			int match = 0;
			int loop = 1;
			for(i=0; strdata[i] != NULL && loop; i++)
			{
				match = 0;
				if(regcomp(&pattern, strdata[i], REG_EXTENDED|REG_ICASE))
				{
					printf("test\n");
					loop = 0;
					break;
				}
				if(ent->info.song->file && !regexec(&pattern,ent->info.song->file, 0, NULL, 0))
				{
					match = 1;
				}
				else if(ent->info.song->artist && !regexec(&pattern,ent->info.song->artist, 0, NULL, 0))
				{
					match = 1;
				}
				else if(ent->info.song->title && !regexec(&pattern,ent->info.song->title, 0, NULL, 0)) 
				{
					match = 1;
				}
				else if(ent->info.song->album && !regexec(&pattern,ent->info.song->album, 0, NULL, 0))
				{
					match = 1;                                                   				
				}
				regfree(&pattern);
				if(!match)
				{

					loop = 0;

				}
			}

			if(match)
			{
				if(data == NULL)
				{
					data = mpd_ob_new_data_struct();
					data->first = data;
					data->next = NULL;
					data->prev = NULL;
				}	
				else
				{
					data->next = mpd_ob_new_data_struct();
					data->next->first = data->first;
					data->next->prev = data;
					data = data->next;
					data->next = NULL;
				}
				data->type = MPD_DATA_TYPE_SONG;
				data->value.song = mpd_songDup(ent->info.song);				
			}
		}
		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);
	free(searchstr);
	free(strdata);
	mpd_ob_unlock_conn(mi);
	if(data == NULL)
	{
		return NULL;
	}
	return data->first;
}


MpdData * mpd_ob_playlist_find(MpdObj *mi, int table, char *string, int exact)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_find: not connected\n");
		return NULL;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_find: lock failed\n");
		return NULL;
	}
	if(exact)
	{
		mpd_sendFindCommand(mi->connection,table,string);
	}
	else
	{
		mpd_sendSearchCommand(mi->connection, table,string);
	}
	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_ob_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;
		}	
		else
		{
			data->next = mpd_ob_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			data->type = MPD_DATA_TYPE_DIRECTORY;
			data->value.directory = strdup(ent->info.directory->path);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			data->type = MPD_DATA_TYPE_SONG;
			data->value.song = mpd_songDup(ent->info.song);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			data->type = MPD_DATA_TYPE_PLAYLIST;
			data->value.playlist = strdup(ent->info.playlistFile->path);
		}

		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}


MpdData * mpd_ob_playlist_get_changes(MpdObj *mi,int old_playlist_id)
{
	MpdData *data = NULL;
	mpd_InfoEntity *ent = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_get_albums: not connected\n");
		return NULL;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_get_albums: lock failed\n");
		return NULL;
	}

	if(old_playlist_id == -1)
	{
		mpd_sendPlaylistIdCommand(mi->connection, -1);
	}
	else
	{
		mpd_sendPlChangesCommand (mi->connection, old_playlist_id);
	}

	while (( ent = mpd_getNextInfoEntity(mi->connection)) != NULL)
	{	
		if(ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{	
			if(data == NULL)
			{
				data = mpd_ob_new_data_struct();
				data->first = data;
				data->next = NULL;
				data->prev = NULL;
			}	
			else
			{
				data->next = mpd_ob_new_data_struct();
				data->next->first = data->first;
				data->next->prev = data;
				data = data->next;
				data->next = NULL;
			}
			data->type = MPD_DATA_TYPE_SONG;
			data->value.song = mpd_songDup(ent->info.song);
		}
		mpd_freeInfoEntity(ent);
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	if(mpd_ob_unlock_conn(mi))
	{
		mpd_ob_free_data_ob(data);
		return NULL;
	}
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}




/* clean this up.. make one while look */
void mpd_ob_free_queue_ob(MpdObj *mi)
{
	MpdQueue *temp = NULL;
	if(mi->queue == NULL)
	{
		return;
	}	
	mi->queue = mi->queue->first;
	while(mi->queue != NULL)
	{
		temp = mi->queue->next;

		if(mi->queue->path != NULL)
		{
			free(mi->queue->path);
		}

		free(mi->queue);
		mi->queue = temp;
	}
	mi->queue = NULL;

}

MpdQueue *mpd_ob_new_queue_struct()
{
	MpdQueue* queue = malloc(sizeof(MpdQueue));

	queue->type = 0;
	queue->path = NULL;
	queue->id = 0;

	return queue;	
}


void mpd_ob_queue_get_next(MpdObj *mi)
{
	if(mi->queue != NULL && mi->queue->next != NULL)
	{
		mi->queue = mi->queue->next;
	}
	else if(mi->queue->next == NULL)
	{
		mpd_ob_free_queue_ob(mi);
		mi->queue = NULL;
	}
}

void mpd_ob_playlist_queue_add(MpdObj *mi,char *path)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_add: not connected\n");
		return;
	}
	if(path == NULL)
	{
		return;
	}

	if(mi->queue == NULL)
	{
		mi->queue = mpd_ob_new_queue_struct();
		mi->queue->first = mi->queue;
		mi->queue->next = NULL;
		mi->queue->prev = NULL;
	}	
	else
	{
		mi->queue->next = mpd_ob_new_queue_struct();
		mi->queue->next->first = mi->queue->first;
		mi->queue->next->prev = mi->queue;
		mi->queue = mi->queue->next;
		mi->queue->next = NULL;
	}
	mi->queue->type = MPD_QUEUE_ADD; 
	mi->queue->path = strdup(path);
}

void mpd_ob_playlist_queue_load(MpdObj *mi,char *path)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_add: not connected\n");
		return;
	}
	if(path == NULL)
	{
		return;
	}

	if(mi->queue == NULL)
	{
		mi->queue = mpd_ob_new_queue_struct();
		mi->queue->first = mi->queue;
		mi->queue->next = NULL;
		mi->queue->prev = NULL;
	}	
	else
	{
		mi->queue->next = mpd_ob_new_queue_struct();
		mi->queue->next->first = mi->queue->first;
		mi->queue->next->prev = mi->queue;
		mi->queue = mi->queue->next;
		mi->queue->next = NULL;
	}
	mi->queue->type = MPD_QUEUE_LOAD; 
	mi->queue->path = strdup(path);
}


void mpd_ob_playlist_queue_commit(MpdObj *mi)
{
	if(mi->queue == NULL)
	{
		return;
	}
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_add: not connected\n");
		return;
	}                                                      	
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_find: lock failed\n");
		return ;
	}                                
	mpd_sendCommandListBegin(mi->connection);		
	/* get first item */
	mi->queue = mi->queue->first;
	while(mi->queue != NULL)
	{
		if(mi->queue->type == MPD_QUEUE_ADD)
		{
			if(mi->queue->path != NULL)
			{
				mpd_sendAddCommand(mi->connection, mi->queue->path);
			}
		}	
		else if(mi->queue->type == MPD_QUEUE_LOAD)
		{
			if(mi->queue->path != NULL)
			{                                                           			
				mpd_sendLoadCommand(mi->connection, mi->queue->path);			
			}
		}
		else if (mi->queue->type == MPD_QUEUE_DELETE_ID)
		{
			if(mi->queue->id >= 0)
			{                                                           						
				mpd_sendDeleteIdCommand(mi->connection, mi->queue->id);			
			}                                                                               		
		}
		mpd_ob_queue_get_next(mi);
	}
	mpd_sendCommandListEnd(mi->connection);
	mpd_finishCommand(mi->connection);
	mpd_ob_unlock_conn(mi);
}






int mpd_ob_stats_queue_update(MpdObj *mi)
{

	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_ob_stats_queue_update: Where not connected\n");
		return TRUE;
	}                                       	
	if(mi->stats != NULL)
	{                                  	
		mpd_freeStats(mi->stats);
		mi->stats = NULL;
	}
	return FALSE;
}

int mpd_ob_stats_update(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_INFO,"mpd_ob_stats_update: Where not connected\n");
		return TRUE;
	}
	if(mpd_ob_lock_conn(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_stats_set_volume: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}

	if(mi->stats != NULL)
	{
		mpd_freeStats(mi->stats);
	}
	mpd_sendStatsCommand(mi->connection);
	mi->stats = mpd_getStats(mi->connection);
	if(mi->stats == NULL)
	{
		debug_printf(DEBUG_ERROR,"mpd_ob_stats_update: Failed to grab stats from mpd\n");
	}
	else if(mi->stats->dbUpdateTime != mi->dbUpdateTime)
	{
		debug_printf(DEBUG_INFO, "mpd_ob_stats_update: database updated\n");
		if(mi->database_changed != NULL)
		{                                                                      		
			mi->database_changed(mi, mi->database_changed_signal_pointer);
		}                                                                                           		
		mi->dbUpdateTime = mi->stats->dbUpdateTime;
	}



	if(mpd_ob_unlock_conn(mi))
	{
		return TRUE;
	}
	return FALSE;
}


int mpd_ob_stats_check(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("not connected\n");
		return FALSE;
	}
	if(mi->stats == NULL)
	{
		/* try to update */
		if(mpd_ob_stats_update(mi))
		{
			printf("failed to update status\n");
			return FALSE;
		}
	}
	return TRUE;
}


long unsigned mpd_ob_server_get_database_update_time(MpdObj *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_server_get_database_update: not connected\n");
		return MPD_O_NOT_CONNECTED;
	}
	if(!mpd_ob_stats_check(mi))
	{
		debug_printf(DEBUG_WARNING,"mpd_ob_server_get_database_update: Failed grabbing status\n");
		return MPD_O_FAILED_STATS;
	}
	return mi->stats->dbUpdateTime;
}























void mpd_ob_playlist_queue_delete_id(MpdObj *mi,int id)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_add: not connected\n");
		return;
	}

	if(mi->queue == NULL)
	{
		mi->queue = mpd_ob_new_queue_struct();
		mi->queue->first = mi->queue;
		mi->queue->next = NULL;
		mi->queue->prev = NULL;
	}	
	else
	{
		mi->queue->next = mpd_ob_new_queue_struct();
		mi->queue->next->first = mi->queue->first;
		mi->queue->next->prev = mi->queue;
		mi->queue = mi->queue->next;
		mi->queue->next = NULL;
	}
	mi->queue->type = MPD_QUEUE_DELETE_ID;
	mi->queue->id = id;
	mi->queue->path = NULL;
}


MpdData * mpd_ob_server_get_output_devices(MpdObj *mi)
{
	mpd_OutputEntity *output = NULL;
	MpdData *data = NULL;
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_playlist_get_artists: not connected\n");
		return NULL;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_playlist_get_artists: lock failed\n");
		return NULL;
	}

	mpd_sendOutputsCommand(mi->connection);
	while (( output = mpd_getNextOutput(mi->connection)) != NULL)
	{	
		if(data == NULL)
		{
			data = mpd_ob_new_data_struct();
			data->first = data;
			data->next = NULL;
			data->prev = NULL;

		}	
		else
		{
			data->next = mpd_ob_new_data_struct();
			data->next->first = data->first;
			data->next->prev = data;
			data = data->next;
			data->next = NULL;
		}
		data->type = MPD_DATA_TYPE_OUTPUT_DEV; 
		data->value.output_dev = output;
	}
	mpd_finishCommand(mi->connection);

	/* unlock */
	mpd_ob_unlock_conn(mi);
	if(data == NULL) 
	{
		return NULL;
	}
	return data->first;
}

int mpd_ob_server_set_output_device(MpdObj *mi,int device_id,int state)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_server_set_output_device: not connected\n");	
		return MPD_O_NOT_CONNECTED;
	}
	if(mpd_ob_lock_conn(mi))
	{
		printf("mpd_ob_server_set_output_device: lock failed\n");
		return MPD_O_LOCK_FAILED;
	}
	if(state)
	{
		mpd_sendEnableOutputCommand(mi->connection, device_id);
	}
	else
	{
		mpd_sendDisableOutputCommand(mi->connection, device_id);
	}	
	mpd_finishCommand(mi->connection);

	mpd_ob_unlock_conn(mi);
	mpd_ob_status_queue_update(mi);
	return FALSE;
}

int mpd_ob_server_check_version(MpdObj *mi, int major, int minor, int micro)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("mpd_ob_server_check_version: not connected\n");	
		return FALSE;                                     	
	}
	if(major > mi->connection->version[0]) return FALSE;
	if(mi->connection->version[0] > major) return TRUE;
	if(minor > mi->connection->version[1]) return FALSE;
	if(mi->connection->version[1] > minor) return TRUE;
	if(micro > mi->connection->version[2]) return FALSE;
	if(mi->connection->version[2] > micro) return TRUE; 	
	return TRUE;
}	
