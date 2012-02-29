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

/**
 * This plugin adds a search field to the sidebar
 */

using Config;
using Gtk;
using Gmpc;

private const bool use_transition_ssearch = Gmpc.use_transition;
private const string some_unique_name_ssearch = Config.VERSION;

public class Gmpc.Plugins.SidebarSearch : Gmpc.Plugin.Base, Gmpc.Plugin.SidebarIface {

	private Entry entry = null;
	private const int[] version = {0,0,0};
    public override unowned int[] get_version() {
        return this.version;
    }
    
    public override unowned string get_name() {
        return "Sidebar Search";
    }


    /* We don't want a title */
    public string sidebar_get_title() {
        return "";
    }
    
    /* We want to stick to the bottom  */
    public int sidebar_get_position() {
        return -1;
    }
	public void sidebar_set_state(Gmpc.Plugin.SidebarState state)
	{
		if(entry == null) return;
		if(state == Plugin.SidebarState.COLLAPSED) 
		{
			entry.hide();
		}else{
			entry.show();
		}
	}
    
	const string searchText="Search";
	public void sidebar_pane_construct(Gtk.VBox parent) {
		entry = new Entry();
		entry.set_text(searchText);
        
        entry.set_icon_from_stock(EntryIconPosition.PRIMARY, "gtk-find");
        entry.set_icon_from_stock(EntryIconPosition.SECONDARY, "gtk-clear");
        entry.set_icon_activatable(EntryIconPosition.PRIMARY, true);
        entry.set_icon_activatable(EntryIconPosition.SECONDARY, true);
        

        entry.focus_in_event.connect( () => {
            if (entry.get_text() == searchText) {
                entry.set_text("");
            }

            return false;
        });
        
        entry.focus_out_event.connect( () => {
            if (entry.get_text() == "") {
                entry.set_text(searchText);
            }
            
            return false;
        });
        
        entry.icon_press.connect( (icon) => {
            entry.grab_focus();
            switch (icon) {
                case EntryIconPosition.PRIMARY:
                    break;
                    
                case EntryIconPosition.SECONDARY:
                    entry.set_text("");
                    break;
            }
        });
        
        entry.activate.connect( (source) => {
				Gmpc.Browser.Find.query_database(null, source.get_text()); 
				entry.set_text(searchText);
        });
        
        Alignment align = new Alignment(1, 1, 1, 1);
        align.set_padding(0,0,2,2);
        align.add(entry);
        parent.pack_start(align, false, false, 0);
        
        parent.show_all();
		this.sidebar_set_state(Gmpc.Playlist.get_sidebar_state());
    }
    
    public void sidebar_pane_destroy(Gtk.VBox parent) {
       foreach(Gtk.Widget child in parent.get_children())
       {
           parent.remove(child);
       }
		entry = null;
    }
}
