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
using Gmpc;


namespace Gmpc {
    namespace Plugin {
        public abstract class Base : GLib.Object { 
            /* set by gmpc. */
            public int id;
            /* Default to dummy */
            public int plugin_type = 1;
            /* The version */
            public abstract weak int[3] get_version();// = {0,0,1};

            public abstract weak string get_name ();

            public virtual void save_yourself () 
            {
                stdout.printf("Default save yourself function\n");
            }

            public virtual bool get_enabled ()
            {
                if(this.get_name() == null) return false;
                return (bool)Gmpc.config.get_int_with_default(this.get_name(), "enabled", 1);
            }
            public virtual void set_enabled (bool state)
            {
                if(this.get_name() != null)
                    Gmpc.config.set_int(this.get_name(), "enabled", (int)state); 
            }

        }
        public interface ToolMenuIface : Base {
            public abstract int tool_menu_integration(Gtk.Menu menu);
        }
        public interface MetaDataIface : Base {
            public abstract int get_data ();
            /* Set get priority */
            public abstract int get_priority ();
            public abstract void set_priority (int priority);
        }
        public interface BrowserIface : Base {
            /* Function is called by gmpc, the plugin should then insert itself in the left tree  */
            public abstract  void browser_add (Gtk.Widget *category_tree);
            /* This gets called, the plugin should add it view in container */
            public abstract void  browser_selected (Widget *container);
            /* Plugin should remove itself from container */
            public abstract void  browser_unselected (Widget *container);

        }
        public interface PreferencesIface : Base {
            public abstract void pane_construct (Gtk.Container container);
            public abstract void pane_destroy (Gtk.Container container);

        }
        public interface SongListIface : Base {
            public abstract int song_list (Gtk.Widget *tree, Gtk.Menu *menu);

        }
    }
}
