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

private const bool use_transition_snsong = Gmpc.use_transition;
private const string some_unique_name_snsong = Config.VERSION;

public class Gmpc.Plugins.SidebarNextSong : Gmpc.Plugin.Base, Gmpc.Plugin.SidebarIface
{

    private Grid hbox = null;
    private Gmpc.MetaData.Image AlbumImage = null;
    private Label label = null;

    private const int[] version = {0,0,0};
    private void status_changed(MPD.Server mi, MPD.Status.Changed what)
    {
        if(hbox == null) return;
        if((what&MPD.Status.Changed.NEXTSONG) == MPD.Status.Changed.NEXTSONG)
        {
            update();
        }
    }
    public SidebarNextSong()
    {
        Gmpc.gmpcconn.status_changed.connect(status_changed);
    }


    public override unowned int[] get_version()
    {
        return this.version;
    }

    public override unowned string get_name()
    {
        return "Sidebar Next Song";
    }


    /* We don't want a title */
    public string sidebar_get_title()
    {
        return "Next song:";
    }

    public int sidebar_get_position()
    {
        return -1;
    }
    public void sidebar_set_state(Gmpc.Plugin.SidebarState state)
    {
        if(hbox == null) return;
        if(state == Plugin.SidebarState.COLLAPSED)
        {
            AlbumImage.size = 24;
            label.hide();
        }
        else
        {
            AlbumImage.size = 32;
            label.show();
        }
        update();
    }
    private void update()
    {
        int id= Gmpc.server.player_get_next_song_id();

        // Make sure the image is redrawn.
        AlbumImage.set_dirty();
        if(id >= 0)
        {
            MPD.Song? song = Gmpc.server.playlist_get_song(id);
            AlbumImage.update_from_song(song);
            char[] buffer = new char[1024];
            song.markup(buffer, "%title%[\n%artist%]");
            label.set_text((string )buffer);
            hbox.set_tooltip_text((string)buffer);
        }
        else
        {
            AlbumImage.set_cover_na();
            label.set_text("");
        }
    }
    public void sidebar_pane_construct(Gtk.Grid parent)
    {
        hbox = new Grid();
        hbox.set_halign(Gtk.Align.FILL);
        hbox.set_column_spacing(6);

        AlbumImage = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, 32);
        hbox.add(AlbumImage);//, false, true, 0);
        AlbumImage.has_tooltip = false;

        label = new Gtk.Label("");
        label.set_hexpand(true);
        label.set_alignment(0f, 0.5f);
        label.set_halign(Gtk.Align.FILL);
        label.set_max_width_chars(3);
        label.set_ellipsize(Pango.EllipsizeMode.END);
        hbox.add(label);//, true, true, 0);

        parent.attach(hbox,0,1,1,1);//, false, false, 0);

        parent.show_all();
        this.sidebar_set_state(Gmpc.Playlist.get_sidebar_state());
    }

    public void sidebar_pane_destroy(Gtk.Grid parent)
    {
        foreach(Gtk.Widget child in parent.get_children())
        {
            parent.remove(child);
        }
        hbox = null;
    }
}
