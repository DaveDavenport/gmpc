#ifndef __MPD_INTERACTION__
#define __MPD_INTERACTION__
#include <libmpdclient.h>

typedef struct _MpdInt 
{
	/* defines if we are connected */
	/* This should be made true if and only if the connection is up and running */
	gboolean connected;
	/* information needed to connect to mpd */
	char *hostname;
	int port;
	char *password;

	/* mpd's structures */
	mpd_Connection *connection;
	mpd_Status *status;
	mpd_Stats *stats;

	/* error message */
	char *error;	
	
	/* internal values */
	/* this "locks" the connections. so we can't have to commands competing with eachother */
	gboolean connection_lock;
	
}MpdInt;


MpdInt * 	mpd_ob_new_default();
MpdInt * 	mpd_ob_new(char *hostname, int port, char *password);
void 		mpd_ob_set_hostname(MpdInt *mi, char *hostname);
void 		mpd_ob_set_password(MpdInt *mi, char *hostname);
void 		mpd_ob_set_port(MpdInt *mi, int port);
int 		mpd_ob_connect(MpdInt *mi);



#endif
