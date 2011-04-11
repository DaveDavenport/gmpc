/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@sarine.nl>
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
 
#include <gtk/gtk.h>
#include "main.h"
#include "title_header.h"
#include "playlist3.h"

/* glue code */
extern GmpcMetadataBrowser *metadata_browser;

static GtkWidget *header_labels[5];

static void playlist3_header_song(void)
{
    mpd_Song *song = mpd_playlist_get_current_song(connection);
    if (song)
    {
        GtkTreeView *tree = (GtkTreeView *) gtk_builder_get_object(pl3_xml, "cat_tree");
        gmpc_metadata_browser_select_browser(metadata_browser, tree);
        gmpc_metadata_browser_set_song(metadata_browser, song);
    }
}


static void playlist3_header_artist(void)
{
    mpd_Song *song = mpd_playlist_get_current_song(connection);
    if (song && song->artist)
    {
        GtkTreeView *tree = (GtkTreeView *) gtk_builder_get_object(pl3_xml, "cat_tree");
        gmpc_metadata_browser_select_browser(metadata_browser, tree);
        gmpc_metadata_browser_set_artist(metadata_browser, song->artist);
    }
}


static void playlist3_header_album(void)
{
    mpd_Song *song = mpd_playlist_get_current_song(connection);
    if (song && song->artist && song->album)
    {
        GtkTreeView *tree = (GtkTreeView *) gtk_builder_get_object(pl3_xml, "cat_tree");
        gmpc_metadata_browser_select_browser(metadata_browser, tree);
        gmpc_metadata_browser_set_album(metadata_browser, song->artist, song->album);
    }
}


void playlist3_new_header(void)
{
    GtkWidget *hbox10 = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hbox10"));
    if (hbox10)
    {
        GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
        GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

        gtk_widget_set_size_request(hbox, 250, -1);
        /** Title */
        header_labels[0] = (GtkWidget *)gmpc_clicklabel_new("");
        gmpc_clicklabel_font_size(GMPC_CLICKLABEL(header_labels[0]), 18);
        gmpc_clicklabel_set_ellipsize(GMPC_CLICKLABEL(header_labels[0]), PANGO_ELLIPSIZE_END);

        header_labels[1] = gtk_label_new(_("By"));
        /** Artist */
        header_labels[2] = (GtkWidget *)gmpc_clicklabel_new("");
        gmpc_clicklabel_set_ellipsize(GMPC_CLICKLABEL(header_labels[2]), PANGO_ELLIPSIZE_NONE);
        gmpc_clicklabel_set_do_italic(GMPC_CLICKLABEL(header_labels[2]), TRUE);

        header_labels[3] = gtk_label_new(_("From"));
        /** Albumr */
        header_labels[4] = (GtkWidget *)gmpc_clicklabel_new("");
        gmpc_clicklabel_set_do_italic(GMPC_CLICKLABEL(header_labels[4]), TRUE);
        gmpc_clicklabel_set_ellipsize(GMPC_CLICKLABEL(header_labels[4]), PANGO_ELLIPSIZE_END);

        gtk_box_pack_start(GTK_BOX(vbox), header_labels[0], FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(hbox), header_labels[1], FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), header_labels[2], FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), header_labels[3], FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), header_labels[4], TRUE, TRUE, 0);

        g_signal_connect(G_OBJECT(header_labels[0]), "clicked", G_CALLBACK(playlist3_header_song), NULL);
        g_signal_connect(G_OBJECT(header_labels[2]), "clicked", G_CALLBACK(playlist3_header_artist), NULL);
        g_signal_connect(G_OBJECT(header_labels[4]), "clicked", G_CALLBACK(playlist3_header_album), NULL);
        gtk_box_pack_start(GTK_BOX(hbox10), vbox, TRUE, TRUE, 0);
        gtk_widget_show_all(hbox10);
    }
}


void playlist3_update_header(void)
{
    char buffer[1024];
    if (header_labels[0] == NULL)
        return;

    if (mpd_check_connected(connection))
    {
        mpd_Song *song = mpd_playlist_get_current_song(connection);
        /** Set new header */
        if (mpd_player_get_state(connection) != MPD_STATUS_STATE_STOP && song)
        {
            mpd_song_markup(buffer, 1024, "[%title%|%shortfile%][ (%name%)]", song);
            gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[0]), buffer);
            if (song->artist)
            {
                gtk_widget_show(header_labels[1]);
                gtk_widget_show(header_labels[2]);
                gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[2]), song->artist);
            } else
            {
                gtk_widget_hide(header_labels[1]);
                gtk_widget_hide(header_labels[2]);
            }
            if (song->album)
            {
                gtk_widget_show(header_labels[3]);
                gtk_widget_show(header_labels[4]);
                if (song->date)
                {
                    gchar *text = g_strdup_printf("%s (%s)", song->album, song->date);
                    gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[4]), text);
                    g_free(text);
                } else
                {
                    gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[4]), song->album);
                }

            } else
            {
                gtk_widget_hide(header_labels[3]);
                gtk_widget_hide(header_labels[4]);
            }

        } else
        {
            gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[0]), _("Not Playing"));
            gtk_widget_hide(header_labels[1]);
            gtk_widget_hide(header_labels[2]);
            gtk_widget_hide(header_labels[3]);
            gtk_widget_hide(header_labels[4]);
        }
    } else
    {
        gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[0]), _("Not Connected"));
        gtk_widget_hide(header_labels[1]);
        gtk_widget_hide(header_labels[2]);
        gtk_widget_hide(header_labels[3]);
        gtk_widget_hide(header_labels[4]);

    }
}

