
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

public class Gmpc.Easy.Command : GLib.Object
{
    private Gtk.EntryCompletion completion = null;
    private Gtk.ListStore store = null;
    private uint signals = 0;


    Command () {
        this.store = new Gtk.ListStore(4,typeof(uint), typeof(string), typeof(void *), typeof(void *));
        this.completion  = new Gtk.EntryCompletion();
        this.completion.model = this.store;
    }


    public
    uint add_entry(string name, GLib.Callback callback, void *userdata)
    {
        Gtk.TreeIter iter;
        this.signals++;
        this.store.append(out iter);
        this.store.set(iter, this.signals,  this.signals, callback, userdata);
        return this.signals;
    }
}
