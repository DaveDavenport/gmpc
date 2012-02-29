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

using Config;
using Gtk;
using Gmpc;
using Gmpc.Plugin;

const string GSBP_LOG_DOMAIN  = "Gmpc.Sidebar.Plugins";

public class Gmpc.Sidebar.Plugins {
    private static ListStore store;

    private static void embed(TreeIter iter) {
        SidebarIface plugin;
        store.get(iter, 3, out plugin, -1);
        
        VBox sidebar_vbox;
        VBox vbox;
        
        Label label;
        Alignment alignment;
        string title;
        int position;

        vbox = new VBox(false, 0);

        title = plugin.sidebar_get_title();

        if (title == null)
            title = plugin.get_name();

        position = plugin.sidebar_get_position();
        
        if (title != "") {
            alignment = new Alignment(0, 0, 0, 0);
            alignment.set_padding(0, 6, 4, 0);

            label = new Label(GLib.Markup.printf_escaped("<b>%s</b>",title));
            label.set_use_markup(true);
            
            alignment.add(label);
            vbox.pack_start(alignment, false, false, 0);
        }
        
        if ( position >= 0) {
            sidebar_vbox = (VBox)Playlist.get_widget_by_id("sidebar_plugins_top");
        }
        else
        {
            sidebar_vbox = (VBox)Playlist.get_widget_by_id("sidebar_plugins_bottom");
            
        }
        sidebar_vbox.ref();

        if (position >= 0) {
            sidebar_vbox.pack_start(vbox, false, false, 0);
        }
        else {
            sidebar_vbox.pack_end(vbox, false, false, 0);
        }
        
        
        
        
        store.set(iter, 4, vbox, -1);
        reorder();
        plugin.sidebar_pane_construct(vbox);

    }

    private static void reorder() {
        // assumption: liststore is sorted descending by position
        TreeIter iter;
        VBox vbox;
        VBox sidebar_vbox;
        bool enabled;
        int pos;
        List<VBox> list_top = new List<VBox> ();
        List<VBox> list_bottom = new List<VBox> ();

        store.get_iter_first(out iter);

        do {
            store.get(iter, 0, out enabled, 2, out pos, 4, out vbox, -1);
            if ((vbox == null) || (!enabled))
                continue;
            
            if (pos >= 0) {
                list_top.append(vbox);
            } else {
                list_bottom.append(vbox);
            }
        
        } while (store.iter_next(ref iter));

        sidebar_vbox = (VBox)Playlist.get_widget_by_id("sidebar_plugins_top");
        sidebar_vbox.ref();
        list_top.reverse();
        for (int i = 0; i < list_top.length(); i++) {
        
            sidebar_vbox.reorder_child((Widget)list_top.nth_data(i), i);
        }
        
        sidebar_vbox = (VBox)Playlist.get_widget_by_id("sidebar_plugins_bottom");
        sidebar_vbox.ref();
        for (int i = 0; i < list_bottom.length(); i++) {
        
            sidebar_vbox.reorder_child((Widget)list_bottom.nth_data(i), i);
        }
    }

    private static void destroy(TreeIter iter) {
        SidebarIface plugin;
        VBox vbox;
        store.get(iter, 3, out plugin, 4, out vbox, -1);
        
        plugin.sidebar_pane_destroy(vbox);
        vbox.destroy();
        vbox = (VBox)null;
    }
    
	private static TreeIter lookup_iter(SidebarIface plugin) {
		TreeIter iter;
		if(store.get_iter_first(out iter))
		{
			SidebarIface pl;
			do{
			store.get(iter, 3, out pl, -1);
			if(pl == plugin) {
				return iter;
			}
			}while(store.iter_next(ref iter));
		}
		GLib.error("Plugin not found in list");
	}
    
    public static void enable(SidebarIface plugin) {
        TreeIter iter = lookup_iter(plugin);
        plugin.set_enabled(true);
        store.set(iter, 0, true, -1);
        embed(iter);
    }

    public static void disable(SidebarIface plugin) {
        TreeIter iter = lookup_iter(plugin);
        plugin.set_enabled(false);
        store.set(iter, 0, false, -1);
        destroy(iter);
    }
	public static void update_state(Gmpc.Plugin.SidebarState state)
	{
		TreeIter iter;
		if(store.get_iter_first(out iter))
		{
			do{
				SidebarIface pl;
				store.get(iter, 3, out pl, -1);
				pl.sidebar_set_state(state);
			}while(store.iter_next(ref iter));
		}
	}


    public static void init(SidebarIface plugin) {
        if (store == null)
            store = new ListStore(5, typeof(bool),          // enabled
                                     typeof(string),        // name
                                     typeof(int),           // position
                                     typeof(SidebarIface),  // plugin
                                     typeof(VBox));         // Widget
		log(GSBP_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "Initializing sidebar plugin");
        TreeIter iter;
        store.append(out iter);
        store.set(iter, 0, plugin.get_enabled(),
                        1, plugin.get_name(),
                        2, plugin.sidebar_get_position(),
                        3, plugin,
                        4, null,
                        -1);
                        
        
        store.set_sort_column_id(2, SortType.DESCENDING);
        
        if (plugin.get_enabled()) {
            enable(plugin);
        }
    }
}
