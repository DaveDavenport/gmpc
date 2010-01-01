
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

using GLib;
using Config;
using Gtk;
using Gdk;
using Cairo;
using MPD;
using Gmpc;
using Gmpc.Rating;


private const bool use_transition_mir = Gmpc.use_transition;

public class Gmpc.MenuItem.Rating : Gtk.MenuItem
{
    private const string some_unique_name = Config.VERSION;
    public Gtk.VBox hbox = null;
    public Gmpc.Rating rating = null;

    public int get_rating ()
    {
        return 0;
    }
    bool button_press_event_callback(Gdk.EventButton event, void *userdata)
    {
        this.rating.button_press_event_callback(this.rating.event_box, event);
        return true;
    }

    bool button_release_event_callback(Gdk.EventButton event, void *userdata)
    {
        return true;
    }

    public Rating (MPD.Server server, MPD.Song song)
    {
        /* this fixes vala bitching */
        GLib.Signal.connect_swapped(this, "button-press-event", (GLib.Callback)button_press_event_callback,this);
        GLib.Signal.connect_swapped(this, "button-release-event", (GLib.Callback)button_release_event_callback,this);

        this.hbox = new Gtk.VBox(false,6);
        this.rating = new Gmpc.Rating(server,song);

        this.hbox.pack_start(new Gtk.Label(_("Rating:")),false,true,0);
        this.hbox.pack_start(this.rating,false,true,0);
        this.add(this.hbox);
        this.show_all();
    }
}
