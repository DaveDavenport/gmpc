/* gmpc-libnotify (GMPC plugin)
 * Copyright (C) 2007-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/

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
#include <string.h>
#include <config.h>

#ifdef HAVE_LIBNOTIFY

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libnotify/notify.h>
#include <gmpc/plugin.h>
#include <gmpc/metadata.h>
#include <gmpc/misc.h>
#include <config.h>

#define LOG_DOMAIN "LibNotifyPlugin"



GdkPixbuf *pixbuf_cache_lookup_icon(int size, const gchar *mdd);

extern GtkStatusIcon *tray_icon2_gsi;
static NotifyNotification *not = NULL;
static gulong data_changed = 0;
static void libnotify_update_cover(GmpcMetaWatcher *gmv, mpd_Song *song, MetaDataType type, MetaDataResult ret, MetaData *met, gpointer data);

static void libnotify_plugin_destroy(void)
{
	if(not)
		notify_notification_close(not,NULL);
	if(data_changed) {
		g_signal_handler_disconnect(G_OBJECT(gmw), data_changed);
		data_changed = 0;
	}
	not = NULL;
}
static void libnotify_plugin_init(void)
{
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	notify_init("gmpc");
	data_changed = g_signal_connect(G_OBJECT(gmw), "data-changed", G_CALLBACK(libnotify_update_cover), NULL);
}

static const gchar *libnotify_get_translation_domain(void)
{
	return GETTEXT_PACKAGE;
}

static void libnotify_update_cover(GmpcMetaWatcher *gmv, mpd_Song *song, MetaDataType type, MetaDataResult ret, MetaData *met, gpointer data)
{
	mpd_Song  *song2;
	if(!not) return;
    if(type != META_ALBUM_ART) return;

	song2 = g_object_get_data(G_OBJECT(not), "mpd-song");

    // Compare if callback is about 'our' song.
    // TODO: optimize, by keeping checksum of current song around?
    {
        char *ck_a = mpd_song_checksum_type(song, META_ALBUM_ART);
        char *ck_b = mpd_song_checksum_type(song2, META_ALBUM_ART);
        if(ck_a == NULL || ck_b == NULL || strcmp(ck_a, ck_b) != 0) {
            g_free(ck_a);
            g_free(ck_b);
            return;
        }
        g_free(ck_a);
        g_free(ck_b);
    }

	if(ret == META_DATA_AVAILABLE) {
		if(met->content_type == META_DATA_CONTENT_RAW)
		{
            GError *error = NULL;

            GdkPixbuf *pb = pixbuf_cache_lookup_icon(64,(const char *)met->md5sum);
            printf("libnotify: %p\n", pb);
            if(pb == NULL) {
                GdkPixbufLoader *pb_l = gdk_pixbuf_loader_new();
                gdk_pixbuf_loader_set_size(pb_l, 64,64);
                printf("libnotify: size: %lu\n", met->size);
                gdk_pixbuf_loader_write(pb_l, met->content, met->size, &error);
                if(error == NULL) {
                    gdk_pixbuf_loader_close(pb_l, &error);
                    if(error == NULL){
                        pb = gdk_pixbuf_loader_get_pixbuf(pb_l);
                        g_object_ref(pb);
                    }
                }
                g_object_unref(pb_l);
            }
            if(error == NULL && pb != NULL)
            {
                screenshot_add_border(pb);
            }else{
                if(error) {
                    printf("libnotify: %s\n", error->message);
                    g_error_free(error);
                }
                if(pb) g_object_unref(pb);
                pb = NULL;
            }
            printf("libnotify: %p\n", pb);
			/* Default to gmpc icon when no icon is found */
			if(!pb) {
				pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),"gmpc" , 64, 0,NULL);
				if(!pb) return; /* should never happen, bail out */
			}
			notify_notification_set_image_from_pixbuf(not, pb);
			if(pb)
				g_object_unref(pb);
		}
	} else if (ret == META_DATA_FETCHING) {
		GdkPixbuf *pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),"gmpc-loading-cover" , 64, 0,NULL);
		/* Default to gmpc icon when no icon is found */
		if(!pb) {
			pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),"gmpc" , 64, 0,NULL);
			if(!pb) return; /* should never happen, bail out */
		}
		notify_notification_set_image_from_pixbuf(not, pb);
		if(pb)
			g_object_unref(pb);
	} else {
		GdkPixbuf *pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),"gmpc" , 64, 0,NULL);
		if(pb == NULL)
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Failed to load gmpc icon");
		notify_notification_set_image_from_pixbuf(not, pb);
		if(pb)
			g_object_unref(pb);
	}
}

static int libnotify_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "libnotify-plugin", "enable", FALSE);
}
static void libnotify_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "libnotify-plugin", "enable", enabled);
}

static void libnotify_song_changed(MpdObj *mi)
{
	mpd_Song *song = NULL;
	if(!cfg_get_single_value_as_int_with_default(config, "libnotify-plugin", "enable", FALSE))
		return;
	song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		MetaDataResult ret;
		MetaData *met= NULL;
		gchar buffer[1024];
		gchar *summary;
		gchar *version = NULL, *ret_name = NULL, *ret_vendor = NULL, *ret_spec_version = NULL;
		int *versions;
		GdkPixbuf *pb=NULL;

		notify_get_server_info(&ret_name, &ret_vendor, &version, &ret_spec_version);
		if(version){
			versions = split_version(version);
		}else{
			versions = g_malloc0(4*sizeof(int));
		}
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "libnotify version: %i %i %i\n", versions[0], versions[1], versions[2]);
		if((versions[0] > 0) || (versions[0] == 0 && versions[1] >= 4))
			mpd_song_markup(buffer, 1024, C_("Summary markup","%title%|%name%|%shortfile%"), song);
		else
			mpd_song_markup_escaped(buffer, 1024, "%title%|%name%|%shortfile%", song);
		summary = g_strdup(buffer);
		mpd_song_markup_escaped(buffer, 1024,
				(char *)C_("Body markup", "[<b>Artist:</b> %artist%\n][<b>Album:</b> %album% [(%date%)]\n][<b>Genre:</b> %genre%\n]"),
				song);
		/* if notification exists update it, else create one */
		if(not == NULL)
		{
			//            notify_notification_close(not, NULL);
			not = notify_notification_new(summary, buffer,NULL);
		}
		else{
			notify_notification_update(not, summary, buffer, NULL);
		}
		notify_notification_set_urgency(not, NOTIFY_URGENCY_LOW);

		g_free(summary);
		/* Add the song to the widget */
		g_object_set_data_full(G_OBJECT(not), "mpd-song", mpd_songDup(song),  (GDestroyNotify)mpd_freeSong);

		pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),"gmpc" , 64, 0,NULL);
		if(pb){
			notify_notification_set_image_from_pixbuf(not, pb);
			g_object_unref(pb);
		}
		ret = meta_data_get_path(song, META_ALBUM_ART,&met, NULL, NULL);
		libnotify_update_cover(gmw, song, META_ALBUM_ART, ret, met, NULL);
		if(met)
			meta_data_free(met);

		if(!notify_notification_show(not, NULL))
		{
			notify_notification_close(not,NULL);
			not = NULL;
		}
		if(ret_name)
			g_free(ret_name);
		if(ret_vendor)
			g_free(ret_vendor);
		if(ret_spec_version)
			g_free(ret_spec_version);
		if(version)
			g_free(version);
		g_free(versions);
	}
}

/* mpd changed */
static void   libnotify_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
	if(what&(MPD_CST_SONGID))
		libnotify_song_changed(mi);
}

/* main plugin_osd info */
gmpcPlugin libnotify_plugin = {
	.name 			    = "Libnotify based notifications",
	.version            = {0,0,2},
	.plugin_type 		= GMPC_PLUGIN_NO_GUI|GMPC_INTERNALL,
	.init 			    = libnotify_plugin_init,
	.destroy 		    = libnotify_plugin_destroy,
	.mpd_status_changed = libnotify_mpd_status_changed,
	.get_enabled 		= libnotify_get_enabled,
	.set_enabled 		= libnotify_set_enabled,

	.get_translation_domain = libnotify_get_translation_domain
};
#endif
