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

using GLib;
using Gtk;


namespace Gmpc {
    public abstract class PluginBase :GLib.Object { 
        public int id;
        public int plugin_type = 1;
        public abstract weak string get_name ();
    //    public abstract weak int[3] get_version ();
        
        public abstract void save_yourself ();

        public abstract bool get_enabled ();
        public abstract void set_enabled (bool state);
    
    }
    namespace Plugin2 {
    public interface MetaData : PluginBase {
       public abstract int get_data ();
       /* Set get priority */
       public abstract int get_priority ();
       public abstract void set_priority (int priority);
    }
    public interface Browser : PluginBase {
        /* Function is called by gmpc, the plugin should then insert itself in the left tree  */
        public abstract  void add (Gtk.Widget *category_tree);
        /* This gets called, the plugin should add it view in container */
        public abstract void  selected (Widget *container);
        /* Plugin should remove itself from container */
        public abstract void  unselected (Widget *container);

    }
    public interface Preferences : PluginBase {
        public abstract Gtk.Widget pref_construct ();
        public abstract Gtk.Widget pref_destroy ();

    }
    public interface SongList : PluginBase {
        public abstract int song_list (Gtk.Widget *tree, Gtk.Menu *menu);

    }
    }
}
