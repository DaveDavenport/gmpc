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

/**
 * This plugin queries HTBackdrops.com for artist images (backdrops)
 */

using Config;
using Gmpc;
using Gmpc.Plugin;

private const bool use_transition_autompd = Gmpc.use_transition;
private const string some_unique_name_autompd = Config.VERSION;
private const string log_domain_autompd = "Gmpc.Plugins.AutoMPD";

public class Gmpc.Plugins.AutoMPD: 
            Gmpc.Plugin.Base,
            Gmpc.Plugin.PreferencesIface
{
    private string mpd_path = null;
    /**
     * Gmpc.Plugin.Base
     */
    private const int[] version = {0,0,2};

    /** Return the plugin version. For an internal plugin this is not that interresting.
     * But we implement it anyway 
     */
    public override unowned int[] get_version()
    {
        return this.version;
    }

    /**
     * The name of the plugin 
     */
    public override unowned string get_name()
    {
        return N_("Automatic MPD");
    }

    private void start_mpd()
    {

    }
    private void stop_mpd()
    {

    }

    private void find_mpd_binary()
    {
        var path = GLib.Environment.get_variable("PATH");
        foreach(var str in path.split(":"))
        {
            var total_path = GLib.Path.build_filename(str, "mpd"); 
            if ( GLib.FileUtils.test(total_path,GLib.FileTest.IS_EXECUTABLE))
            {
                mpd_path = total_path; 
                GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG, 
                        "Found MPD Path: %s", mpd_path);
                return;
            }
        }
        GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG, 
                "No MPD found in path.");
    }
    construct {
        find_mpd_binary();
    }
    /**
     * Preferences pane
     */
     public void preferences_pane_construct(Gtk.Container container)
     {

     }
     public void preferences_pane_destroy(Gtk.Container container)
     {

     }
}
