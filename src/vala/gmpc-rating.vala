
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
using Gtk;
using Gdk;
using Cairo;
using MPD;
using Gmpc;



public class Gmpc.Rating : Gtk.Frame
{
    private const bool use_transition = Gmpc.use_transition;
    private weak MPD.Server server  = null;
    private MPD.Song song       = null;
    private Gtk.Image[] rat;
    private Gtk.HBox    box;
    public Gtk.EventBox event_box;
    private int rating = -1;
    static int id = id+1; 

    private ulong status_changed_id = 0;
   
    ~Rating() {
        if (this.status_changed_id > 0 &&  GLib.SignalHandler.is_connected(gmpcconn, this.status_changed_id)) {
            GLib.SignalHandler.disconnect(gmpcconn, this.status_changed_id);
            this.status_changed_id = 0;
        }

    }
    public bool button_press_event_callback(Gtk.EventBox wid, Gdk.EventButton event)
    {
        if(event.type == Gdk.EventType.BUTTON_PRESS)
        {
            if(event.button == 1)
            {
                int width = this.allocation.width;
                int button = (int)((((event.x)/(double)width)+0.15)*5);
                MPD.Sticker.Song.set(this.server, this.song.file, "rating", button.to_string());
                this.set_rating(button);
            }
        }

        return false;
    }

    private void status_changed ( MPD.Server server, MPD.Status.Changed what,Gmpc.Connection conn)
    {
        if(((what&MPD.Status.Changed.STICKER) != 0))
        {
            this.update();
        }
    }

    public Rating (MPD.Server server, MPD.Song song) {
        this.server = server;
        this.song = song;
        this.update();
        this.status_changed_id = GLib.Signal.connect_swapped(gmpcconn, "status_changed", (GLib.Callback)status_changed, this);
    }

    construct {
        int i;
        this.shadow = Gtk.ShadowType.NONE;

        this.box = new Gtk.HBox(true,6);
        this.event_box = new Gtk.EventBox();
        this.event_box.visible_window = false;
        this.rat = new Gtk.Image[5];
        for(i=0;i<5;i++) {
            this.rat[i] = new Gtk.Image.from_icon_name("rating", Gtk.IconSize.MENU);
            this.box.pack_start(this.rat[i], false, false, 0);
        }

        this.add(this.event_box);
        this.event_box.add(this.box);

        this.event_box.button_press_event += button_press_event_callback;
        this.show_all();
    }


    public void set_rating(int rating)
    {
        int i=0;
        if(rating != this.rating)
        {
            for(i=0;i<5;i++)
            {
                this.rat[i].sensitive = i<rating;
            }
            this.rating = rating;
        }
    }
    public void update()
    {
        var value = MPD.Sticker.Song.get(this.server,this.song.file, "rating");
        if(value == null) {
            this.set_rating(0);            
        } else {
            this.set_rating(value.to_int());
        }
    }
}
