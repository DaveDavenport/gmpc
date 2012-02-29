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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <zlib.h>
#include "gmpc_easy_download.h"
#include "main.h"

#define LOG_DOMAIN "EasyDownload"
/****
 * ZIP MISC
 * **********/
#define HEAD_CRC     0x02	/* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04	/* bit 2 set: extra field present */
#define ORIG_NAME    0x08	/* bit 3 set: original file name present */
#define COMMENT      0x10	/* bit 4 set: file comment present */
static char gz_magic[2] = { 0x1f, 0x8b };

static int skip_gzip_header(const char *src, gsize size)
{
	int idx;
	if (size < 10 || memcmp(src, gz_magic, 2))
		return -1;
	if (src[2] != Z_DEFLATED)
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, 
					"unsupported compression method (%d).\n", (int)src[3]);
		return -1;
	}
	idx = 10;
	/* skip past header, mtime and xos */

	if (src[3] & EXTRA_FIELD)
		idx += src[idx] + (src[idx + 1] << 8) + 2;
	if (src[3] & ORIG_NAME)
	{
		while (src[idx])
			idx++;
		idx++;
	}
	if (src[3] & COMMENT)
	{
		while (src[idx])
			idx++;
		idx++;
	}
	if (src[3] & HEAD_CRC)
		idx += 2;
	return idx;
}

static int read_cb(void *z, char *buffer, int size)
{
	int r = 0;
	z_stream *zs = z;
	if (zs)
	{
		zs->next_out = (void *)buffer;
		zs->avail_out = size;
		r = inflate(zs, Z_SYNC_FLUSH);
		if (r == Z_OK || r == Z_STREAM_END || r == Z_NEED_DICT || r == Z_BUF_ERROR)
		{
			return size - zs->avail_out;
		}
	}
	g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "failed unzipping stream: %i\n", r);
	return -1;
}

static int close_cb(void *z)
{
	z_stream *zs = z;
	inflateEnd(zs);
	g_free(zs);
	return 0;
}

static SoupSession *soup_session = NULL;

static void gmpc_easy_download_set_proxy(SoupSession * session)
{
	if (session == NULL)
		return;

	if (cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use Proxy", FALSE))
	{
		char *value = cfg_get_single_value_as_string(config, "Network Settings", "Proxy Address");
		char *username = NULL;
		char *password = NULL;
		gint port = cfg_get_single_value_as_int_with_default(config, "Network Settings", "Proxy Port", 8080);
		if (cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use authentication", FALSE))
		{
			password = cfg_get_single_value_as_string(config, "Network Settings", "Proxy authentication password");
			username = cfg_get_single_value_as_string(config, "Network Settings", "Proxy authentication username");
		}
		if (value)
		{
			SoupURI *uri = NULL;
			gchar *ppath = NULL;
			if (username && username[0] != '\0' && password && password[0] != '\0')
			{
				gchar *usere = gmpc_easy_download_uri_escape(username);
				gchar *passe = gmpc_easy_download_uri_escape(password);
				ppath = g_strdup_printf("http://%s:%s@%s:%i", usere, passe, value, port);
				g_free(usere);
				g_free(passe);
			} else if (username && username[0] != '\0')
			{
				gchar *usere = gmpc_easy_download_uri_escape(username);
				ppath = g_strdup_printf("http://%s@%s:%i", usere, value, port);
				g_free(usere);
			} else
			{
				ppath = g_strdup_printf("http://%s:%i", value, port);
			}
			uri = soup_uri_new(ppath);
			g_object_set(G_OBJECT(session), SOUP_SESSION_PROXY_URI, uri, NULL);
			soup_uri_free(uri);
			g_free(ppath);
		}
		g_free(username);
		g_free(password);
		g_free(value);
	} else
	{
		g_object_set(G_OBJECT(session), SOUP_SESSION_PROXY_URI, NULL, NULL);
	}
}

/***
 * preferences window
 */
/* for gtkbuilder */
void proxy_pref_use_proxy_toggled(GtkWidget * toggle_button);
void proxy_pref_http_address_changed(GtkWidget * entry);
void proxy_pref_http_port_changed(GtkWidget * entry);
void proxy_pref_use_auth_toggled(GtkWidget * toggle_button);
void proxy_pref_auth_username_changed(GtkWidget * entry);
void proxy_pref_auth_password_changed(GtkWidget * entry);

static GtkBuilder *proxy_pref_xml = NULL;
static void proxy_pref_destroy(GtkWidget * container)
{
	GObject *temp = gtk_builder_get_object(proxy_pref_xml, "frame_proxy_settings");
	gtk_container_remove(GTK_CONTAINER(container), GTK_WIDGET(temp));
	g_object_unref(proxy_pref_xml);
	proxy_pref_xml = NULL;
}

void proxy_pref_use_proxy_toggled(GtkWidget * toggle_button)
{
	cfg_set_single_value_as_int(config, "Network Settings", "Use Proxy",
								gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
	gmpc_easy_download_set_proxy(soup_session);
}

void proxy_pref_http_address_changed(GtkWidget * entry)
{
	cfg_set_single_value_as_string(config, "Network Settings", "Proxy Address",
								   (char *)gtk_entry_get_text(GTK_ENTRY(entry)));
	gmpc_easy_download_set_proxy(soup_session);
}

void proxy_pref_http_port_changed(GtkWidget * entry)
{
	cfg_set_single_value_as_int(config, "Network Settings", "Proxy Port",
								gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)));
	gmpc_easy_download_set_proxy(soup_session);
}

void proxy_pref_use_auth_toggled(GtkWidget * toggle_button)
{
	cfg_set_single_value_as_int(config, "Network Settings", "Use authentication",
								gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
	gmpc_easy_download_set_proxy(soup_session);
}

void proxy_pref_auth_username_changed(GtkWidget * entry)
{
	cfg_set_single_value_as_string(config, "Network Settings", "Proxy authentication username",
								   (char *)gtk_entry_get_text(GTK_ENTRY(entry)));
	gmpc_easy_download_set_proxy(soup_session);
}

void proxy_pref_auth_password_changed(GtkWidget * entry)
{
	cfg_set_single_value_as_string(config, "Network Settings", "Proxy authentication password",
								   gtk_entry_get_text(GTK_ENTRY(entry)));
	gmpc_easy_download_set_proxy(soup_session);
}

static void proxy_pref_construct(GtkWidget * container)
{
	GObject *temp = NULL;
	gchar *string;
	GError *error = NULL;
	gchar *path = gmpc_get_full_glade_path("preferences-proxy.ui");
	proxy_pref_xml = gtk_builder_new();
	gtk_builder_add_from_file(proxy_pref_xml, path, &error);

	if (error)
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failed to load %s: '%s'", path, error->message);
		g_error_free(error);
		g_free(path);

		return;
	}

	q_free(path);
	/* use proxy */
	temp = gtk_builder_get_object(proxy_pref_xml, "checkbutton_use_proxy");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp),
								 cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use Proxy",
																		  FALSE));

	/* hostname */
	temp = gtk_builder_get_object(proxy_pref_xml, "entry_http_hostname");
	string = cfg_get_single_value_as_string(config, "Network Settings", "Proxy Address");
	if (string)
	{
		gtk_entry_set_text(GTK_ENTRY(temp), string);
		g_free(string);
	}

	/* port */
	temp = gtk_builder_get_object(proxy_pref_xml, "spinbutton_http_port");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(temp),
							  cfg_get_single_value_as_int_with_default(config, "Network Settings", "Proxy Port", 8080));

	/* use auth */
	temp = gtk_builder_get_object(proxy_pref_xml, "checkbutton_use_auth");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp),
								 cfg_get_single_value_as_int_with_default(config, "Network Settings",
																		  "Use authentication", FALSE));

	/* username */
	temp = gtk_builder_get_object(proxy_pref_xml, "entry_auth_username");
	string = cfg_get_single_value_as_string(config, "Network Settings", "Proxy authentication username");
	if (string)
	{
		gtk_entry_set_text(GTK_ENTRY(temp), string);
		g_free(string);
	}

	/* username */
	temp = gtk_builder_get_object(proxy_pref_xml, "entry_auth_password");
	string = cfg_get_single_value_as_string(config, "Network Settings", "Proxy authentication password");
	if (string)
	{
		gtk_entry_set_text(GTK_ENTRY(temp), string);
		g_free(string);
	}

	/* signal autoconnect */
	gtk_builder_connect_signals(proxy_pref_xml, NULL);
	/* Add to parent */
	temp = gtk_builder_get_object(proxy_pref_xml, "frame_proxy_settings");
	gtk_container_add(GTK_CONTAINER(container), GTK_WIDGET(temp));
	gtk_widget_show_all(container);
}

gmpcPrefPlugin proxyplug_pref = {
	.construct = proxy_pref_construct,
	.destroy = proxy_pref_destroy
};

gmpcPlugin proxyplug = {
	.name = N_("Proxy"),
	.version = {0, 0, 0}
	,
	.plugin_type = GMPC_INTERNALL,
	.pref = &proxyplug_pref
};

/**
 * LIBSOUP BASED ASYNC DOWNLOADER
 */

static void gmpc_easy_async_free_handler_real(GEADAsyncHandler * handle);
typedef struct
{
	SoupMessage *msg;
	gchar *uri;
	GEADAsyncCallback callback;
	gpointer userdata;
	gchar *data;
	goffset length;
	int is_gzip;
	int is_deflate;
	z_stream *z;
	gpointer extra_data;
	guint uid;
	guint old_status_code;
} _GEADAsyncHandler;

static guint uid = 0;
static void gmpc_easy_async_headers_update(SoupMessage * msg, gpointer data)
{
	_GEADAsyncHandler *d = data;
	const gchar *encoding = soup_message_headers_get(msg->response_headers, "Content-Encoding");
	goffset size = soup_message_headers_get_content_length(msg->response_headers);
	g_log("EasyDownload", G_LOG_LEVEL_DEBUG,
			  "Expected download length: %u",(guint)size);
	if (encoding)
	{
		if (strcmp(encoding, "gzip") == 0)
		{
			d->is_gzip = 1;
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Url is gzipped");
		} else if (strcmp(encoding, "deflate") == 0)
		{
			d->is_deflate = 1;
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Url is enflated");
		}
	}

	/* If a second set comes in, close that */
	else
	{
		d->is_gzip = 0;
		d->is_deflate = 0;
	}

	/**
	 * Don't record data from redirection, in a while it _will_ be redirected,
	 * We care about that
	 */
	if (d->old_status_code != 0 && d->old_status_code != msg->status_code)
	{
		g_log("EasyDownload", G_LOG_LEVEL_DEBUG,
			  "Cleaning out the previous block of data: status_code:  %i(old) ->%i(new)",
			  d->old_status_code, msg->status_code);
		/* Clear buffer */
		g_free(d->data);
		d->data = NULL;
		d->length = 0;
		if (d->z)
			close_cb(d->z);
		d->z = NULL;
	}
	d->old_status_code = msg->status_code;
}

static void gmpc_easy_async_status_update(SoupMessage * msg, SoupBuffer * buffer, gpointer data)
{
	_GEADAsyncHandler *d = data;
	/* don't store error data, not used anyway */
	if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, 
						"Error mesg status code: %i\n", msg->status_code);
		return;
	}
	if (d->is_gzip || d->is_deflate)
	{
		if (d->z == NULL)
		{
			long data_start;
			d->z = g_malloc0(sizeof(*d->z));
			data_start = (d->is_gzip == 1) ? skip_gzip_header(buffer->data, buffer->length) : 0;
			d->z->next_in = (void *)((buffer->data) + data_start);
			d->z->avail_in = buffer->length - data_start;
            d->z->zalloc = NULL;
            d->z->zfree = NULL;
            d->z->opaque = NULL;
			if (inflateInit2(d->z, -MAX_WBITS) == Z_OK)
			{
				int res = 0;
				do
				{
					d->data = g_realloc(d->data, d->length + 12 * 1024 + 1);
					res = read_cb(d->z, &(d->data[d->length]), 12 * 1024);

					if (res > 0)
						d->length += res;

					d->data[d->length] = '\0';
				} while (res > 0);
				if (res < 0)
				{
					g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failure during unzipping 1: %s", d->uri);
					soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_MALFORMED);
				}
			} else
			{
				/* give error */
				g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failure during inflateInit2: %s", d->uri);
				soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_MALFORMED);
			}
		} else
		{
			int res = 0;
			d->z->next_in = (void *)((buffer->data));
			d->z->avail_in = buffer->length;
			do
			{
				d->data = g_realloc(d->data, d->length + 12 * 1024 + 1);
				res = read_cb(d->z, &(d->data[d->length]), 12 * 1024);

				if (res > 0)
					d->length += res;

				d->data[d->length] = '\0';
			} while (res > 0);
			if (res < 0)
			{
				g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failure during unzipping 2: %s", d->uri);
				soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_MALFORMED);
			}
		}

	} else
	{
		d->data = g_realloc(d->data, d->length + buffer->length + 1);
		g_memmove(&(d->data[d->length]), buffer->data, (size_t) buffer->length);
		d->length += buffer->length;
		d->data[d->length] = '\0';
	}
	d->callback((GEADAsyncHandler *) d, GEAD_PROGRESS, d->userdata);
}

static void gmpc_easy_async_callback(SoupSession * session, SoupMessage * msg, gpointer data)
{
	_GEADAsyncHandler *d = data;
	if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code))
	{
		d->callback((GEADAsyncHandler *) d, GEAD_DONE, d->userdata);
	} else if (msg->status_code == SOUP_STATUS_CANCELLED)
	{
		d->callback((GEADAsyncHandler *) d, GEAD_CANCELLED, d->userdata);
	} else
	{
		d->callback((GEADAsyncHandler *) d, GEAD_FAILED, d->userdata);
	}
	gmpc_easy_async_free_handler_real((GEADAsyncHandler *) d);
}

static void gmpc_easy_async_free_handler_real(GEADAsyncHandler * handle)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	if (d->z)
		close_cb(d->z);
	g_free(d->uri);
	g_free(d->data);
	g_free(d);
}

/**
 * Get the total size of the download, if available.
 */
goffset gmpc_easy_handler_get_content_size(const GEADAsyncHandler * handle)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	return soup_message_headers_get_content_length(d->msg->response_headers);
}

const char *gmpc_easy_handler_get_uri(const GEADAsyncHandler * handle)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	return d->uri;
}

const char *gmpc_easy_handler_get_data(const GEADAsyncHandler * handle, goffset * length)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	if (length)
		*length = d->length;
	return d->data;
}

const guchar *gmpc_easy_handler_get_data_vala_wrap(const GEADAsyncHandler * handle, gint * length)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	if (length)
		*length = (gint) d->length;
	return (guchar *) d->data;
}
const char *gmpc_easy_handler_get_data_as_string(const GEADAsyncHandler * handle)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	g_assert(d->data[d->length] == '\0');
	return (gchar *) d->data;
}
void gmpc_easy_handler_set_user_data(const GEADAsyncHandler * handle, gpointer user_data)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	d->extra_data = user_data;
}

gpointer gmpc_easy_handler_get_user_data(const GEADAsyncHandler * handle)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	return d->extra_data;
}

void gmpc_easy_async_cancel(const GEADAsyncHandler * handle)
{
	_GEADAsyncHandler *d = (_GEADAsyncHandler *) handle;
	soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_CANCELLED);
}

GEADAsyncHandler *gmpc_easy_async_downloader(const gchar * uri, GEADAsyncCallback callback, gpointer user_data)
{
	if (uri == NULL)
	{
		g_log(LOG_DOMAIN,G_LOG_LEVEL_WARNING, "No download uri specified.");
		return NULL;
	}
	return gmpc_easy_async_downloader_with_headers(uri, callback, user_data, NULL);
}

GEADAsyncHandler *gmpc_easy_async_downloader_with_headers(const gchar * uri, GEADAsyncCallback callback,
														  gpointer user_data, ...)
{
	SoupMessage *msg;
	_GEADAsyncHandler *d;
	va_list ap;
	char *va_entry;
	if (soup_session == NULL)
	{
		soup_session = soup_session_async_new();
		gmpc_easy_download_set_proxy(soup_session);
		g_object_set(soup_session, "timeout", 5, NULL);
		/* Set user agent, to get around wikipedia ban. */
		g_object_set(soup_session, "user-agent", "gmpc ",NULL);
	}

	msg = soup_message_new("GET", uri);
	if (!msg)
		return NULL;

	soup_message_headers_append(msg->request_headers, "Accept-Encoding", "deflate,gzip");
	va_start(ap, user_data);
	va_entry = va_arg(ap, typeof(va_entry));
	while (va_entry)
	{
		char *value = va_arg(ap, typeof(value));
		soup_message_headers_append(msg->request_headers, va_entry, value);
		va_entry = va_arg(ap, typeof(va_entry));
	}
	va_end(ap);

	d = g_malloc0(sizeof(*d));
	d->uid = ++uid;
	d->is_gzip = 0;
	d->is_deflate = 0;
	d->z = NULL;
	d->data = NULL;
	d->msg = msg;
	d->uri = g_strdup(uri);
	d->callback = callback;
	d->userdata = user_data;
	d->extra_data = NULL;
	d->old_status_code = 0;
	soup_message_body_set_accumulate(msg->response_body, FALSE);
	g_signal_connect_after(msg, "got-chunk", G_CALLBACK(gmpc_easy_async_status_update), d);
	g_signal_connect_after(msg, "got-headers", G_CALLBACK(gmpc_easy_async_headers_update), d);
	soup_session_queue_message(soup_session, msg, gmpc_easy_async_callback, d);

	return (GEADAsyncHandler *) d;
}

void gmpc_easy_async_quit(void)
{
	if (soup_session)
	{
		soup_session_abort(soup_session);
		g_object_unref(soup_session);
		soup_session = NULL;
	}
}

char *gmpc_easy_download_uri_escape(const char *part)
{
	return soup_uri_encode(part, "&+");
}

/**************************************************************/


typedef struct {
	void *a;
	void *b;
	GEADAsyncCallbackVala callback;
} valaf;

static void temp_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
	valaf *f = (valaf*)user_data;
	f->callback(handle, status, f->b, f->a);
	if(status != GEAD_PROGRESS) g_free(f);
}
GEADAsyncHandler * gmpc_easy_async_downloader_vala(const char *path, gpointer user_data2, GEADAsyncCallbackVala callback,
														  gpointer user_data
														  )
{
	valaf *f = g_malloc0(sizeof(*f));
	f->a = user_data;
	f->b =user_data2;
	f->callback = callback;
	return gmpc_easy_async_downloader(path, temp_callback, f);
}


/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
