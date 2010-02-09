
/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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
using Config;
using Gtk;
using Gmpc;

private const bool use_transition_mb = Gmpc.use_transition;
private const string some_unique_name_mb = Config.VERSION;

public class  Gmpc.Plugin.DatabaseUpdateTracker : Gmpc.Plugin.Base {
    private Gtk.Image image = null;
    public const int[] version = {0,0,2};

    public override weak int[] get_version() {
        return this.version;
    }

    public override weak string get_name() {
        return "Database Update Tracker";
    }

    construct {
        /* Mark the plugin as an internal dummy */
        this.plugin_type = 8+4;
        /* Attach status changed signal */
        gmpcconn.status_changed.connect(status_changed);
        gmpcconn.connection_changed.connect(connection_changed);
    }

    private void start_updating()
    {
        if(this.image != null) return;
        this.image = new Gtk.Image.from_icon_name("gtk-refresh", Gtk.IconSize.MENU); 
        this.image.show();
        this.image.set_tooltip_text(_("MPD is rescanning the database"));
        Gmpc.Playlist3.add_status_icon(this.image);
    }
    private void stop_updating()
    {
        if(this.image == null) return;

        this.image.parent.remove(this.image);
        this.image = null;
    }
    private void show_message(int db_time )
    {
        time_t r_time = (time_t) db_time;
        string message;
        Time tm = Time.local(r_time);
        message = "%s %s".printf(_("MPD Database has been updated at:"), tm.format("%c"));

        Gmpc.Messages.show((string)message, Gmpc.Messages.Level.INFO);
    }
    private void connection_changed(Connection gc, MPD.Server server, int connection)
    {
        if(connection == 1)
        {
            string id = Gmpc.profiles.get_current_id();
            if(id != null)
            {
                var dut =  Gmpc.profiles.get_db_update_time(id);
                var serv_dut = server.get_database_update_time(); 
                if(dut != serv_dut)
                {
                    show_message(serv_dut);
                    Gmpc.profiles.set_db_update_time(id, serv_dut);
                }
            }
            if(server.is_updating_database())
            {
                start_updating();
            }
        }
        else{
            /* Remove icon on disconnect */
            stop_updating();
        }
    }
    private void status_changed(Connection gc, MPD.Server server, MPD.Status.Changed what)
    {
        if(!this.get_enabled()) return;
        if((what&MPD.Status.Changed.UPDATING) == MPD.Status.Changed.UPDATING)
        {
            if(server.is_updating_database()) {
                start_updating();
            }else{
                stop_updating();
            }
        }

        if((what&MPD.Status.Changed.DATABASE) == MPD.Status.Changed.DATABASE)
        {
            string id = Gmpc.profiles.get_current_id();
            if(id != null)
            {
                var dut =  Gmpc.profiles.get_db_update_time(id);
                var serv_dut = Gmpc.server.get_database_update_time(); 
                if(dut != serv_dut)
                {
                    show_message(serv_dut);
                    Gmpc.profiles.set_db_update_time(id, serv_dut);
                }
                stop_updating();
            }

        }
    }

}
