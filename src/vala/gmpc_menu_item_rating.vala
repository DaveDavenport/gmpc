
/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
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
using Gmpc.Rating;

public class Gmpc.MenuItem.Rating : Gtk.MenuItem
{
    public Gtk.VBox hbox = null;
    public Gmpc.Rating rating = null;
//    private int rating = 0;

    public int get_rating ()
    {
        return 0;
    }
    override bool button_press_event(Gdk.EventButton event)
    {
        stdout.printf("Button press event\n");
        this.rating.button_press_event(this.rating.event, event);
        return true;
    }

    public Rating (MPD.Server server, MPD.Song song)
    {
/*        int i = 0;*/
        /*
        if(rating < 0) {
            this.rating = -rating;
            this.sensitive = false;
        }
        this.rating = rating;
        */
        this.hbox = new Gtk.VBox(false,6);
/*
        for(i=0;i<5;i++)
        {
            var image = new Gtk.Image.from_icon_name("emblem-favorite", Gtk.IconSize.MENU);
            image.sensitive = (i < rating);
            this.hbox.pack_start(image, false, true, 0);
        }*/
        this.rating = new Gmpc.Rating(server,song);

        this.hbox.pack_start(new Gtk.Label("Rating:"),false,true,0);
        this.hbox.pack_start(this.rating,false,true,0);
        this.add(this.hbox);
        this.show_all();
    }
}
