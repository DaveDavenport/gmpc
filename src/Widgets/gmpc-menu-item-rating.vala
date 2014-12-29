/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2014 Qball Cow <qball@sarine.nl>
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


private const bool use_transition_mir = Gmpc.use_transition;

public class Gmpc.MenuItem.Rating : Gtk.MenuItem
{
    private const string some_unique_name = Config.VERSION;
    public Gtk.VBox hbox = null;
    public Gmpc.Rating rating = null;
    private int value = 0;
    private MPD.Song song       = null;
    private unowned MPD.Server server   = null;

    public int get_rating ()
    {
        return 0;
    }
    bool button_press_event_callback(Gdk.EventButton event, void *userdata)
    {
        return true;
    }

    public Rating (MPD.Server server, MPD.Song song, int value)
    {
        this.server = server;
        this.song = song.copy();
        this.value = value;
        /* this fixes vala bitching */
        this.activate.connect(() => {
                MPD.Sticker.Song.set(this.server, this.song.file, "rating", (value).to_string());
        });


        var hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL,6);
        for (int i =0; i < 5; i++ ){
            var image = new Gtk.Image.from_icon_name("rating", Gtk.IconSize.MENU);
            hbox.pack_start(image, false, false, 0);
            if(i > (value/2.0 -0.01)) {
                image.set_sensitive(false);
            }
        }

        this.add(hbox);
        this.show_all();
    }
}
