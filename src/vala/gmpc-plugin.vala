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

using GLib;
using Gtk;
using Gmpc;
using MPD;


namespace Gmpc {
    namespace Plugin {
        /**
         * This is the base class that a plugin should inherit from.
         *
         */
        public abstract class Base : GLib.Object { 
            /* This string tell gmpc what translation domain to use when trying to translate the plugins name. 
             * If NULL then gmpc's translation domain is used.
             */
            public unowned string translation_domain = null;
            /* This is set by gmpc, it contains the full path of the plugin.
             * This is used by gmpc_plugin_get_data_path to calculate the location of the data files.
             */
            public string path;
            /**
             * This id is set by gmpc. It is a unique id identifying the plugin.
             */
            public int id;
            /**
             * The type of the plugin. see #PluginType. 
             * This is inherited from the old style plugins and no longer used.
             * It will be used to mark a plugin internal
             */
            public int plugin_type = 1;
            /**
             * Function should return the version of the plugin
             */
            public abstract unowned int[] get_version();
            /**
             * Return the name of the plugin
             */
            public abstract unowned string get_name ();
            /**
             * This is called before the plugin is destroyed. Plugins should save it state here.
             *
             * A Browser plugin should store the position in the side-tree here.
             * Optional function. 
             */
            public virtual void save_yourself () 
            {
            }
            /**
             * Function used by gmpc to check if the plugin is enabled.
             * By default it is stored in the get_name() category under the enabled key.
             * 
             * @return The state (true or false)
             */
            public virtual bool get_enabled ()
            {
                if(this.get_name() == null) return false;
                return (bool)Gmpc.config.get_int_with_default(this.get_name(), "enabled", 1);
            }

            /**
             * Function to enable/disable the plugin
             * @param state the enable state to set the plugin in. (true or false)
             * 
             * Function used by gmpc to enable/disable the plugin. 
             * By default it is stored in the get_name() category under the enabled key.
             * If something needs to be done on enable/disable override this function.
             */
            public virtual void set_enabled (bool state)
            {
                if(this.get_name() != null)
                    Gmpc.config.set_int(this.get_name(), "enabled", (int)state); 
            }

        }
        /**
         * This interface allows the plugin to add one, or more, entries in the Tools menu.
         * If need to remove or undate an entry call pl3_tool_menu_update(). This will tell gmpc
         * To clear the menu, and call this function again on every plugin.
         */
        public interface ToolMenuIface : Base {
            public abstract int tool_menu_integration(Gtk.Menu menu);
        }
        public delegate void MetaDataCallback(owned GLib.List<Gmpc.MetaData.Item>? list);
        /* untested */
        public interface MetaDataIface : Base {
            public abstract void get_metadata (MPD.Song song, Gmpc.MetaData.Type type, MetaDataCallback callback);
            /* Set get priority */
            public abstract int get_priority ();
            public abstract void set_priority (int priority);
        }
        public interface BrowserIface : Base {
            /* Function is called by gmpc, the plugin should then insert itself in the left tree  */
            public abstract  void browser_add (Gtk.Widget category_tree);
            /* This gets called, the plugin should add it view in container */
            public abstract void  browser_selected (Gtk.Container container);
            /* Plugin should remove itself from container */
            public abstract void  browser_unselected (Gtk.Container container);
            /* Option menu */
            public virtual int browser_option_menu(Gtk.Menu menu)
            {
                return 0;
            }
            /* Go menu */
            public virtual int browser_add_go_menu(Gtk.Menu menu)
            {
                return 0;
            }

        }
        public interface IntegrateSearchIface : Base {
            public virtual bool field_supported (MPD.Tag.Type tag)
            {
                return true;
            }
            public abstract MPD.Data.Item ? search(MPD.Tag.Type tag, string search_query);
        }
        public interface PreferencesIface : Base {
            public abstract void preferences_pane_construct (Gtk.Container container);
            public abstract void preferences_pane_destroy (Gtk.Container container);

        }

        /* untested */
        public interface SongListIface : Base {
            public abstract int song_list (Gtk.Widget tree, Gtk.Menu menu);

        }
    }
}
