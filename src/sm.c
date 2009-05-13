/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Modified and borrowed from Bill Wilson <billw@gkrellm.net> (GKrellM)
 * This projects' homepage is: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <config.h>
#ifdef ENABLE_SM

#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xmd.h>
#include <X11/SM/SMlib.h>
#include <libmpd/debug_printf.h>
#include "main.h"
#include "sm.h"

#define LOG_DOMAIN "SessionManagement"
static gchar 	*client_id;
static gchar	*session_id = NULL;


static void cb_smc_save_yourself(SmcConn smc_conn, SmPointer client_data, gint save_type,
		gboolean shutdown, gint interact_style, gboolean fast)
{
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"Save myself\n");
	SmcSaveYourselfDone(smc_conn, True);
}

	static void
cb_smc_die(SmcConn smc_conn, SmPointer client_data)
{
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"gmpc die\n");
	SmcCloseConnection(smc_conn, 0, NULL);
	main_quit();
}

	static void
cb_smc_save_complete(SmcConn smc_conn, SmPointer client_data)
{
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"gmpc save complete\n");
}

	static void
cb_smc_shutdown_cancelled(SmcConn smc_conn, SmPointer client_data)
{
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"shutdown cancelled\n");
}

	static void
cb_ice_connection_messages(IceConn ice_connection, gint source,
		GdkInputCondition condition)
{
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Ice connection");
	IceProcessMessages(ice_connection, NULL, NULL);
}

void smc_connect(gint argc, gchar **argv)
{
	SmProp			userid, program, restart, restart_style, clone, pid,
					*props[5];
	SmPropValue		userid_val, pid_val, restart_style_val;
	CARD8			restartstyle;
	SmcCallbacks	*callbacks;
	SmcConn			smc_connection;
	IceConn			ice_connection;
	struct passwd	*pwd;
	uid_t			uid;
	gchar			error_string[256], pid_str[16], userid_string[256];
	gulong			mask;
	gint			i, j;

	/* Session manager callbacks
	*/
	callbacks = g_new0(SmcCallbacks, 1);
	callbacks->save_yourself.callback = cb_smc_save_yourself;
	callbacks->die.callback = cb_smc_die;
	callbacks->save_complete.callback = cb_smc_save_complete;
	callbacks->shutdown_cancelled.callback = cb_smc_shutdown_cancelled;

	mask = SmcSaveYourselfProcMask | SmcDieProcMask | SmcSaveCompleteProcMask
		| SmcShutdownCancelledProcMask;

	smc_connection = SmcOpenConnection(NULL /* SESSION_MANAGER env variable */,
			NULL /* share ICE connection */,
			SmProtoMajor, SmProtoMinor, mask,
			callbacks,
			session_id, &client_id,
			sizeof(error_string), error_string);
	q_free(callbacks);
	if (!smc_connection)
		return;

	gdk_set_sm_client_id(client_id);

	/* Session manager properties - 4 are required.
	*/
	userid.name = (char *)SmUserID;
	userid.type = (char *)SmARRAY8;
	userid.num_vals = 1;
	userid.vals = &userid_val;
	uid = getuid();
	if ((pwd = getpwuid(uid)) != NULL)
		snprintf(userid_string, sizeof(userid_string), "%s", pwd->pw_name);
	else
		snprintf(userid_string, sizeof(userid_string), "%d", uid);
	userid_val.value = userid_string;
	userid_val.length = strlen(userid_string);

	pid.name = (char  *)SmProcessID;
	pid.type = (char *)SmARRAY8;
	pid.num_vals = 1;
	pid.vals = &pid_val;
	sprintf(pid_str, "%i", getpid());
	pid_val.value = (SmPointer) pid_str;
	pid_val.length = strlen(pid_str);

	restart.name = (char *)SmRestartCommand;
	restart.type = (char *)SmLISTofARRAY8;
	restart.vals = g_new0(SmPropValue, argc + 2);
	j = 0;
	for (i = 0; i < argc; ++i) {
		if ( strcmp(argv[i], "--sm-client-id") ) {
			restart.vals[j].value = (SmPointer) argv[i];
			restart.vals[j++].length = strlen(argv[i]);
		} else
			i++;
	}
	restart.vals[j].value = (SmPointer) "--sm-client-id";
	restart.vals[j++].length = strlen("--sm-client-id");
	restart.vals[j].value = (SmPointer) client_id;
	restart.vals[j++].length = strlen(client_id);
	restart.num_vals = j;

	restartstyle = SmRestartImmediately;
	restart_style.name =(char *) SmRestartStyleHint;
	restart_style.type =(char *) SmCARD8;
	restart_style.num_vals = 1;
	restart_style.vals = &restart_style_val;
	restart_style_val.value = (SmPointer) &restartstyle;
	restart_style_val.length = 1;

	clone.name = (char *)SmCloneCommand;
	clone.type = (char *)SmLISTofARRAY8;
	clone.vals = restart.vals;
	clone.num_vals = restart.num_vals - 2;

	program.name = (char *)SmProgram;
	program.type = (char *)SmARRAY8;
	program.vals = restart.vals;
	program.num_vals = 1;

	props[0] = &program;
	props[1] = &userid;
	props[2] = &restart;
	props[3] = &clone;
	props[4] = &pid;
	SmcSetProperties(smc_connection, 5, props);

	q_free(restart.vals);

	ice_connection = SmcGetIceConnection(smc_connection);
	gdk_input_add(IceConnectionNumber(ice_connection), GDK_INPUT_READ,
			(GdkInputFunction) cb_ice_connection_messages, ice_connection);
}

#endif
