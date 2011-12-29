/* gmpc-extraplaylist (GMPC plugin)
 * Copyright (C) 2006-2009 Qball Cow <qball@gmpclient.org>
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

#include <string.h>
#include "plugin.h"
#include "plugin-internal.h"
#include "playlist3.h"
#include "gmpc-extras.h"
#include "config.h"
#include "browsers/playlist3-current-playlist-browser.h"
/* External pointer + function, there internal from gmpc */
GtkWidget *extraplaylist = NULL;
static GtkWidget *extraplaylist_paned = NULL;
static GmpcPluginBase *play_queue_plugin = NULL;
static gboolean includes_sidebar = FALSE;

static GtkWidget *cb_include_sidebar = NULL;

static void extra_playlist_add(void);

static void extra_playlist_save(void) {
	if(extraplaylist) {
		int pos = gtk_paned_get_position(GTK_PANED(extraplaylist_paned));
		if(pos>0)
			cfg_set_single_value_as_int(config, "extraplaylist", "paned-pos", pos);
	}
}

static int get_enabled(void) {
	return cfg_get_single_value_as_int_with_default(config,"extraplaylist", "enabled", 0); 
}

static void ep_view_changed(GtkTreeSelection *selection, gpointer user_data)
{
    GtkTreeModel *model= NULL;
    GtkTreeIter iter;
    if(gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        int type = 0;
        _gmpcPluginParent *parent = NULL;
        gtk_tree_model_get(model, &iter, PL3_CAT_TYPE, &type, -1);
        parent = (_gmpcPluginParent *)plugin_get_from_id(type);
        if(parent)
        {
            if(parent->new){
                if(G_OBJECT_TYPE(parent->new) == G_OBJECT_TYPE(play_queue_plugin))
                {
                    if(extraplaylist){
                        gtk_widget_hide(extraplaylist);
                        if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
                            gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),GTK_CONTAINER(extraplaylist)); 
                    }
                    return;
                }
            }
            if(get_enabled())
            {
                if(!extraplaylist) {
                    extra_playlist_add();
                }else{
                    if(gtk_bin_get_child(GTK_BIN(extraplaylist)) == NULL){
                        gmpc_plugin_browser_iface_browser_selected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin), GTK_CONTAINER(extraplaylist));
                        gtk_widget_show(extraplaylist);
                    }
                }

            }else if(extraplaylist){
                gtk_widget_hide(extraplaylist);
                if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
                    gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),GTK_CONTAINER(extraplaylist)); 
            }
        }
    }
}

static void extra_playlist_remove(void) {
    GtkWidget *temp = NULL;
    if(pl3_xml == NULL) return;
    if(extraplaylist == NULL  && get_enabled() == FALSE) return;
    if(extraplaylist == NULL) return;

    if (includes_sidebar) {
        temp = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hpaned1-hbox"));
        g_object_ref(temp);

        extra_playlist_save();
        /* Remove widget */
        gtk_widget_hide(extraplaylist);
        if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
                    gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),GTK_CONTAINER(extraplaylist));

        /* Remove it from inserted widget */
        gtk_container_remove(GTK_CONTAINER(extraplaylist_paned), temp);

        /* Destroy the previously added stuff split view */
        gtk_widget_destroy(extraplaylist);
        extraplaylist = NULL;
        gtk_widget_destroy(extraplaylist_paned);
        extraplaylist = NULL;
        gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(pl3_xml,"hpaned1_vbox")), temp);
    }
    else {
        temp = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox7"));
        g_object_ref(temp);

        extra_playlist_save();
        /* Remove widget */
        gtk_widget_hide(extraplaylist);
        if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
                    gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),GTK_CONTAINER(extraplaylist));

        /* Remove it from inserted widget */
        gtk_container_remove(GTK_CONTAINER(extraplaylist_paned), temp);

        /* Destroy the previously added stuff split view */
        gtk_widget_destroy(extraplaylist);
        extraplaylist = NULL;
        gtk_widget_destroy(extraplaylist_paned);
        extraplaylist = NULL;
        gtk_box_pack_end(GTK_BOX(gtk_builder_get_object(pl3_xml,"hpaned1-hbox")),temp, TRUE, TRUE,0);
    }

}


static void extra_playlist_add(void) {

    extra_playlist_remove();

    GtkWidget *temp = NULL;
	if(pl3_xml == NULL) return;
    if(extraplaylist == NULL  && get_enabled() == FALSE) return;
    if(extraplaylist != NULL) return;

    if ((cfg_get_single_value_as_int_with_default(config, "extraplaylist", "include-sidebar",FALSE)) &&
       (cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout", TRUE))) {
        includes_sidebar = TRUE;
        temp = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hpaned1-hbox"));
    }
    else {
        includes_sidebar = FALSE;
        temp = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox7"));
    }
    g_object_ref(temp);

	/**
	 * Hack it into the main view
	 */
    extraplaylist = gtk_event_box_new();
    /* Set border to fits gmpc's standard  */
    gtk_container_set_border_width(GTK_CONTAINER(extraplaylist), 0);

    if(cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout", TRUE))
    {
        extraplaylist_paned = gtk_vpaned_new();
    }else{
        extraplaylist_paned = gtk_hpaned_new();
    }

    if (includes_sidebar) {
        gtk_container_remove(GTK_CONTAINER(gtk_builder_get_object(pl3_xml,"hpaned1_vbox")),temp);
    }
    else {
        gtk_container_remove(GTK_CONTAINER(gtk_builder_get_object(pl3_xml,"hpaned1-hbox")),temp);
    }

    if(!cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout-swapped",FALSE))
    {
        gtk_paned_pack1(GTK_PANED(extraplaylist_paned), temp, TRUE, TRUE); 
        gtk_paned_pack2(GTK_PANED(extraplaylist_paned), extraplaylist, TRUE, TRUE); 
    }else{
        gtk_paned_pack2(GTK_PANED(extraplaylist_paned), temp, TRUE, TRUE); 
        gtk_paned_pack1(GTK_PANED(extraplaylist_paned), extraplaylist, TRUE, TRUE); 
    }

    if (includes_sidebar) {
        gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(pl3_xml,"hpaned1_vbox")),extraplaylist_paned);
    }
    else {
        gtk_box_pack_end(GTK_BOX(gtk_builder_get_object(pl3_xml, "hpaned1-hbox")), extraplaylist_paned,TRUE, TRUE,0);
    }


	gtk_paned_set_position(GTK_PANED(extraplaylist_paned),
			cfg_get_single_value_as_int_with_default(config, "extraplaylist", "paned-pos", 400));

	gtk_widget_show(extraplaylist_paned);
    gtk_widget_hide(extraplaylist);

    if(play_queue_plugin == NULL) {
        play_queue_plugin = (GmpcPluginBase *)play_queue_plugin_new("extra-playlist-plugin");
    }
    ep_view_changed(gtk_tree_view_get_selection(playlist3_get_category_tree_view()),NULL);

    /* Attach changed signal */
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(playlist3_get_category_tree_view())), "changed",
        G_CALLBACK(ep_view_changed), NULL);

}


static void extra_playlist_init(void ) {
    if( cfg_get_single_value_as_int_with_default(config,"extraplaylist", "enabled", 0)) {
		gtk_init_add((GtkFunction )extra_playlist_add, NULL);
	}
}
static void set_enabled(int enable) {
    cfg_set_single_value_as_int(config,"extraplaylist", "enabled", enable);
    if(enable)
	{
		if(!extraplaylist) {
			extra_playlist_add();
		}else{
            ep_view_changed(gtk_tree_view_get_selection(playlist3_get_category_tree_view()),NULL);
        }

    } else if(extraplaylist){
		gtk_widget_hide(extraplaylist);
        if(gtk_bin_get_child(GTK_BIN(extraplaylist)))
            gmpc_plugin_browser_iface_browser_unselected(GMPC_PLUGIN_BROWSER_IFACE(play_queue_plugin),GTK_CONTAINER(extraplaylist)); 
	}
}

/**
 * Preferences 
 */
static void preferences_layout_changed(GtkToggleButton *but, gpointer user_data)
{
    gint active = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "extraplaylist", "vertical-layout", active);
    gtk_widget_set_sensitive(GTK_WIDGET(cb_include_sidebar), cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout", TRUE));
    extra_playlist_add();
}

static void preferences_layout_swapped_changed(GtkToggleButton *but, gpointer user_data)
{
    gint active = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "extraplaylist", "vertical-layout-swapped", active);
    extra_playlist_add();
}

static void preferences_layout_sidebar_changed(GtkToggleButton *but, gpointer user_data)
{
    gint active = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "extraplaylist", "include-sidebar", active);
    extra_playlist_add();
}

static  void preferences_construct(GtkWidget *container)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 3);
    GtkWidget *label = NULL;

    GtkWidget *frame = gtk_frame_new("");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))), "<b>Appearance</b>");

    GtkWidget *align = gtk_alignment_new(0,0,0,0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(align), 12, 0, 12, 0);

    /* The checkbox */
    label = gtk_check_button_new_with_label("Use horizontal layout");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout", TRUE));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(label), "toggled", G_CALLBACK(preferences_layout_changed), NULL);

    /* The checkbox */
    cb_include_sidebar = gtk_check_button_new_with_label("Use full width");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb_include_sidebar), cfg_get_single_value_as_int_with_default(config, "extraplaylist", "include-sidebar", FALSE));
    gtk_widget_set_sensitive(GTK_WIDGET(cb_include_sidebar), cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout", TRUE));

    GtkWidget *align2 = gtk_alignment_new(0,0,0,0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(align2), 0, 0, 20, 0);
    gtk_container_add(GTK_CONTAINER(align2), cb_include_sidebar);
    gtk_box_pack_start(GTK_BOX(vbox), align2, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(cb_include_sidebar), "toggled", G_CALLBACK(preferences_layout_sidebar_changed), NULL);

    label = gtk_check_button_new_with_label("Swap position of the extra playlist");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), cfg_get_single_value_as_int_with_default(config, "extraplaylist", "vertical-layout-swapped", FALSE));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(label), "toggled", G_CALLBACK(preferences_layout_swapped_changed), NULL);
   

    /* show and add */
    gtk_container_add(GTK_CONTAINER(align), vbox);
    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(align));
    gtk_widget_show_all(frame);
    gtk_container_add(GTK_CONTAINER(container), frame);

}
static void preferences_destroy(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), gtk_bin_get_child(GTK_BIN(container)));

}
 gmpcPrefPlugin extra_playlist_preferences = {
    .construct = preferences_construct,
    .destroy = preferences_destroy

 };

gmpcPlugin extraplaylist_plugin = {
    .name = "Extra playlist",
	.version = {0, 0, 1},
	.plugin_type = GMPC_PLUGIN_NO_GUI | GMPC_INTERNALL,
	.init = extra_playlist_init,            /* initialization */
	.save_yourself = extra_playlist_save,         /* Destroy */
	.get_enabled = get_enabled,
	.set_enabled = set_enabled,

    .pref = &extra_playlist_preferences,
};
