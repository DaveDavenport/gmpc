/* 
 *  Copyright (C) 1999-2006 Qball Cow
 *
 *  Code copied from gkrellm by <Bill Wilson    billw@gkrellm.net>
 *  Modified by Qball Cow <qball@qballcow.nl>
 *
 *  This program is free software which I release under the GNU General Public
 *  License. You may redistribute and/or modify this program under the terms
 *  of that license as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.  Version 2 is in the
 *  COPYRIGHT file in the top level directory of this distribution.
 * 
 *  To get a copy of the GNU General Puplic License, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static gchar 	*client_id;
static gchar	*session_id = NULL;


static void cb_smc_save_yourself(SmcConn smc_conn, SmPointer client_data, gint save_type,
		gboolean shutdown, gint interact_style, gboolean fast)
{
	debug_printf(DEBUG_INFO,"Save myself\n");
	SmcSaveYourselfDone(smc_conn, True);
}

	static void
cb_smc_die(SmcConn smc_conn, SmPointer client_data)
{
	debug_printf(DEBUG_INFO,"gmpc die\n");
	SmcCloseConnection(smc_conn, 0, NULL);
	main_quit();
}

	static void
cb_smc_save_complete(SmcConn smc_conn, SmPointer client_data)
{
	debug_printf(DEBUG_INFO,"gmpc save complete\n");
}

	static void
cb_smc_shutdown_cancelled(SmcConn smc_conn, SmPointer client_data)
{
	debug_printf(DEBUG_INFO,"shutdown cancelled\n");
}

	static void
cb_ice_connection_messages(IceConn ice_connection, gint source,
		GdkInputCondition condition)
{
	IceProcessMessages(ice_connection, NULL, NULL);
}

void smc_connect(gint argc, gchar **argv)
{
	SmProp			userid, program, restart, restart_style, clone, pid,
					*props[6];
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
	g_free(callbacks);
	if (!smc_connection)
		return;

	gdk_set_sm_client_id(client_id);

	/* Session manager properties - 4 are required.
	*/
	userid.name = SmUserID;
	userid.type = SmARRAY8;
	userid.num_vals = 1;
	userid.vals = &userid_val;
	uid = getuid();
	if ((pwd = getpwuid(uid)) != NULL)
		snprintf(userid_string, sizeof(userid_string), "%s", pwd->pw_name);
	else
		snprintf(userid_string, sizeof(userid_string), "%d", uid);
	userid_val.value = userid_string;
	userid_val.length = strlen(userid_string);

	pid.name = SmProcessID;
	pid.type = SmARRAY8;
	pid.num_vals = 1;
	pid.vals = &pid_val;
	debug_printf(DEBUG_INFO,pid_str, "%i", getpid());
	pid_val.value = (SmPointer) pid_str;
	pid_val.length = strlen(pid_str);

	restart.name = SmRestartCommand;
	restart.type = SmLISTofARRAY8;
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
	restart_style.name = SmRestartStyleHint;
	restart_style.type = SmCARD8;
	restart_style.num_vals = 1;
	restart_style.vals = &restart_style_val;
	restart_style_val.value = (SmPointer) &restartstyle;
	restart_style_val.length = 1;

	clone.name = SmCloneCommand;
	clone.type = SmLISTofARRAY8;
	clone.vals = restart.vals;
	clone.num_vals = restart.num_vals - 2;

	program.name = SmProgram;
	program.type = SmARRAY8;
	program.vals = restart.vals;
	program.num_vals = 1;

	props[0] = &program;
	props[1] = &userid;
	props[2] = &restart;
	props[3] = &clone;
	props[4] = &pid;
	SmcSetProperties(smc_connection, 5, props);

	g_free(restart.vals);

	ice_connection = SmcGetIceConnection(smc_connection);
	gdk_input_add(IceConnectionNumber(ice_connection), GDK_INPUT_READ,
			(GdkInputFunction) cb_ice_connection_messages, ice_connection);
}

#endif
