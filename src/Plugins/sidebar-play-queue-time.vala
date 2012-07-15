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
public class Gmpc.Plugins.Sidebar.PlayQueueTime : Gmpc.Plugin.Base, Gmpc.Plugin.SidebarIface
{
    private const int[] version = {0,0,0};
    private Gtk.Label time_label = null;

    private void status_changed(MPD.Server mi, MPD.Status.Changed what)
    {
        if((what&MPD.Status.Changed.PLAYLIST) == MPD.Status.Changed.PLAYLIST)
        {
            update();
        }
    }

    public PlayQueueTime()
    {
        Gmpc.gmpcconn.status_changed.connect(status_changed);
        Gmpc.playlist.total_playtime_changed.connect((loaded_song, total_playtime)=>
        {
            update_label(total_playtime, loaded_song);
        });
    }
    private void update_label(ulong total_playtime, ulong loaded_songs)
    {
        if(time_label == null) return;
        if(total_playtime > 0 && loaded_songs > 0)
        {
            int length = MPD.PlayQueue.length(Gmpc.server);
            if(loaded_songs < length)
            {
                ulong est_playtime = (ulong)GLib.Math.floor(total_playtime* (length/(double)loaded_songs));
                string time = Gmpc.Misc.format_time_newline(est_playtime);
                time_label.set_label(time+_("(estimation)"));
            }
            else
            {
                string time = Gmpc.Misc.format_time_newline(total_playtime);
                time_label.set_label(time);
            }
        }
        else
        {
            time_label.set_label(_("n/a"));
        }
    }
    private void update()
    {
        ulong loaded_song;
        ulong total_playtime;
        Gmpc.playlist.get_total_playtime(out loaded_song, out total_playtime);
        update_label(total_playtime, loaded_song);
    }
    public override unowned int[] get_version()
    {
        return this.version;
    }

    public override unowned string get_name()
    {
        return "Sidebar play queue time";
    }

    /* We don't want a title */
    public string sidebar_get_title()
    {
        return "Play Queue playtime";
    }

    public int sidebar_get_position()
    {
        return -1;
    }

    public void sidebar_set_state(Gmpc.Plugin.SidebarState state)
    {
        if(time_label == null) return;
        if(state == Plugin.SidebarState.COLLAPSED)
        {
            time_label.hide();
        }
        else
        {
            time_label.show();
        }
    }

    public void sidebar_pane_construct(Gtk.VBox parent)
    {
        stdout.printf("sidebar pane construct");
        time_label = new Gtk.Label("play time");
        time_label.set_line_wrap(true);

        (time_label as Gtk.Misc).set_alignment(0.0f, 0.5f);
        (time_label as Gtk.Misc).set_padding(5,0);
        parent.pack_start(time_label, false, false, 0);
        parent.show_all();
        this.sidebar_set_state(Gmpc.Playlist.get_sidebar_state());
        update();
    }

    public void sidebar_pane_destroy(Gtk.VBox parent)
    {
        foreach(Gtk.Widget child in parent.get_children())
        {
            parent.remove(child);
        }
        time_label = null;
    }
}
