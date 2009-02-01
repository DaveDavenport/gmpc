
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

    construct {
        this.store = new Gtk.ListStore(5,typeof(uint), typeof(string), typeof(string),typeof(void *), typeof(void *));
        this.completion  = new Gtk.EntryCompletion();
        this.completion.model = this.store;
        this.completion.text_column = 1;
        this.completion.inline_completion = true;
        this.completion.inline_selection = true;
        this.completion.popup_completion = false;
    }


    public delegate void Callback (void *data, string param);
    public
    uint add_entry(string name, string pattern, Callback *callback, void *userdata)
    {
        Gtk.TreeIter iter;
        this.signals++;
        this.store.append(out iter);
        this.store.set(iter, 0,this.signals,1,name,2,pattern,3, callback,4, userdata,-1);
        return this.signals;
    }

    public
    void
    activate(Gtk.Entry entry)
    {
        weak Gtk.TreeModel model = this.store;
        string value = entry.get_text();
        Gtk.TreeIter iter;
        if(value.length == 0)
        {
            entry.get_toplevel().destroy();
            return;
        }
        /* ToDo: Make this nicer... maybe some fancy parsing */ 
        if(model.get_iter_first(out iter))
        {
            do{
                string name,pattern,test;
                Callback callback = null;
                void * data;
                model.get(iter, 1, out name,2, out pattern, 3, out callback,4, out data, -1);

                test = "%s[ ]*%s".printf(name, pattern);
                if(GLib.Regex.match_simple(test, value, GLib.RegexCompileFlags.CASELESS, 0))
                {
                    var param = value.substring(name.length, -1);
                    callback(data, param); 
                    entry.get_toplevel().destroy();
                    return;
                }
            }while(model.iter_next(ref iter));
        }
        entry.get_toplevel().destroy();
        Gmpc.Messages.show("Unknown command: '%s'".printf(entry.get_text()), Gmpc.Messages.Level.INFO);
    }
    private bool key_press_event(Gtk.Widget widget, Gdk.EventKey event)
    {
        /* Escape */
        if(event.keyval == 0xff1b)
        {
            widget.get_toplevel().destroy();
            return true;
        }
        /* Tab key */
        else if (event.keyval == 0xff09)
        {
            ((Gtk.Editable) widget).set_position(-1); 
            return true;
        }
        return false;
    }
    public
    void
    popup(Gtk.Widget win)
    {
        var window = new Gtk.Window(Gtk.WindowType.TOPLEVEL);
        var entry = new Gtk.Entry();
        window.add(entry);

        /* Setup window */
        window.decorated = false;
        window.modal = true;
        window.set_keep_above(true);
        window.set_transient_for((Gtk.Window)win);
        window.position = Gtk.WindowPosition.CENTER_ON_PARENT;

        /* setup entry */
        entry.set_completion(this.completion);
        entry.activate += this.activate;
        entry.key_press_event += this.key_press_event;



        window.show_all();
        entry.grab_focus();
    }
}
