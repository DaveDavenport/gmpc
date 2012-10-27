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
 * Usage:
 * Create text file: ~/.config/gmpc/external-command.cfg
 * Add sections for each command, Each section has a cmd and a name entry.
 * The cmd should be a path to the executable with possible arguments,
 * the cmd string is parsed by gmpc.
 *
 * Example:
 * [1]
 * cmd="/home/me/test.sh -f test3"
 * name="Test script"
 *
 * The paths  (relative to MPD directory) are passed to the script as arguments.
 */
using GLib;
using Gtk;
using Gmpc;
using Config;

private const bool use_transition_externalcommand = Gmpc.use_transition;
private const string some_unique_name_externalcommand = Config.VERSION;
private const string log_domain_externalcommand = "Gmpc.Plugins.ExternalCommand";

namespace Gmpc
{
    public class ExternalCommand :
        Gmpc.Plugin.Base,
        Gmpc.Plugin.SongListIface,
        Gmpc.Plugin.PreferencesIface
    {
        private const int[]     version           = {0,0,0};
        private Gmpc.Settings   cfg_ec            = null;

        public override unowned int[] get_version() {
            return this.version;
        }

        public override unowned string get_name () {
            return "Call external command";
        }

        construct {
            string settings_path = Gmpc.user_path("external-command.cfg");
            cfg_ec = new Gmpc.Settings.from_file(settings_path);
            this.plugin_type = 8;
        }

        ~ExternalCommand() {
            cfg_ec = null;
        }

        private void start_command(Gtk.MenuItem item) {
            // Get the command.
            string cmd = item.get_data("cmd");
            // Calculate number of arguments.
            int args  = 0;
            int index= 0;
            Gtk.TreeView tree = item.get_data("tree");
            Gtk.TreeSelection sel  = tree.get_selection();
            args += sel.count_selected_rows();

            // Parse the command list.
            string[] argv;
            try {
                Shell.parse_argv(cmd, out argv);
            } catch (Error e) {
                Gmpc.Messages.show("%s: %s".printf(_("Failed to parse command"), e.message), Gmpc.Messages.Level.WARNING);
                return;
            }

            // Resize the list to fit the songs passed.
            index = argv.length;
            args += index;
            //  +1 (closing NULL)
            argv.resize((args+1));


            // Path.
            weak string? music_directory = null;
            var  cp = Gmpc.profiles.get_current();
            if( cp != null )
            {
                if(Gmpc.profiles.get_music_directory(cp) != null)
                {
                    music_directory = Gmpc.profiles.get_music_directory(cp);
                }
            }

            // Set song paths as arguments.
            Gtk.TreeModel model;
            var rows = sel.get_selected_rows(out model);
            foreach ( Gtk.TreePath path in rows ){
                string file;
                Gtk.TreeIter iter;
                if(model.get_iter(out iter, path)){
                    model.get(iter,3, out file);
                    if(music_directory != null) {
                        argv[index] = Path.build_filename(music_directory,file);
                    }else{
                        argv[index] = file;
                    }
                    index++;
                }
            }
            // Closing null
            argv[index] = null;

            // Execute.
            try {
                GLib.Process.spawn_async(null, argv, null, SpawnFlags.SEARCH_PATH, null, null);
            } catch (Error e) {
                Gmpc.Messages.show("%s: %s".printf(_("Failed to start external command"), e.message), Gmpc.Messages.Level.WARNING);
                return;
            }
        }

        public int song_list (Gtk.Widget tree, Gtk.Menu menu)
        {
            int retv = 0;

            SettingsList sl = cfg_ec.get_class_list();
            weak SettingsList iter = sl;
            while(iter != null) {
                string cmd = cfg_ec.get_string(iter.key, "cmd");
                string name = cfg_ec.get_string(iter.key, "name");

                // If both values are not valid, continue.
                if(cmd == null || name == null) continue;
                var item = new Gtk.ImageMenuItem.with_label("Run external command: "+name);

                // Attach tree widget to item and the command.
                item.set_data("tree", tree);
                item.set_data("cmd", cmd);
                // Click Signal.
                item.activate.connect(start_command);
                // Add to menu
                menu.append(item);
                retv++;
                // Next item.
                iter = iter.next;
            }
            return retv;
        }


        /************************
         * Preferences
         ************************/
        private Gtk.Builder pref_builder = null;

        /* Destroy preferences construct */
        public void preferences_pane_construct(Gtk.Container container)
        {
            if(pref_builder != null) return;
            pref_builder = new Gtk.Builder();
            try
            {
                pref_builder.add_from_file(Gmpc.data_path("preferences-external-command.ui"));
            }
            catch (GLib.Error e)
            {
                GLib.error("Failed to load GtkBuilder file: %s", e.message);
            }

            var b = pref_builder.get_object("main_box") as Gtk.Widget;
            container.add(b);
        }
        /* Destroy preferences pane */
        public void preferences_pane_destroy(Gtk.Container container)
        {
            container.remove((container as Gtk.Bin).get_child());
            pref_builder = null;
        }
    }
}
