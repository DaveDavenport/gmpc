/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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
#include <libmpd/debug_printf.h>
#include <glib/gstdio.h>
#include "main.h"
#include "playlist3.h"
#include "gmpc_easy_download.h"
#include "gmpc-extras.h"

 #ifdef XSPF
#include <xspf_c.h>
#endif

#ifdef SPIFF
#include <spiff/spiff_c.h>
#endif

#define MAX_PLAYLIST_SIZE 12*1024
/***
 * Parse PLS files:
 */
static void url_parse_pls_file(const char *data, int size)
{
	int i = 0;
	int songs = 0;
	gchar **tokens = g_regex_split_simple("\n", data, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY);	// g_strsplit(data, "\n", -1);
	if (tokens) {
		for (i = 0; tokens[i]; i++) {
			/* Check for File */
			if (!strncmp(tokens[i], "File", 4)) {
				int del = 0;
				/* split the string, look for delimiter = */
				for (del = 3; tokens[i][del] != '\0' && tokens[i][del] != '='; del++) ;
				/** if delimiter is found, and the url behind it starts with http:// add it*/
				if (tokens[i][del] == '=' && strncmp(&tokens[i][del + 1], "http://", 7) == 0) {
					mpd_playlist_add(connection, &tokens[i][del + 1]);
					songs++;
				}
			}
		}
		g_strfreev(tokens);
	}
	if (songs) {
		char *string = g_strdup_printf(_("Added %i %s"), songs, ngettext("stream", "streams", songs));
		pl3_push_statusbar_message(string);
		q_free(string);
	}
}

/***
 * Parse EXTM3U Files:
 */
static void url_parse_extm3u_file(const char *data, int size)
{
	int i = 0;
	int songs = 0;
	gchar **tokens = g_regex_split_simple("(\r\n|\n|\r)", data, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY);	// g_strsplit(data, "\n", -1);
	if (tokens) {
		for (i = 0; tokens[i]; i++) {
			/* Check for File */
			if (!strncmp(tokens[i], "http://", 7)) {
				mpd_playlist_add(connection, tokens[i]);
				songs++;
			}
		}
		g_strfreev(tokens);
	}
	if (songs) {
		char *string = g_strdup_printf(_("Added %i %s"), songs, ngettext("stream", "streams", songs));
		pl3_push_statusbar_message(string);
		q_free(string);
	}
}

 #ifdef XSPF
/***
  * parse xspf file 
  */
 static void url_parse_xspf_file(const char *data, int size,const char *uri)
 {
     int songs= 0;
     int has_http = FALSE, has_file = FALSE;
     struct xspf_track *strack;
     struct xspf_mvalue *sloc;
	 struct xspf_list *slist;
	 char **handlers = mpd_server_get_url_handlers(connection);
     int i = 0;
     for (i = 0; handlers && handlers[i]; i++) {
         if (strcmp(handlers[i], "http://") == 0) {
             has_http = TRUE;
         } else if (strcmp(handlers[i], "file://") == 0) {
             has_file = TRUE;
         }
     }
     if (handlers)
         g_strfreev(handlers);

     slist = xspf_parse_memory(data,(int)size,uri);
     if (slist != NULL)
     {
         XSPF_LIST_FOREACH_TRACK(slist, strack) {
             XSPF_TRACK_FOREACH_LOCATION(strack, sloc) {
                 char *scheme = g_uri_parse_scheme(sloc->value);
                 if(scheme)
                 {
                     debug_printf(DEBUG_INFO, "Trying to add url: %s", sloc->value);
                     if(strcmp(scheme, "http") == 0 && has_http) 
                     {
                         mpd_playlist_add(connection, sloc->value);
                         songs++;
                     }
                     else if(strcmp(scheme, "file") == 0 && has_file)
                     {
                         mpd_playlist_add(connection, sloc->value);
                         songs++;
                     }
                     g_free(scheme);
                 }
                 else{
                     debug_printf(DEBUG_ERROR, "Failed to parse scheme: %s",sloc->value);
                 }
             }
         }
         xspf_free(slist);
     }
     if (songs) {
         char *string = g_strdup_printf(_("Added %i %s"), songs, ngettext("stream", "streams", songs));
         pl3_push_statusbar_message(string);
         q_free(string);
     }


}
#else 
#ifdef SPIFF
/***
 * parse spiff file 
 */
static void url_parse_spiff_file(const char *data, int size, const gchar *uri)
{
    int songs= 0;
    const gchar *tempdir = g_get_tmp_dir();
    gchar *filename = g_build_filename(tempdir, "gmpc-temp-spiff-file",NULL);
    if(filename)
    {
        GError *error = NULL;
        int has_http = FALSE, has_file = FALSE;
        char **handlers = mpd_server_get_url_handlers(connection);
        int i = 0;
        for (i = 0; handlers && handlers[i]; i++) {
            if (strcmp(handlers[i], "http://") == 0) {
                has_http = TRUE;
            } else if (strcmp(handlers[i], "file://") == 0) {
                has_file = TRUE;
            }
        }
        if (handlers)
            g_strfreev(handlers);

        g_file_set_contents(filename, data,(gssize)size, &error);
        if(!error)
        {
            struct spiff_track *strack;
            struct spiff_mvalue *sloc;
            struct spiff_list *slist = spiff_parse(filename);
            if (slist != NULL)
            {
                SPIFF_LIST_FOREACH_TRACK(slist, strack) {
                    SPIFF_TRACK_FOREACH_LOCATION(strack, sloc) {
                        char *scheme = g_uri_parse_scheme(sloc->value);
                        if(scheme)
                        {
                            debug_printf(DEBUG_INFO, "Trying to add url: %s", sloc->value);
                            if(strcmp(scheme, "http") == 0 && has_http) 
                            {
                                mpd_playlist_add(connection, sloc->value);
                                songs++;
                            }
                            else if(strcmp(scheme, "file") == 0 && has_file)
                            {
                                mpd_playlist_add(connection, sloc->value);
                                songs++;
                            }
                            g_free(scheme);
                        }
                        else{
                            debug_printf(DEBUG_ERROR, "Failed to parse scheme: %s",sloc->value);
                        }
                    }
                }
                spiff_free(slist);
            }
            g_unlink(filename);
        }
        else 
        {
            debug_printf(DEBUG_ERROR, "Error message: %s", error->message);
            g_error_free(error);
        }

        g_free(filename);
    }
    if (songs) {
        char *string = g_strdup_printf(_("Added %i %s"), songs, ngettext("stream", "streams", songs));
        pl3_push_statusbar_message(string);
        q_free(string);
    }


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
	if(scheme == NULL){
		return FALSE;
	}
	handlers = mpd_server_get_url_handlers(connection);
	/* iterate all entries and find matching handler */
	for(i=0;handlers && handlers[i]; i++)
	{
		if(strncasecmp(handlers[i], scheme, strlen(handlers[i]-3)) == 0)
		{
			/* If we found a match, the url is valid */
			g_free(scheme);
			if(handlers) g_strfreev(handlers);
			return TRUE;
		}
	}
	g_free(scheme);
	if(handlers) g_strfreev(handlers);
	return FALSE;
}

/**
 * Handle user input
 */
static void url_entry_changed(GtkEntry * entry, GtkWidget * add_button)
{
	const gchar *text = gtk_entry_get_text(entry);
	gtk_widget_set_sensitive(add_button, url_validate_url(text));
}

static int url_check_binary(const char *data, const int size)
{
	int binary = FALSE;
	binary = !g_utf8_validate(data, size, NULL);
	if (binary)
		printf("Binary data found\n");
	return binary;
}

static void parse_data(const char *data, guint size, const char *text)
{
	if (url_check_binary(data, size)) {
		debug_printf(DEBUG_INFO, "Adding url\n", text);
		mpd_playlist_add(connection, (char *)text);
		pl3_push_statusbar_message(_("Added 1 stream"));
	}
    else if (!strncasecmp(data, "<?xml", 5)) {
        debug_printf(DEBUG_INFO,  "Detected a xml file, might be xspf");
        /* This might just be a xspf file */
#ifdef XSPF		
		url_parse_xspf_file(data, size, text);
#else
#ifdef SPIFF
        url_parse_spiff_file(data, size, text);
#endif
#endif
    }
	/** pls file: */
	else if (!strncasecmp(data, "[playlist]", 10)) {
		debug_printf(DEBUG_INFO, "Detected a PLS\n");
		url_parse_pls_file(data, size);
	}
	/** Extended M3U file */
	else if (!strncasecmp(data, "#EXTM3U", 7)) {
		debug_printf(DEBUG_INFO, "Detected a Extended M3U\n");
		url_parse_extm3u_file(data, size);
	}
	/** Hack to detect most non-extended m3u files */
	else if (!strncasecmp(data, "http://", 7)) {
		debug_printf(DEBUG_INFO, "Might be a M3U, or generic list\n");
		url_parse_extm3u_file(data, size);
	}
	/** Assume Binary file */
	else {
		debug_printf(DEBUG_INFO, "Adding url: %s\n", text);
		mpd_playlist_add(connection, (char *)text);
		pl3_push_statusbar_message(_("Added 1 stream"));
	}
}

static void url_fetcher_download_callback(const GEADAsyncHandler * handle, const GEADStatus status, gpointer data)
{
	const gchar *uri = gmpc_easy_handler_get_uri(handle);
	if (status == GEAD_DONE) {
		goffset length;
		const char *ddata = gmpc_easy_handler_get_data(handle, &length);
		parse_data(ddata, (guint) length, uri);
		gmpc_url_fetching_gui_set_completed(data);
	} else if (status == GEAD_CANCELLED) {
		gmpc_url_fetching_gui_set_completed(data);
	} else if (status == GEAD_PROGRESS) {
		goffset length;
		goffset total = gmpc_easy_handler_get_content_size(handle);
		const char *ddata = gmpc_easy_handler_get_data(handle, &length);
		if (data) {
			if (total > 0) {
				gdouble prog = (length / (double)total);
				gmpc_url_fetching_gui_set_progress(data, (prog > 1)?1:prog);
			} else {
				gmpc_url_fetching_gui_set_progress(data,-1);
			}
		}
		if (length > MAX_PLAYLIST_SIZE) {
			printf("Cancel to much data. Try to parse\n");
			parse_data(ddata, (guint) length, uri);
			printf("done\n");
			gmpc_easy_async_cancel(handle);
		}
	} else {
		/* add failed urls anyway */
		mpd_playlist_add(connection, uri);
		gmpc_url_fetching_gui_set_completed(data);
	}
}

/****************************************
 * Parsing uri
 */

static int parse_uri(const char *uri, gpointer data)
{
	gchar *scheme;	
	/* Check NULL */
	if(uri == NULL) {
		gmpc_url_fetching_gui_set_completed(data);
		return;
	}
	/* Check local path */
	scheme = g_uri_parse_scheme(uri);

	if(scheme == NULL) {
		/* local uri */
		if(g_file_test(uri, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR))
		{
			FILE *fp = g_fopen(uri, "r");			
			if(fp)
			{
				char buffer[MAX_PLAYLIST_SIZE];
				ssize_t t = fread(buffer,1, MAX_PLAYLIST_SIZE-1,fp);
				/* Make sure it is NULL terminated */
				buffer[t] = '\0';
				parse_data(buffer, (guint)t, uri);

				fclose(fp);
				gmpc_url_fetching_gui_set_completed(data);
			}
		}else{
			gchar *temp = g_strdup_printf("%s: '%s'", _("Failed to open local file"), uri);
			gmpc_url_fetching_gui_set_error(data,temp);
			g_free(temp);
		}
	}else
	{
		/* remote uri */
		if(url_validate_url(uri))
		{
			if(strcasecmp(scheme, "http") == 0){
				gmpc_easy_async_downloader(uri, url_fetcher_download_callback, data);
			}else{
				mpd_playlist_add(connection, (char *)uri);
				/* indicates it should stop */
				gmpc_url_fetching_gui_set_completed(data);
			}
		}else{
			gchar *temp = g_strdup_printf("%s: '%s'", _("Uri scheme not supported"), scheme);
			gmpc_url_fetching_gui_set_error(data,temp);
			g_free(temp);
		}
	}
	if(scheme) 
		g_free(scheme);
	/* Dialog needs to be kept running */
	return FALSE;
}

static void gufg_parse_callback(GmpcUrlFetchingGui *a, const gchar *url, void *user_data, GError **error)
{
	gmpc_url_fetching_gui_set_processing(a);
	parse_uri(url, a);
}

static gboolean gufg_validate_callback(GmpcUrlFetchingGui *a,const gchar *url)
{
	return (strlen(url) > 0 && (G_IS_DIR_SEPARATOR(url[0]) || url_validate_url(url)));
}

void url_start(void)
{
	GtkWidget *a = gmpc_url_fetching_gui_new(gufg_parse_callback,NULL, gufg_validate_callback,NULL, g_object_unref);
}

void url_start_real(const gchar * url)
{
	parse_uri(url, NULL);
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
