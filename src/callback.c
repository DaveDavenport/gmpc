/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"

/* returns TRUE when an error */
/* NEEDS A REWRITE */
int check_for_errors()
{
	if(info.connection == NULL) return TRUE;
	/* check for an error */
	if(info.connection->error)
	{
		if(debug)g_print("**DEBUG**  error: %s\n", info.connection->errorStr);
		/* check for connection errors */
		if(	info.connection->error == MPD_ERROR_TIMEOUT 		||
				info.connection->error == MPD_ERROR_CONNCLOSED 	||
				info.connection->error == MPD_ERROR_UNKHOST 		||
				info.connection->error == MPD_ERROR_CONNPORT 		||
				info.connection->error == MPD_ERROR_NOTMPD		||
				info.connection->error == MPD_ERROR_NORESPONSE	||
				info.connection->error == MPD_ERROR_SENDING)
		{
			msg_set_base(_("GMPC - Connection Error. Please check youre settings in the preferences menu"));
			mpd_closeConnection(info.connection);
			info.conlock = TRUE;
			info.connection = NULL;
			/*Set some stuff right.  */
			scroll.exposed = 1;
			info.song = -1;
			info.state = -1;

			gtk_timeout_remove(update_timeout);
			update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
		}
		else info.conlock = FALSE;
		/* clear all error's so it doesnt annoy me later */
		if(info.connection) mpd_clearError(info.connection);
		return TRUE;
	}
	else return FALSE;
}
