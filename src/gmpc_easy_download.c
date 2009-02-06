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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <curl/curl.h>
#include <libmpd/debug_printf.h>
#include <libsoup/soup.h>
#include <zlib.h>
#include "gmpc_easy_download.h"
#include "main.h"
#define CURL_TIMEOUT 10 

static int quit = FALSE;
/****
 * ZIP MISC
 * **********/
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
static char gz_magic[2] = {0x1f, 0x8b};
static
int skip_gzip_header(const unsigned char * src, gsize size) {
    int idx;
    if (size < 10 || memcmp(src, gz_magic, 2))
        return -1;
    if (src[2] != Z_DEFLATED) {
        fprintf(stderr, "unsupported compression method (%d).\n", (int)src[3]);
        return -1;
    }
    idx = 10;          
    /* skip past header, mtime and xos */

    if (src[3] & EXTRA_FIELD)
        idx += src[idx] + (src[idx+1]<<8) + 2;
    if (src[3] & ORIG_NAME) {
        while (src[idx]) idx++; idx++;
    }
    if (src[3] & COMMENT) {
        while (src[idx]) idx++; idx++; 
    }
    if (src[3] & HEAD_CRC)
        idx += 2;
    return idx;
}

static int read_cb(void *z, char *buffer, int size)
{
    z_stream *zs = z;
    if(zs){
        zs->next_out = (void *)buffer;
        zs->avail_out = size;
        int r = inflate(zs, Z_SYNC_FLUSH);
        if(r == Z_OK || r == Z_STREAM_END || r == Z_NEED_DICT){
            return size-zs->avail_out;
        }
    }
    printf("failed unzipping stream\n");
    return -1;
}
static int close_cb (void *z)
{
    printf("Close unzip stream\n");
    z_stream *zs = z;
    inflateEnd(zs);
    g_free(zs);
    return 0;
}



static SoupSession *soup_session = NULL;

static void gmpc_easy_download_set_proxy(SoupSession *session)
{
    if(session == NULL)
        return;

    if(cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use Proxy", FALSE))
    {
            char *value = cfg_get_single_value_as_string(config, "Network Settings", "Proxy Address");
            char *username = NULL;
            char *password = NULL;
            gint port =  cfg_get_single_value_as_int_with_default(config, "Network Settings", "Proxy Port",8080);
            if(cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use authentication", FALSE))
            {
                password = cfg_get_single_value_as_string(config, "Network Settings", "Password");
                username = cfg_get_single_value_as_string(config, "Network Settings", "Username");
            }
            if(value)
            {
                SoupURI *uri = NULL; 
                gchar *ppath = NULL;
                if(username && username[0] != '\0' && password && password[0] != '\0')
                {
                    gchar *usere = gmpc_easy_download_uri_escape(username);
                    gchar *passe = gmpc_easy_download_uri_escape(password);
                    ppath = g_strdup_printf("http://%s:%s@%s:%i",usere, passe, value, port);
                    g_free(usere);g_free(passe);
                }else if (username && username[0] != '\0')
                {
                    gchar *usere = gmpc_easy_download_uri_escape(username);
                    ppath = g_strdup_printf("http://%s@%s:%i",usere, value, port);
                    g_free(usere);
                }else{
                    ppath = g_strdup_printf("http://%s:%i", value, port);
                }
                uri= soup_uri_new(ppath);
                g_object_set(G_OBJECT(session), SOUP_SESSION_PROXY_URI, uri,NULL);
                soup_uri_free(uri);
                g_free(ppath);
            }
            g_free(username);
            g_free(password);
            g_free(value);
    }else {
        g_object_set(G_OBJECT(session), SOUP_SESSION_PROXY_URI, NULL,NULL);
    }
}


int gmpc_easy_download_with_headers(const char *url,gmpc_easy_download_struct *dld,...)
{

    SoupSession *session = NULL;
    SoupMessage *msg = NULL;
    int status;
	int success = FALSE;
    va_list ap;
    char *va_entry;
	/*int res;*/
	if(!dld) return 0;
	if(url == NULL) return 0;
	if(url[0] == '\0') return 0;
	/**
	 * Make sure it's clean
	 */
	gmpc_easy_download_clean(dld);

    session = soup_session_sync_new();
    gmpc_easy_download_set_proxy(session);
    /** Check for local url */
    if(strncmp(url, "http://", 7) && g_file_test(url, G_FILE_TEST_EXISTS))
    {
        gsize size;
        if(g_file_get_contents(url, &(dld->data),&(size), NULL))
        {
            dld->size = (int)size;
            return 1;
        }
        return 0;
    }

    if(cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use Proxy", FALSE))
    {
            char *value = cfg_get_single_value_as_string(config, "Network Settings", "Proxy Address");
            gint port =  cfg_get_single_value_as_int_with_default(config, "Network Settings", "Proxy Port",8080);
            if(value)
            {
                gchar *ppath = g_strdup_printf("http://%s:%i", value, port);
                SoupURI *uri = soup_uri_new(ppath);
                session = soup_session_sync_new_with_options(SOUP_SESSION_PROXY_URI, uri,NULL);
                soup_uri_free(uri);
                g_free(ppath);
                g_free(value);
            }
    }
    if(!session){
        session = soup_session_sync_new();
    }

    msg = soup_message_new("GET",url);


    soup_message_headers_append(msg->request_headers, "Accept-Encoding","gzip,deflate");

    va_start(ap, dld);
    va_entry = va_arg(ap, typeof(va_entry));
    while(va_entry)
    {
        char *value = va_arg(ap, typeof(value));
        soup_message_headers_append(msg->request_headers, va_entry, value);
        va_entry = va_arg(ap, typeof(va_entry));
    }
    va_end(ap);

    status = soup_session_send_message(session, msg);
    if(SOUP_STATUS_IS_SUCCESSFUL (status)){
        const gchar *encoding = soup_message_headers_get(msg->response_headers, "Content-Encoding");
        soup_message_body_flatten(msg->response_body);

        if(encoding && (strcmp(encoding, "gzip") == 0 || strcmp(encoding, "deflate") == 0))
        {
            /* 12k buffer */
            char *new_buffer=NULL;
            int size;
            z_stream *zs = g_malloc0(sizeof(*zs));
            long data_start = (strcmp(encoding,"gzip") == 0)?skip_gzip_header(msg->response_body->data,msg->response_body->length):0;
            if (data_start != -1){
                zs->next_in  = (void *)((msg->response_body->data)+ data_start);
                zs->avail_in = msg->response_body->length - data_start;
                if (inflateInit2(zs, -MAX_WBITS) == Z_OK)
                {
                    long total_size = 0;
                    int res;
                    do{
                        new_buffer = g_realloc(new_buffer, (total_size+(12*1024))*sizeof(char));
                        res = read_cb(zs, &new_buffer[total_size],12*1024);
                        if(res > 0) total_size+=res;
                    }while(res > 0);
                    if(res == 0){
                        dld->data = new_buffer;
                        dld->size = (int)total_size;
                        success = 1;
                    }else{
                        g_free(new_buffer);
                    }
                }
            }
            close_cb(zs);
        }
        else
        {
            /* copy libsoup's buffer */
            dld->size = msg->response_body->length;
            dld->data = (char *)g_memdup(msg->response_body->data, (guint)msg->response_body->length);
            success = 1;
        }
    }
    else 
    {
        success = 0;
    }
    g_object_unref(msg);
    g_object_unref(session);
	if(success) return 1;
	if(dld->data) q_free(dld->data);
	dld->data = NULL;
	return 0;
}

int gmpc_easy_download(const char *url,gmpc_easy_download_struct *dld)
{
    return gmpc_easy_download_with_headers(url, dld, NULL);
}


void gmpc_easy_download_clean(gmpc_easy_download_struct *dld)
{
	if(dld->data)q_free(dld->data);
	dld->data = NULL;
	dld->size = 0;
}

/***
 * preferences window
 */
/* for gtkbuilder */
void proxy_pref_use_proxy_toggled(GtkWidget *toggle_button);
void proxy_pref_http_address_changed(GtkWidget *entry);
void proxy_pref_http_port_changed(GtkWidget *entry);
void proxy_pref_use_auth_toggled(GtkWidget *toggle_button);
void proxy_pref_auth_username_changed(GtkWidget *entry);
void proxy_pref_auth_password_changed(GtkWidget *entry);

static GtkBuilder *proxy_pref_xml = NULL;
static void proxy_pref_destroy(GtkWidget *container)
{
    GObject *temp = gtk_builder_get_object(proxy_pref_xml, "frame_proxy_settings");
	gtk_container_remove(GTK_CONTAINER(container), GTK_WIDGET(temp));
    g_object_unref(proxy_pref_xml);
    proxy_pref_xml = NULL;
}
void proxy_pref_use_proxy_toggled(GtkWidget *toggle_button)
{
	cfg_set_single_value_as_int(config, "Network Settings", "Use Proxy",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
    gmpc_easy_download_set_proxy(soup_session);
}
void proxy_pref_http_address_changed(GtkWidget *entry)
{
    cfg_set_single_value_as_string(config, "Network Settings", "Proxy Address",(char *)gtk_entry_get_text(GTK_ENTRY(entry)));
    gmpc_easy_download_set_proxy(soup_session);
}
void proxy_pref_http_port_changed(GtkWidget *entry)
{
    cfg_set_single_value_as_int(config, "Network Settings", "Proxy Port",gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry)));
    gmpc_easy_download_set_proxy(soup_session);
}
void proxy_pref_use_auth_toggled(GtkWidget *toggle_button)
{
	cfg_set_single_value_as_int(config, "Network Settings", "Use authentication",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
    gmpc_easy_download_set_proxy(soup_session);
}
void proxy_pref_auth_username_changed(GtkWidget *entry)
{
    cfg_set_single_value_as_string(config, "Network Settings", "Proxy authentication username",(char *)gtk_entry_get_text(GTK_ENTRY(entry)));
    gmpc_easy_download_set_proxy(soup_session);
}
void proxy_pref_auth_password_changed(GtkWidget *entry)
{
    cfg_set_single_value_as_string(config, "Network Settings", "Proxy authentication password",gtk_entry_get_text(GTK_ENTRY(entry)));
    gmpc_easy_download_set_proxy(soup_session);
}

static void proxy_pref_construct(GtkWidget *container)
{
    GObject *temp = NULL;
    gchar *string;
	gchar *path = gmpc_get_full_glade_path("preferences-proxy.ui");
	proxy_pref_xml = gtk_builder_new();
    gtk_builder_add_from_file(proxy_pref_xml, path, NULL);
	q_free(path);
    /* use proxy */
    temp = gtk_builder_get_object(proxy_pref_xml, "checkbutton_use_proxy");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), 
			cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use Proxy", FALSE));

    /* hostname */
    temp = gtk_builder_get_object(proxy_pref_xml, "entry_http_hostname");
    string = cfg_get_single_value_as_string(config, "Network Settings", "Proxy Address");
    if(string)
    {
        gtk_entry_set_text(GTK_ENTRY(temp),string); 
        g_free(string);
    }

    /* port */
    temp = gtk_builder_get_object(proxy_pref_xml, "spinbutton_http_port");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(temp), 
			cfg_get_single_value_as_int_with_default(config, "Network Settings", "Proxy Port",8080));

    /* use auth*/
    temp = gtk_builder_get_object(proxy_pref_xml, "checkbutton_use_auth");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), 
			cfg_get_single_value_as_int_with_default(config, "Network Settings", "Use authentication", FALSE));

    /* username */
    temp = gtk_builder_get_object(proxy_pref_xml, "entry_auth_username");
    string = cfg_get_single_value_as_string(config, "Network Settings", "Proxy authentication username");
    if(string)
    {
        gtk_entry_set_text(GTK_ENTRY(temp),string); 
        g_free(string);
    }

    /* username */
    temp = gtk_builder_get_object(proxy_pref_xml, "entry_auth_password");
    string = cfg_get_single_value_as_string(config, "Network Settings", "Proxy authentication password");
    if(string)
    {
        gtk_entry_set_text(GTK_ENTRY(temp),string); 
        g_free(string);
    }

    /* signal autoconnect */
    gtk_builder_connect_signals(proxy_pref_xml, NULL);
    /* Add to parent */
    temp = gtk_builder_get_object(proxy_pref_xml, "frame_proxy_settings");
	gtk_container_add(GTK_CONTAINER(container), GTK_WIDGET(temp));
}

void quit_easy_download(void)
{
    debug_printf(DEBUG_INFO,"quitting easy download\n");
    quit = TRUE;
}
gmpcPrefPlugin proxyplug_pref = {
	.construct		= proxy_pref_construct,
	.destroy		= proxy_pref_destroy
};
gmpcPlugin proxyplug = {
	.name 			= N_("Proxy"),
	.version 	 	= {0,0,0},
	.plugin_type	= GMPC_INTERNALL,
	.pref			= &proxyplug_pref
};

/**
 * LIBSOUP BASED ASYNC DOWNLOADER
 */
typedef struct {
    SoupMessage *msg;
    gchar *uri;
    GEADAsyncCallback callback; 
    gpointer userdata;
    gchar *data;
    goffset length;
    int is_gzip;
    int is_deflate;
    z_stream *z;
}_GEADAsyncHandler;
static void gmpc_easy_async_headers_update(SoupMessage *msg, gpointer data)
{
    const gchar *encoding = soup_message_headers_get(msg->response_headers, "Content-Encoding");
    _GEADAsyncHandler *d = data;
    if(encoding) {
        if(strcmp(encoding, "gzip") == 0)
        {
            d->is_gzip = 1;
        }else if (strcmp(encoding, "deflate") == 0) {
            d->is_deflate= 1;
        }
    }
}

static void gmpc_easy_async_status_update(SoupMessage *msg, SoupBuffer *buffer, gpointer data)
{
    _GEADAsyncHandler *d = data;
    if(d->is_gzip||d->is_deflate) {
        if(d->z == NULL) {
            d->z= g_malloc0(sizeof(*d->z));
            long data_start = (d->is_gzip ==1)?skip_gzip_header(buffer->data,buffer->length):0;
            d->z->next_in  = (void *)((buffer->data)+ data_start);
            d->z->avail_in = buffer->length - data_start;
            if (inflateInit2(d->z, -MAX_WBITS) == Z_OK){
                int res = 0;
                do{
                    d->data = g_realloc(d->data, d->length+12*1024);
                    res = read_cb(d->z, &(d->data[d->length]), 12*1024);
                    if(res>0)
                        d->length += res;
                }while(res >0);
                if(res < 0) {
                    soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_MALFORMED);
                }
            }
            else
            {
                /* give error*/
                soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_MALFORMED);
            }
        }
        else{
            d->z->next_in  = (void *)((buffer->data));
            d->z->avail_in = buffer->length;
            int res = 0;
            do{
                d->data = g_realloc(d->data, d->length+12*1024);
                res = read_cb(d->z, &(d->data[d->length]), 12*1024);
                if(res>0)
                    d->length += res;
                }while(res >0);
                if(res < 0) {
                    soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_MALFORMED);
                }
        }

    }else{
        d->data = g_realloc(d->data, d->length+buffer->length);
        g_memmove(&(d->data[d->length]), buffer->data, (size_t)buffer->length);
        d->length += buffer->length;
    }
    d->callback((GEADAsyncHandler *)d,GEAD_PROGRESS, d->userdata);
}

static void gmpc_easy_async_callback(SoupSession *session, SoupMessage *msg, gpointer data)
{
    _GEADAsyncHandler *d = data;
    if(SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)){
        d->callback((GEADAsyncHandler *)d,GEAD_DONE, d->userdata);
    }
    else if (msg->status_code == SOUP_STATUS_CANCELLED)
    {
        d->callback((GEADAsyncHandler *)d,GEAD_CANCELLED,d->userdata);
    }
    else {
        d->callback((GEADAsyncHandler *)d,GEAD_FAILED,d->userdata);
    }
}


void gmpc_easy_async_free_handler(GEADAsyncHandler *handle)
{
    _GEADAsyncHandler *d = (_GEADAsyncHandler *)handle;
    if(d->z) close_cb(d->z);
    g_free(d->uri);
    g_free(d->data);
    g_free(d);
}

goffset gmpc_easy_handler_get_content_size(GEADAsyncHandler *handle)
{
    _GEADAsyncHandler *d = (_GEADAsyncHandler *)handle;
    return soup_message_headers_get_content_length(d->msg->response_headers);
}

const char  * gmpc_easy_handler_get_uri(GEADAsyncHandler *handle)
{
    _GEADAsyncHandler *d = (_GEADAsyncHandler *)handle;
    return d->uri;
}

const char * gmpc_easy_handler_get_data(GEADAsyncHandler *handle, goffset *length)
{
    _GEADAsyncHandler *d = (_GEADAsyncHandler *)handle;
    if(length)
	    *length = d->length;
    return d->data;
}

void gmpc_easy_async_cancel(GEADAsyncHandler *handle)
{
    _GEADAsyncHandler *d = (_GEADAsyncHandler *)handle;
    soup_session_cancel_message(soup_session, d->msg, SOUP_STATUS_CANCELLED);
}

GEADAsyncHandler *gmpc_easy_async_downloader(const gchar *uri, GEADAsyncCallback callback, gpointer user_data)
{
    return gmpc_easy_async_downloader_with_headers(uri, callback, user_data, NULL);
}
GEADAsyncHandler *gmpc_easy_async_downloader_with_headers(const gchar *uri, GEADAsyncCallback callback, gpointer user_data,...)
{
    SoupMessage *msg;
    _GEADAsyncHandler *d;
    va_list ap;
    char *va_entry;
    if(soup_session == NULL) {
        soup_session = soup_session_async_new();
        gmpc_easy_download_set_proxy(soup_session);
    }

    msg = soup_message_new("GET", uri);
    if(!msg) return NULL;

    soup_message_headers_append(msg->request_headers, "Accept-Encoding","deflate,gzip");
    va_start(ap, user_data);
    va_entry = va_arg(ap, typeof(va_entry));
    while(va_entry)
    {
        char *value = va_arg(ap, typeof(value));
        soup_message_headers_append(msg->request_headers, va_entry, value);
        va_entry = va_arg(ap, typeof(va_entry));
    }
    va_end(ap);

    d = g_malloc0(sizeof(*d));
    d->is_gzip = 0;
    d->is_deflate=0;
    d->z = NULL;
    d->data = NULL;
    d->msg = msg;
    d->uri = g_strdup(uri);
    d->callback = callback;
    d->userdata = user_data;
    soup_message_body_set_accumulate(msg->response_body, FALSE);
    g_signal_connect_after(msg, "got-chunk", G_CALLBACK(gmpc_easy_async_status_update), d);
    g_signal_connect_after(msg, "got-headers", G_CALLBACK(gmpc_easy_async_headers_update), d);
    soup_session_queue_message(soup_session, msg, gmpc_easy_async_callback, d);
    
    return (GEADAsyncHandler*)d;
}


void gmpc_easy_async_quit(void)
{
    if(soup_session)
    {
        soup_session_abort(soup_session);
        g_object_unref(soup_session);
        soup_session = NULL;
    }
}

char *gmpc_easy_download_uri_escape(const char *part)
{
    return soup_uri_encode(part, NULL);
}
