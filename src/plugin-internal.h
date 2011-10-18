
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
#include <config.h>
#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include "metadata.h"
#include "gmpc-profiles.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"

#ifndef __PLUGIN_INTERNAL_H__
#define __PLUGIN_INTERNAL_H__

/**
 * Plugin functions
 */

int gmpc_plugin_get_id(gmpcPluginParent * plug);
void gmpc_plugin_init(gmpcPluginParent * plug);
void gmpc_plugin_destroy(gmpcPluginParent * plug);
void gmpc_plugin_save_yourself(gmpcPluginParent * plug);

gboolean gmpc_plugin_get_enabled(const gmpcPluginParent * plug);
void gmpc_plugin_set_enabled(gmpcPluginParent * plug, gboolean enabled);
gboolean gmpc_plugin_has_enabled(gmpcPluginParent * plug);

const gchar *gmpc_plugin_get_translation_domain(gmpcPluginParent * plug);
const char *gmpc_plugin_get_name(gmpcPluginParent * plug);
void gmpc_plugin_status_changed(gmpcPluginParent * plug, MpdObj * mi,
                                ChangedStatusType what);

gint gmpc_plugin_tool_menu_integration(gmpcPluginParent * plug, GtkMenu * menu);
void gmpc_plugin_mpd_connection_changed(gmpcPluginParent * plug, MpdObj * mi,
                                        int connected, gpointer data);

gboolean gmpc_plugin_is_browser(gmpcPluginParent * plug);
gboolean gmpc_plugin_is_sidebar(gmpcPluginParent *plug);
void gmpc_plugin_sidebar_set_state(gmpcPluginParent *plug, GmpcPluginSidebarState state);
void gmpc_plugin_sidebar_init(gmpcPluginParent *plug);
void gmpc_plugin_browser_unselected(gmpcPluginParent * plug,
                                    GtkWidget * container);
void gmpc_plugin_browser_selected(gmpcPluginParent * plug,
                                  GtkWidget * container);
void gmpc_plugin_browser_add(gmpcPluginParent * plug, GtkWidget * cat_tree);
int gmpc_plugin_browser_cat_right_mouse_menu(gmpcPluginParent * plug,
                                             GtkWidget * menu, int type,
                                             GtkWidget * tree,
                                             GdkEventButton * event);
int gmpc_plugin_browser_key_press_event(gmpcPluginParent * plug, GtkWidget * mw,
                                        GdkEventKey * event, int type);
int gmpc_plugin_browser_add_go_menu(gmpcPluginParent * plug, GtkWidget * menu);
int gmpc_plugin_browser_song_list_option_menu(gmpcPluginParent * plug,
                                              GmpcMpdDataTreeview * tree,
                                              GtkMenu * menu);
gboolean gmpc_plugin_browser_has_integrate_search(gmpcPluginParent * plug);
MpdData *gmpc_plugin_browser_integrate_search(gmpcPluginParent * plug,
                                              const int search_field,
                                              const gchar * query,
                                              GError ** error);
gboolean gmpc_plugin_browser_integrate_search_field_supported(gmpcPluginParent *
                                                              plug,
                                                              const int
                                                              search_field);

gboolean gmpc_plugin_has_preferences(gmpcPluginParent * plug);
void gmpc_plugin_preferences_construct(gmpcPluginParent * plug,
                                       GtkWidget * wid);
void gmpc_plugin_preferences_destroy(gmpcPluginParent * plug, GtkWidget * wid);

int gmpc_plugin_get_type(gmpcPluginParent * plug);
const int *gmpc_plugin_get_version(gmpcPluginParent * plug);
gboolean gmpc_plugin_is_internal(gmpcPluginParent * plug);

/* metadata */
gboolean gmpc_plugin_is_metadata(gmpcPluginParent * plug);
int gmpc_plugin_metadata_get_priority(gmpcPluginParent * plug);
void gmpc_plugin_metadata_set_priority(gmpcPluginParent * plug, int priority);

void gmpc_plugin_metadata_query_metadata_list(gmpcPluginParent * plug,
                                              mpd_Song * song,
                                              MetaDataType type,
                                              void (*callback) (GList * uris,
                                                                gpointer data),
                                              gpointer data);

typedef struct _gmpcPluginParent
{
    gmpcPlugin *old;
    GmpcPluginBase *new;
} _gmpcPluginParent;
#endif
