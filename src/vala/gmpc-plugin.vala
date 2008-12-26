/* Gnome Music Player (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
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

using GLib;
using Gtk;


namespace Gmpc {
    public interface Plugin2 {
        public abstract string get_name ();
        public abstract int[3] get_version ();
      
        /*  */
        public abstract void save_yourself ();

        /* Get/set enabled */
        public abstract bool get_enabled ();
        public abstract void set_enabled (bool state);
    }

    public abstract class PluginBase : Plugin2 { 
        public abstract string get_name ();
        public abstract int[3] get_version ();
        
        public abstract void save_yourself ();

        public abstract bool get_enabled ();
        public abstract void set_enabled (bool state);
    }
    public interface MetaData : Plugin2 {
       public abstract int get_data ();
       /* Set get priority */
       public abstract int get_priority ();
       public abstract void set_priority (int priority);
    }
    public interface Browser : Plugin2 {
        /* Function is called by gmpc, the plugin should then insert itself in the left tree  */
        public abstract  void add (Gtk.Widget *category_tree);
        /* This gets called, the plugin should add it view in container */
        public abstract void  selected (Widget *container);
        /* Plugin should remove itself from container */
        public abstract void  unselected (Widget *container);

    }
    public interface Preferences : Plugin2 {
        public abstract Gtk.Widget pref_construct ();
        public abstract Gtk.Widget pref_destroy ();

    }
    public interface SongList : Plugin2 {
        public abstract int song_list (Gtk.Widget *tree, Gtk.Menu *menu);

    }

}
