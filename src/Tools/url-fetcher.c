/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/
 
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libmpd/libmpd.h>
#include <glib/gstdio.h>
#include "main.h"
#include "playlist3.h"
#include "gmpc_easy_download.h"
#include "gmpc-extras.h"

#define LOG_DOMAIN "UrlFetcher"

#ifdef XSPF
#include <xspf_c.h>
#endif

#ifdef SPIFF
#include <spiff/spiff_c.h>
#endif

#define MAX_PLAYLIST_SIZE 12*1024




typedef struct _UrlParseData
{
	GList *result;
	void (*error_callback)(const gchar *error_msg, gpointer user_data);
	void (*result_callback)(GList *result,gpointer user_data);
	void (*progress_callback)(gdouble progress, gpointer user_data);
	gpointer user_data;

} UrlParseData;


/**
 * Default callback adds to play_queue
 */
 static void url_parse_default_error_callback(const gchar *message, gpointer user_data)
 {
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "UrlFetcher Default error callback: %s", message);
 }
static void url_parse_default_callback(GList *result, gpointer user_data)
{
	GList *iter;
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "UrlFetcher Default callback called: %i items", g_list_length(result));
	for(iter = g_list_first(result); iter != NULL; iter = g_list_next(iter))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "MPD Playlist add: %s", (const char *)iter->data);
		mpd_playlist_add(connection, (const char *)iter->data);
	}
}

static UrlParseData *url_parse_data_new(void)
{
	UrlParseData *d = g_slice_new0(UrlParseData);
	/* Set default callback */
	d->result_callback = url_parse_default_callback;
	d->error_callback = url_parse_default_error_callback;
	return d;
}
static void url_parse_data_free(UrlParseData *data)
{
	g_list_foreach(data->result, (GFunc)g_free, NULL);
	g_list_free(data->result);
	g_slice_free(UrlParseData, data);
}

/***
 * Parse PLS files:
 */
static GList * url_parse_pls_file(const char *data, int size)
{
	int i = 0;
	gchar **tokens = g_regex_split_simple("\n", data, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY);	// g_strsplit(data, "\n", -1);
	GList *retv = NULL;
	if (tokens)
	{
		for (i = 0; tokens[i]; i++)
		{
			/* Check for File */
			if (!strncmp(tokens[i], "File", 4))
			{
				int del = 0;
				/* split the string, look for delimiter = */
				for (del = 3; tokens[i][del] != '\0' && tokens[i][del] != '='; del++) ;
				/** if delimiter is found, and the url behind it starts with http:// add it*/
				if (tokens[i][del] == '=' && strncmp(&tokens[i][del + 1], "http://", 7) == 0)
				{
					retv = g_list_prepend(retv, g_strdup(&tokens[i][del+1]));
				}
			}
		}
		g_strfreev(tokens);
	}
	return g_list_reverse(retv);
}

/***
 * Parse EXTM3U Files:
 */
static GList * url_parse_extm3u_file(const char *data, int size)
{
	int i = 0;
	GList *retv = NULL;
	gchar **tokens = g_regex_split_simple("(\r\n|\n|\r)", data, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY);	// g_strsplit(data, "\n", -1);
	if (tokens)
	{
		for (i = 0; tokens[i]; i++)
		{
			/* Check for File */
			if (!strncmp(tokens[i], "http://", 7))
			{
				retv = g_list_prepend(retv, g_strdup(tokens[i]));
			}
		}
		g_strfreev(tokens);
	}
	return g_list_reverse(retv);
}

#ifdef XSPF
/***
  * parse xspf file 
  */
static GList *url_parse_xspf_file(const char *data, int size, const char *uri)
{
	GList *retv = NULL;
	int has_http = FALSE, has_file = FALSE;
	struct xspf_track *strack;
	struct xspf_mvalue *sloc;
	struct xspf_list *slist;
	char **handlers = mpd_server_get_url_handlers(connection);
	int i = 0;
	for (i = 0; handlers && handlers[i]; i++)
	{
		if (strcmp(handlers[i], "http://") == 0)
		{
			has_http = TRUE;
		} else if (strcmp(handlers[i], "file://") == 0)
		{
			has_file = TRUE;
		}
	}
	if (handlers)
		g_strfreev(handlers);

	slist = xspf_parse_memory(data, (int)size, uri);
	if (slist != NULL)
	{
		XSPF_LIST_FOREACH_TRACK(slist, strack)
		{
			XSPF_TRACK_FOREACH_LOCATION(strack, sloc)
			{
				char *scheme = g_uri_parse_scheme(sloc->value);
				if (scheme)
				{
					g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Trying to add url: %s", sloc->value);
					if (strcmp(scheme, "http") == 0 && has_http)
					{
						retv = g_list_prepend(retv,  g_strdup(sloc->value));
					} else if (strcmp(scheme, "file") == 0 && has_file)
					{
						retv = g_list_prepend(retv,  g_strdup(sloc->value));
					}
					g_free(scheme);
				} else
				{
					g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Failed to parse scheme: %s", sloc->value);
				}
			}
		}
		xspf_free(slist);
	}
	return g_list_reverse(retv);
}
#else
#ifdef SPIFF
/***
 * parse spiff file 
 */
static GList *url_parse_spiff_file(const char *data, int size, const gchar * uri)
{
	GList *retv = NULL;
	const gchar *tempdir = g_get_tmp_dir();
	gchar *filename = g_build_filename(tempdir, "gmpc-temp-spiff-file", NULL);
	if (filename)
	{
		GError *error = NULL;
		int has_http = FALSE, has_file = FALSE;
		char **handlers = mpd_server_get_url_handlers(connection);
		int i = 0;
		for (i = 0; handlers && handlers[i]; i++)
		{
			if (strcmp(handlers[i], "http://") == 0)
			{
				has_http = TRUE;
			} else if (strcmp(handlers[i], "file://") == 0)
			{
				has_file = TRUE;
			}
		}
		if (handlers)
			g_strfreev(handlers);

		g_file_set_contents(filename, data, (gssize) size, &error);
		if (!error)
		{
			struct spiff_track *strack;
			struct spiff_mvalue *sloc;
			struct spiff_list *slist = spiff_parse(filename);
			if (slist != NULL)
			{
				SPIFF_LIST_FOREACH_TRACK(slist, strack)
				{
					SPIFF_TRACK_FOREACH_LOCATION(strack, sloc)
					{
						char *scheme = g_uri_parse_scheme(sloc->value);
						if (scheme)
						{
							g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Trying to add url: %s", sloc->value);
							if (strcmp(scheme, "http") == 0 && has_http)
							{
								retv = g_list_prepend(retv,  g_strdup(sloc->value));
							} else if (strcmp(scheme, "file") == 0 && has_file)
							{
								retv = g_list_prepend(retv,  g_strdup(sloc->value));
							}
							g_free(scheme);
						} else
						{
							g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Failed to parse scheme: %s", sloc->value);
						}
					}
				}
				spiff_free(slist);
			}
			g_unlink(filename);
		} else
		{
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Error message: %s", error->message);
			g_error_free(error);
		}

		g_free(filename);
	}
	return retv;
}
#endif
#endif
/**
 * Check url for correctness
 */
static gboolean url_validate_url(const gchar * text)
{
	int i;
	gchar *scheme;
	gchar **handlers = NULL;
	/** test if text has a length */
	if (!text || text[0] == '\0')
		return FALSE;
	/* Get the scheme of the url */
	scheme = g_uri_parse_scheme(text);
	/* If no scheme, then it is not valid */
	if (scheme == NULL)
	{
		return FALSE;
	}
	handlers = mpd_server_get_url_handlers(connection);
	/* iterate all entries and find matching handler */
	for (i = 0; handlers && handlers[i]; i++)
	{
		if (strncasecmp(handlers[i], scheme, strlen(handlers[i] - 3)) == 0)
		{
			/* If we found a match, the url is valid */
			g_free(scheme);
			if (handlers)
				g_strfreev(handlers);
			return TRUE;
		}
	}
	g_free(scheme);
	if (handlers)
		g_strfreev(handlers);
	return FALSE;
}

/**
 * Handle user input
 */
static int url_check_binary(const char *data, const int size)
{
	int binary = FALSE;
	binary = !g_utf8_validate(data, size, NULL);
	if (binary)
		printf("Binary data found\n");
	return binary;
}

static GList *parse_data(const char *data, guint size, const char *text)
{
	GList *urls = NULL;
	if (url_check_binary(data, size))
	{
		urls = g_list_append(urls, g_strdup(text));
	} else if (!strncasecmp(data, "<?xml", 5))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Detected a xml file, might be xspf");
		/* This might just be a xspf file */
#ifdef XSPF
		urls = url_parse_xspf_file(data, size, text);
#else
#ifdef SPIFF
		urls = url_parse_spiff_file(data, size, text);
#endif
#endif
	}
	/** pls file: */
	else if (!strncasecmp(data, "[playlist]", 10))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Detected a PLS\n");
		urls = url_parse_pls_file(data, size);
	}
	/** Extended M3U file */
	else if (!strncasecmp(data, "#EXTM3U", 7))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Detected a Extended M3U\n");
		urls = url_parse_extm3u_file(data, size);
	}
	/** Hack to detect most non-extended m3u files */
	else if (!strncasecmp(data, "http://", 7))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Might be a M3U, or generic list\n");
		urls = url_parse_extm3u_file(data, size);
	}
	/** Assume Binary file */
	else
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Adding url: %s\n", text);
		urls = g_list_append(urls, g_strdup(text));
	}

	return urls;
}

static void url_fetcher_download_callback(const GEADAsyncHandler * handle, const GEADStatus status, gpointer data)
{
	UrlParseData *upd = (UrlParseData*)data;
	const gchar *uri = gmpc_easy_handler_get_uri(handle);
	if (status == GEAD_DONE)
	{
		goffset length;
		const char *ddata = gmpc_easy_handler_get_data(handle, &length);
		upd->result = parse_data(ddata, (guint) length, uri);
		upd->result_callback(upd->result, upd->user_data);
		url_parse_data_free(upd);
		upd = NULL;
	} else if (status == GEAD_CANCELLED)
	{
		upd->result_callback(upd->result, upd->user_data);
		url_parse_data_free(upd);
		upd = NULL;
	} else if (status == GEAD_PROGRESS)
	{
		goffset length;
		goffset total = gmpc_easy_handler_get_content_size(handle);
		const char *ddata = gmpc_easy_handler_get_data(handle, &length);
		if (data)
		{
			if (total > 0)
			{
				gdouble prog = (length / (double)total);
				if(upd->progress_callback != NULL) 
					upd->progress_callback(prog, upd->user_data);
			} else
			{
				if(upd->progress_callback != NULL) 
					upd->progress_callback(-1, upd->user_data);
			}
		}
		if (length > MAX_PLAYLIST_SIZE)
		{
			upd->result = parse_data(ddata, (guint) length, uri);
			gmpc_easy_async_cancel(handle);
		}
	} else
	{
		upd->result_callback(upd->result, upd->user_data);
		url_parse_data_free(upd);
		upd = NULL;
	}
}

/****************************************
 * Parsing uri
 */

static void parse_uri(const char *uri, UrlParseData *upd)
{
	gchar *scheme;
	g_assert(upd != NULL);
	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Trying url: %s", uri);
	/* Check NULL */
	if (uri == NULL)
	{
		upd->result_callback(upd->result, upd->user_data);
		url_parse_data_free(upd);
		return;
	}
	/* Check local path */
	scheme = g_uri_parse_scheme(uri);

	if (scheme == NULL)
	{
		/* local uri */
		if (g_file_test(uri, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			FILE *fp = g_fopen(uri, "r");
			if (fp)
			{
				char buffer[MAX_PLAYLIST_SIZE];
				ssize_t t = fread(buffer, 1, MAX_PLAYLIST_SIZE - 1, fp);
				/* Make sure it is NULL terminated */
				buffer[t] = '\0';
				upd->result = parse_data(buffer, (guint) t, uri);
				upd->result_callback(upd->result, upd->user_data);
				url_parse_data_free(upd);

				fclose(fp);
			}
		} else
		{
			gchar *temp = g_strdup_printf("%s: '%s'", _("Failed to open local file"), uri);
			if(upd->error_callback != NULL)
				upd->error_callback(temp, upd->user_data);
			url_parse_data_free(upd);
			g_free(temp);
		}
	}
	else
	{
		/* remote uri */
		if (url_validate_url(uri))
		{
			if (strcasecmp(scheme, "http") == 0)
			{
				gmpc_easy_async_downloader(uri, url_fetcher_download_callback, upd);
			} else
			{
				upd->result = g_list_append(NULL, g_strdup(uri)); 
				upd->result_callback(upd->result, upd->user_data);
				url_parse_data_free(upd);
			}
		} else
		{
			gchar *temp = g_strdup_printf("%s: '%s'", _("Uri scheme not supported"), scheme);
			if(upd->error_callback != NULL)
				upd->error_callback(temp, upd->user_data);
			url_parse_data_free(upd);
			g_free(temp);
		}
	}
	if (scheme)
		g_free(scheme);
	/* Dialog needs to be kept running */
	return;
}
static void gufg_set_progress(gdouble prog, gpointer a)
{
	gmpc_url_fetching_gui_set_progress(GMPC_URL_FETCHING_GUI(a), prog);	
}
static void gufg_set_error(const gchar *error_msg, gpointer a)
{
	gmpc_url_fetching_gui_set_error(GMPC_URL_FETCHING_GUI(a), error_msg);	
}
static void gufg_set_result(GList *result, gpointer a)
{
	url_parse_default_callback(result, a);
	gmpc_url_fetching_gui_set_completed(GMPC_URL_FETCHING_GUI(a));	
}
static void gufg_parse_callback(GmpcUrlFetchingGui * a, const gchar * url, void *user_data)
{
	if(url != NULL)
	{
		UrlParseData *data = url_parse_data_new();
		gmpc_url_fetching_gui_set_processing(a);
		data->user_data = a;
		data->progress_callback = gufg_set_progress;
		data->error_callback = gufg_set_error;
		data->result_callback = gufg_set_result;

		parse_uri(url, data);
	}
}

static gboolean gufg_validate_callback(GmpcUrlFetchingGui * a, const gchar * url, void *user_data)
{
	return (strlen(url) > 0 && (G_IS_DIR_SEPARATOR(url[0]) || url_validate_url(url)));
}

static gboolean gufg_validate_callback_0160(GmpcUrlFetchingGui * a, const gchar * url, void *user_data)
{
	return TRUE;
}

static void gufg_parse_callback_0160(GmpcUrlFetchingGui * a, const gchar * url, void *user_data)
{
	if(url != NULL)
	{
		if (mpd_playlist_load(connection, url) == MPD_PLAYLIST_LOAD_FAILED)
		{
			gufg_parse_callback(a, url, user_data);
			return;
		}
	}
	gmpc_url_fetching_gui_set_completed(a);
}

void url_start_easy_command(void *data,char *param, void *d )
{
	g_debug("Url easy command received: %s\n", param);
	url_start_real(param);
}
void url_start(void)
{
	if (mpd_server_check_version(connection, 0, 16, 0))
	{
		gmpc_url_fetching_gui_new(gufg_parse_callback_0160, NULL, gufg_validate_callback_0160, NULL, g_object_unref);
	} else
	{
		gmpc_url_fetching_gui_new(gufg_parse_callback, NULL, gufg_validate_callback, NULL, g_object_unref);
	}
}

void url_start_real(const gchar * url)
{
	if (mpd_server_check_version(connection, 0, 16, 0))
	{
		printf("add url2: '%s'\n", url);
		if (mpd_playlist_load(connection, url) == MPD_PLAYLIST_LOAD_FAILED)
		{
			UrlParseData *upd = url_parse_data_new();
			parse_uri(url, upd);
			return;
		}
	} else
	{
		UrlParseData *upd = url_parse_data_new();
		parse_uri(url, upd);
	}
}

void url_start_custom(const gchar *url, 
	void (*error_callback)(const gchar *error_msg, gpointer user_data),
	void (*result_callback)(GList *result,gpointer user_data),
	void (*progress_callback)(gdouble progress, gpointer user_data),
	gpointer user_data)
{
	UrlParseData *data = url_parse_data_new();
	data->user_data = user_data;
	data->progress_callback = progress_callback;
	data->error_callback = error_callback;
	data->result_callback = result_callback;

	parse_uri(url, data);
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
