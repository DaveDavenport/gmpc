#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug_printf.h"
#include "libmpd.h"


/*************************************************************************************/
MpdInt * mpd_ob_create()
{
	MpdInt * mi = malloc(sizeof(MpdInt));
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
	mi->connection = NULL;
	mi->status = NULL;
	mi->stats = NULL;
	mi->error = NULL;
	/* connection is locked because where not connected */
	mi->connection_lock = TRUE;
	
	return mi;
}

void mpd_ob_free(MpdInt *mi)
{
	debug_printf(DEBUG_INFO, "mpd_ob_free: destroying MpdInt object\n");
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
	if(mi->error)
	{
		free(mi->error);
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
	free(mi);
}

int mpd_ob_lock_conn(MpdInt *mi)
{
	debug_printf(DEBUG_INFO, "mpd_ob_lock_conn: Locking connection\n");
	if(mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_lock_conn: Failed to lock connection, allready locked\n");
		return TRUE;
	}
	mi->connection_lock = TRUE;
	return FALSE;
}

int mpd_ob_unlock_conn(MpdInt *mi)
{
	debug_printf(DEBUG_INFO, "mpd_ob_unlock_conn: unlocking connection\n");
	if(!mi->connection_lock)
	{
		debug_printf(DEBUG_WARNING, "mpd_ob_unlock_conn: Failed to unlock connection, allready unlocked\n");
		return FALSE;
	}
	
	mi->connection_lock = FALSE;

	return FALSE;
}

MpdInt * mpd_ob_new_default()
{
	debug_printf(DEBUG_INFO, "mpd_ob_new_default: creating a new mpdInt object\n");
	return mpd_ob_create();
}

MpdInt *mpd_ob_new(char *hostname,  int port, char *password)
{
	MpdInt *mi = mpd_ob_create();
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


void mpd_ob_set_hostname(MpdInt *mi, char *hostname)
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

void mpd_ob_set_password(MpdInt *mi, char *password)
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

void mpd_ob_set_port(MpdInt *mi, int port)
{
	if(mi == NULL)
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_set_port: mi == NULL\n");
		return;
	}
	mi->port = port;
}
int mpd_ob_disconnect(MpdInt *mi)
{
	/* set disconnect flag */
	mi->connected = 0;
	/* lock */
	mpd_ob_lock_conn(mi);

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
}

int mpd_ob_connect(MpdInt *mi)
{
	if(mi == NULL)
	{
		/* should return some spiffy error here */
		return -1;
	}

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
	mi->connection = mpd_newConnection(mi->hostname,mi->port,1);
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

	mpd_ob_unlock_conn(mi);
	return 0;
}

int mpd_ob_check_connected(MpdInt *mi)
{
	if(mi == NULL)
	{
		return FALSE;
	}
	return mi->connected;
}
int mpd_ob_status_queue_update(MpdInt *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("Where not connected\n");
		return TRUE;
	}                                       	
	if(mi->status != NULL)
	{                                  	
		mpd_freeStatus(mi->status);
		mi->status = NULL;
	}
}

int mpd_ob_status_update(MpdInt *mi)
{
	if(!mpd_ob_check_connected(mi))
	{
		printf("Where not connected\n");
		return TRUE;
	}
	if(mi->status != NULL)
	{
		mpd_freeStatus(mi->status);
	}
	mpd_sendStatusCommand(mi->connection);
	mi->status = mpd_getStatus(mi->connection);
	if(mi->status == NULL)
	{
		printf("mpd_ob_status_update: Failed to grab status from mpd\n");
		return TRUE;
	}

	return FALSE;
}

/* returns TRUE when status is availible, when not availible and connected it tries to grab it */
int mpd_ob_status_check(MpdInt *mi)
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


int mpd_ob_status_get_volume(MpdInt *mi)
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

int mpd_ob_status_set_volume(MpdInt *mi,int volume)
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
	mpd_ob_status_update(mi);
	/* return current volume */
	return mpd_ob_status_get_volume(mi);
}

/*******************************************************************************************
 * PLAYER
 */

int mpd_ob_player_get_state(MpdInt *mi)
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

int mpd_ob_player_get_current_song_id(MpdInt *mi)
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

float mpd_ob_status_set_volume_as_float(MpdInt *mi, float fvol)
{
	int volume = mpd_ob_status_set_volume(mi, (int)(fvol*100.0));
	if(volume > -1)
	{
		return (float)volume/100.0;
	}
	return (float)volume;
}


int mpd_ob_player_play(MpdInt *mi)
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

	mpd_sendPlayCommand(mi->connection,-1);
	mpd_finishCommand(mi->connection);


	mpd_ob_unlock_conn(mi);
	if(mpd_ob_status_update(mi))
	{
		return MPD_O_FAILED_STATUS;
	}
	return FALSE;
}

int mpd_ob_player_stop(MpdInt *mi)
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

int mpd_ob_player_next(MpdInt *mi)
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

int mpd_ob_player_prev(MpdInt *mi)
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


int mpd_ob_player_pause(MpdInt *mi)
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


/*******************************************************************************
 * PLAYLIST 
 */

mpd_Song * mpd_ob_playlist_get_song(MpdInt *mi, int songid)
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
	mpd_sendPlaylistInfoCommand(mi->connection, songid);
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
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_song: Failed to grab song from mpd\n");
		return NULL;
	}
	song = mpd_songDup(ent->info.song);
	mpd_freeInfoEntity(ent);

	return song;
}

mpd_Song * mpd_ob_playlist_get_current_song(MpdInt *mi)
{
	int song_id = -1;
	if(!mpd_ob_check_connected(mi))
	{
		debug_printf(DEBUG_ERROR, "mpd_ob_playlist_get_current_song: Not Connected\n");
		return NULL;
	}

	song_id = mpd_ob_player_get_current_song_id(mi);
	if(song_id < 0)
	{
		return NULL;
	}

	return mpd_ob_playlist_get_song(mi, song_id);
}

