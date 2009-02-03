
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
    private Gtk.Window window = null;
    
    private
    bool
    completion_function(Gtk.EntryCompletion comp, string key, Gtk.TreeIter iter)
    {
        string value;
        var model = comp.model;

        model.get(iter, 1, out value, -1);
        if(value != null){
            string a = "^%s.*".printf(key);
            return GLib.Regex.match_simple(a, value, GLib.RegexCompileFlags.CASELESS, 0);
        }

        return false;
    }
    construct {
        this.store = new Gtk.ListStore(6,typeof(uint), typeof(string), typeof(string),typeof(void *), typeof(void *),typeof(string));
        this.completion  = new Gtk.EntryCompletion();
        this.completion.model = this.store;
        this.completion.text_column = 1;
        this.completion.inline_completion = true;
        this.completion.inline_selection = true;
        this.completion.popup_completion = true;

        this.completion.set_match_func(completion_function, null);

        var renderer = new Gtk.CellRendererText();
        this.completion.pack_end(renderer, false);
        this.completion.add_attribute(renderer, "text", 5);
        renderer.set("foreground", "grey",null);
    }


    public delegate void Callback (void *data, string param);
    public
    uint add_entry(string name, string pattern,string hint, Callback *callback, void *userdata)
    {
        Gtk.TreeIter iter;
        this.signals++;
        this.store.append(out iter);
        this.store.set(iter, 0,this.signals,1,name,2,pattern,3, callback,4, userdata,5,hint,-1);
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
            this.window.destroy();
            this.window = null;
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

                test = "%s[ ]*%s$".printf(name, pattern);
                if(GLib.Regex.match_simple(test, value, GLib.RegexCompileFlags.CASELESS, 0))
                {
                    stdout.printf("matched: %s to %s\n", test, value);
                    var param = value.substring(name.length, -1);
                    var param_str = param.strip();
                    callback(data, param_str); 
                    window.destroy();
                    window =null;
                    return;
                }
            }while(model.iter_next(ref iter));
        }
        window.destroy();
        window = null;
        Gmpc.Messages.show("Unknown command: '%s'".printf(entry.get_text()), Gmpc.Messages.Level.INFO);
    }
    private bool key_press_event(Gtk.Widget widget, Gdk.EventKey event)
    {
        /* Escape */
        if(event.keyval == 0xff1b)
        {
            this.window.destroy();
            this.window = null;
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

    private
    bool
    popup_expose_handler(Gtk.Widget widget, Gdk.EventExpose event)
    {
        var ctx =  Gdk.cairo_create(widget.window);
        int width = widget.allocation.width;
        int height = widget.allocation.height;


        if(widget.is_composited())
        {
            ctx.set_operator(Cairo.Operator.SOURCE);
            ctx.set_source_rgba(1.0,1.0,1.0,0.0);
        }
        else{
            ctx.set_source_rgb(1.0,1.0,1.0);
        }

        ctx.paint();
        /* */

        ctx.rectangle(1.0,1.0,width-2,height-2);
        var pattern = new Cairo.Pattern.linear(0.0,0.0,0.0, height);

        pattern.add_color_stop_rgba(0.0,0.0,0.0,0.0,0.5);
        pattern.add_color_stop_rgba(0.5,0.0,0.0,0.0,1.0);
        pattern.add_color_stop_rgba(1.0,0.0,0.0,0.0,0.5);
        ctx.set_source(pattern);
        ctx.fill_preserve();
        ctx.set_source_rgba(1.0,1.0,1.0,1.0);
        ctx.stroke();

        ctx.rectangle(0.0,0.0,width,height);
        ctx.set_source_rgba(0.0,0.0,0.0,1.0);
        ctx.stroke();


        return false;
    }
    public
    void
    popup(Gtk.Widget win)
    {
        if(this.window == null)
        {
            this.window = new Gtk.Window(Gtk.WindowType.TOPLEVEL);
            var entry = new Gtk.Entry();

            window.border_width=24;
            entry.width_chars = 50;

            window.add(entry);


            /* Composite */
            if(window.is_composited()) {
                var screen = window.get_screen();
                var colormap = screen.get_rgba_colormap();
                window.set_colormap(colormap);
            }
            window.app_paintable = true;
            window.expose_event += popup_expose_handler;

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
            window.present();
            entry.grab_focus();
        }
        else{
            this.window.present();
        }
    }
}
