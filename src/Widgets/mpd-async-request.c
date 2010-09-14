#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <gio/gio.h>
#include "main.h"
#include "mpd-async-request.h"

#define LOG_DOMAIN "MpdAsyncRequest"

typedef struct
{
	MpdObj *mi;
	MpdAsyncFunction *function;
	gpointer function_data;
	MpdAsyncCallback *callback;
	gpointer callback_data;
	MpdData *result;
} MpdAsyncData;

static void __async_handler_function(GSimpleAsyncResult * simple, GObject * object, GCancellable * cancel)
{
	MpdAsyncData *data = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(simple));
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Start async function\n");
	if (mpd_connect(data->mi) == MPD_OK)
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "call function\n");
		data->result = (data->function(data->mi, data->function_data));
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Got result: %p\n", data->result);
	}

	mpd_free(data->mi);
}

static void __mpd_async_simple_callback(GObject * object, GAsyncResult * simple, gpointer user_data)
{
	MpdAsyncData *data = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(simple));
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Start async result function %p\n", data);
	if (data->callback)
		data->callback(data->result, data->callback_data);
}

static void __mpd_async_result_free_data(gpointer udata)
{
	MpdAsyncData *data = (MpdAsyncData *) udata;
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Free data\n");
	g_free(data);
}

void mpd_async_request(MpdAsyncCallback * callback, gpointer callback_data, MpdAsyncFunction * function,
					   gpointer function_data)
{
	gchar *string = NULL;
	GSimpleAsyncResult *res = NULL;
	MpdAsyncData *data = g_new0(MpdAsyncData, 1);
	data->mi = mpd_new_default();
	data->function = function;
	data->function_data = function_data;
	data->callback = callback;
	data->callback_data = callback_data;
	/**
     * Set Hostname
     */
	string = connection_get_hostname();
	mpd_set_hostname(data->mi, string);
	/**
	 * Set port
	 */
	mpd_set_port(data->mi, connection_get_port());
	/**
	 * Timeout
	 */
	mpd_set_connection_timeout(data->mi,
							   cfg_get_single_value_as_float_with_default(config, "connection", "timeout",
																		  DEFAULT_TIMEOUT));

	if (connection_use_auth())
	{
		string = connection_get_password();
		mpd_set_password(data->mi, string);
	} else
	{
		mpd_set_password(data->mi, "");
	}

	res = g_simple_async_result_new(NULL, __mpd_async_simple_callback, NULL, mpd_async_request);
	g_simple_async_result_set_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(res), data, __mpd_async_result_free_data);
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Start async thread %p\n", data);
	g_simple_async_result_run_in_thread(res, __async_handler_function, G_PRIORITY_LOW, NULL);
}
