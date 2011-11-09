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
#include "gmpc-extras.h"
#include "main.h"
#include "playlist3.h"
#include "plugin.h"
#include "status_icon.h"

/**
 * Status icons
 */
static GtkWidget *si_repeat = NULL;
static GtkWidget *si_consume = NULL;
static GtkWidget *si_repeat_single = NULL;
static GtkWidget *si_random = NULL;

void main_window_update_status_icons(void)
{
    if (si_repeat_single)
    {
        GtkWidget *image = gtk_bin_get_child(GTK_BIN(si_repeat_single));
        if (mpd_check_connected(connection) && mpd_player_get_single(connection))
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), TRUE);
            gtk_widget_set_tooltip_text(si_repeat_single, _("Single Mode enabled"));
        } else
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), FALSE);
            gtk_widget_set_tooltip_text(si_repeat_single, _("Single Mode disabled"));
        }
    }
    if (si_consume)
    {
        GtkWidget *image = gtk_bin_get_child(GTK_BIN(si_consume));
        if (mpd_check_connected(connection) && mpd_player_get_consume(connection))
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), TRUE);
            gtk_widget_set_tooltip_text(si_consume, _("Consume Mode enabled"));
        } else
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), FALSE);
            gtk_widget_set_tooltip_text(si_consume, _("Consume Mode disabled"));
        }
    }
    if (si_repeat)
    {
        GtkWidget *image = gtk_bin_get_child(GTK_BIN(si_repeat));
        if (mpd_check_connected(connection) && mpd_player_get_repeat(connection))
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), TRUE);
            gtk_widget_set_tooltip_text(si_repeat, _("Repeat enabled"));
        } else
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), FALSE);
            gtk_widget_set_tooltip_text(si_repeat, _("Repeat disabled"));
        }
    }
    if (si_random)
    {
        GtkWidget *image = gtk_bin_get_child(GTK_BIN(si_random));
        if (mpd_check_connected(connection) && mpd_player_get_random(connection))
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), TRUE);
            gtk_widget_set_tooltip_text(si_random, _("Random enabled"));
        } else
        {
            gtk_widget_set_sensitive(GTK_WIDGET(image), FALSE);
            gtk_widget_set_tooltip_text(si_random, _("Random disabled"));
        }
    }
}


void main_window_add_status_icon(GtkWidget * icon)
{
    GtkWidget *hbox = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "status_icon_box"));
    g_return_if_fail(icon != NULL);
//    gtk_box_pack_end(GTK_BOX(hbox), icon, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(hbox), icon);
	gtk_widget_show(icon);
	gtk_widget_show(hbox);
}

void main_window_init_default_status_icons(void)
{
	GmpcToolsBindingOverlayNotify *p = gtk_builder_get_object(pl3_xml, "binding_overlay_notify");
    si_repeat = gmpc_widgets_overlay_new();

	g_object_set(G_OBJECT(si_repeat),
				"modifier", GDK_MOD1_MASK,
				"overlay-text", "r",
				"binding-overlay-notifier", p,
				NULL);

    g_signal_connect(G_OBJECT(si_repeat), "button-release-event", G_CALLBACK(repeat_toggle), NULL);
    gtk_container_add(GTK_CONTAINER(si_repeat), gtk_image_new_from_icon_name("stock_repeat", GTK_ICON_SIZE_MENU));
    gtk_widget_show_all(si_repeat);
    main_window_add_status_icon(si_repeat);

    si_random = gmpc_widgets_overlay_new();

	g_object_set(G_OBJECT(si_random),
				"modifier", GDK_MOD1_MASK,
				"overlay-text", "f",
				"binding-overlay-notifier", p,
				NULL);

    g_signal_connect(G_OBJECT(si_random), "button-release-event", G_CALLBACK(random_toggle), NULL);
    gtk_container_add(GTK_CONTAINER(si_random), gtk_image_new_from_icon_name("stock_shuffle", GTK_ICON_SIZE_MENU));
    gtk_widget_show_all(si_random);
    main_window_add_status_icon(si_random);

    si_repeat_single = gmpc_widgets_overlay_new();

	g_object_set(G_OBJECT(si_repeat_single),
				"modifier", GDK_MOD1_MASK,
				"overlay-text", "t",
				"binding-overlay-notifier", p,
				NULL);

    g_signal_connect(G_OBJECT(si_repeat_single), "button-release-event", G_CALLBACK(repeat_single_toggle), NULL);
    gtk_container_add(GTK_CONTAINER(si_repeat_single),
        gtk_image_new_from_icon_name("media-repeat-single", GTK_ICON_SIZE_MENU));
    gtk_widget_show_all(si_repeat_single);
    main_window_add_status_icon(si_repeat_single);

    si_consume =  gmpc_widgets_overlay_new();
	g_object_set(G_OBJECT(si_consume),
				"modifier", GDK_MOD1_MASK,
				"overlay-text", "y",
				"binding-overlay-notifier", p,
				NULL);
    g_signal_connect(G_OBJECT(si_consume), "button-release-event", G_CALLBACK(consume_toggle), NULL);
    gtk_container_add(GTK_CONTAINER(si_consume), gtk_image_new_from_icon_name("media-consume", GTK_ICON_SIZE_MENU));
    gtk_widget_show_all(si_consume);
    main_window_add_status_icon(si_consume);
}
