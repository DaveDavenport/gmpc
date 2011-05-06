/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
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
#include "control_window.h"
#include "gmpc-extras.h"
#include "main.h"

/**
 * Reuse code from playlist3.
 */
extern GtkBuilder *pl3_xml;
gboolean playlist_player_volume_changed(GtkWidget * vol_but, int new_vol);
void pl3_pb_seek_event(GtkWidget * pb, guint seek_time, gpointer user_data);

/**
 * Draw background of control box.
 */
static gboolean
expose_window(GtkWidget *widget, GdkEventExpose *event, gpointer date)
{
    cairo_t *cr;
    cr = gdk_cairo_create(widget->window);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, .7);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);             /* paint source */

    cairo_set_source_rgba(cr, .8, .8, .8, .7);
    cairo_rectangle(cr, 0.5,0.5,
        widget->allocation.width-1.0,
        widget->allocation.height-1.0);
    cairo_stroke(cr);
    cairo_destroy(cr);

    return FALSE;
}


/**
 * change the colors recursively.
 * Background black, text/fg white.
 */
static void control_window_modify_colors(GtkWidget *base)
{
    GList *iter;
    if(GTK_IS_HSCALE(base) == FALSE)
    {
        gtk_widget_modify_bg(base, GTK_STATE_NORMAL,&(base->style->black));
        gtk_widget_modify_fg(base, GTK_STATE_NORMAL,&(base->style->white));
        gtk_widget_modify_text(base, GTK_STATE_NORMAL,&(base->style->white));
    }
    if(GTK_IS_CONTAINER(base))
    {
        for (iter = gtk_container_get_children(GTK_CONTAINER(base));
            iter != NULL;
            iter = g_list_next(iter))
        {
            control_window_modify_colors(iter->data);
        }
    }
}


/**
 * Leave fullscreen
 */
static void control_window_leave_fullscreen(GtkWidget *button, GtkWidget *parent)
{
    gtk_window_unfullscreen(GTK_WINDOW(parent));
}


/**
 * Constructor
 */
GtkWidget *create_control_window(GtkWidget *parent)
{
    gint width, height;
    GtkWidget *pp_button, *next_button, *prev_button, *ff_button;
    GtkWidget *vol, *progress, *hbox, *play_image;
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(parent));
    int new_volume;

    /* Create window */
    GtkWidget *base = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(base), GDK_WINDOW_TYPE_HINT_DOCK);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(base), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(base), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(base), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(base), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(base), GTK_WINDOW(parent));
    gtk_container_set_border_width(GTK_CONTAINER(base), 4);

    /* Overwrite background drawing */
    gtk_widget_set_app_paintable(base, TRUE);
    g_signal_connect(G_OBJECT(base), "expose-event",
        G_CALLBACK(expose_window), NULL);

    hbox = gtk_hbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(base), hbox);

    /* Previous button */
    ff_button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(ff_button),
        gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN,
        GTK_ICON_SIZE_BUTTON));
    gtk_button_set_relief(GTK_BUTTON(ff_button), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), ff_button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(ff_button), "clicked",
        G_CALLBACK(control_window_leave_fullscreen), parent);
    /* Volume button */
    vol = (GtkWidget *)gmpc_widgets_volume_new();
    gtk_box_pack_end(GTK_BOX(hbox), vol, FALSE, FALSE, 0);
    new_volume = mpd_status_get_volume(connection);
    gmpc_widgets_volume_set_volume_level(GMPC_WIDGETS_VOLUME(vol), new_volume );
    g_object_set_data(G_OBJECT(base), "vol", vol);
    g_signal_connect(G_OBJECT(vol), "value_changed",
        G_CALLBACK(playlist_player_volume_changed), NULL);
    /* Progress */
    progress = (GtkWidget *)gmpc_progress_new();
    gmpc_progress_set_hide_text(GMPC_PROGRESS(progress), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), progress, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(base), "progress", progress);
    g_signal_connect(G_OBJECT(progress), "seek-event", G_CALLBACK(pl3_pb_seek_event), NULL);

    /* Previous button */
    prev_button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(prev_button),
        gtk_image_new_from_stock("gtk-media-previous", GTK_ICON_SIZE_BUTTON));
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(prev_button),
        GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPrevious")));
    gtk_button_set_relief(GTK_BUTTON(prev_button), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), prev_button, FALSE, FALSE, 0);

    /* Play button */
    pp_button = gtk_button_new();
    if(mpd_player_get_state(connection) == MPD_PLAYER_PLAY)
    {
        play_image = gtk_image_new_from_stock("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
    }
    else
    {
        play_image = gtk_image_new_from_stock("gtk-media-play", GTK_ICON_SIZE_BUTTON);
    }
    gtk_container_add(GTK_CONTAINER(pp_button), play_image);
    g_object_set_data(G_OBJECT(base), "play_image", play_image);

    gtk_activatable_set_related_action(GTK_ACTIVATABLE(pp_button),
        GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPlayPause")));
    gtk_button_set_relief(GTK_BUTTON(pp_button), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), pp_button, FALSE, FALSE, 0);

    /* Next */
    next_button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(next_button),
        gtk_image_new_from_stock("gtk-media-next", GTK_ICON_SIZE_BUTTON));
    gtk_activatable_set_related_action(GTK_ACTIVATABLE(next_button),
        GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDNext")));
    gtk_button_set_relief(GTK_BUTTON(next_button), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), next_button, FALSE, FALSE, 0);

    /* Move window to right location and set size */
    gtk_window_get_size(GTK_WINDOW(parent), &width, &height);
    gtk_widget_set_size_request(GTK_WIDGET(base), width*0.6, height*0.05);
    gtk_window_move(GTK_WINDOW(base), width*0.2, height);

    if (gdk_screen_is_composited(screen))
    {
        GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);
        if (colormap)
            gtk_widget_set_colormap(base, colormap);
        gtk_window_set_opacity(GTK_WINDOW(base), 0.7);
    }
    /* Change colors */
    control_window_modify_colors(base);
    gtk_widget_show_all(base);

    return base;
}


void control_window_status_update(MpdObj * mi, ChangedStatusType what, GtkWidget *base)
{
    GtkWidget *volume_button, *progress, *play_image;
    if(base == NULL) return;
    volume_button = g_object_get_data(G_OBJECT(base), "vol");
    progress = g_object_get_data(G_OBJECT(base), "progress");
    play_image = g_object_get_data(G_OBJECT(base), "play_image");

    if (what & MPD_CST_STATE)
    {
        int state = mpd_player_get_state(mi);
        switch (state)
        {
            case MPD_PLAYER_PLAY:
                gtk_image_set_from_stock(GTK_IMAGE(play_image),
                    "gtk-media-pause", GTK_ICON_SIZE_BUTTON);
                break;
            case MPD_PLAYER_PAUSE:
                gtk_image_set_from_stock(GTK_IMAGE(play_image),
                    "gtk-media-play", GTK_ICON_SIZE_BUTTON);
                break;
            default:
                gtk_image_set_from_stock(GTK_IMAGE(play_image),
                    "gtk-media-play", GTK_ICON_SIZE_BUTTON);
                /* Make sure it's reset correctly */
                gmpc_progress_set_time(GMPC_PROGRESS(progress), 0, 0);
        }
    }
    if (what & MPD_CST_ELAPSED_TIME)
    {
        if (mpd_check_connected(connection))
        {
            int totalTime = mpd_status_get_total_song_time(connection);
            int elapsedTime = mpd_status_get_elapsed_song_time(connection);
            gmpc_progress_set_time(GMPC_PROGRESS(progress), totalTime, elapsedTime);
        } else
        {
            gmpc_progress_set_time(GMPC_PROGRESS(progress), 0, 0);
        }
    }
    if (what & MPD_CST_VOLUME)
    {
        int volume = gmpc_widgets_volume_get_volume_level(GMPC_WIDGETS_VOLUME(volume_button));
        int new_volume = mpd_status_get_volume(connection);
        if (new_volume >= 0 &&
            mpd_server_check_command_allowed(connection, "setvol") == MPD_SERVER_COMMAND_ALLOWED
            )
        {
            gtk_widget_set_sensitive(volume_button, TRUE);
            /* don't do anything if nothing is changed */
            if (new_volume != volume)
            {
                gmpc_widgets_volume_set_volume_level(GMPC_WIDGETS_VOLUME(volume_button), new_volume );
            }
        } else
        {
            gtk_widget_set_sensitive(volume_button, FALSE);
        }
    }
}


/**
 *  Destructor
 */
void control_window_destroy(GtkWidget *cw)
{
    if(cw == NULL) return;
    gtk_widget_destroy(cw);

}
