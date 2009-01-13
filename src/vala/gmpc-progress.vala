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


public class Gmpc.Progress : Gtk.EventBox 
{
    private uint total              = 0;
    private uint current            = 0;
    private bool _do_countdown      = false;
    private Pango.Layout _layout    = null;
    public bool _hide_text          = false;
    private Style my_style = null;

    public bool hide_text {
        get { 
        return _hide_text; }
        set {
            _hide_text = value; 
            this.queue_resize();
        }
    }

    public bool do_countdown {
        get { 
            return _do_countdown; 
        }
        set {
            _do_countdown = value; 
            this.redraw();
        }
    }

    /* Construct function */
    construct {
        this.app_paintable = true;
        this.expose_event += this.on_expose2;
        /* Set a string so we can get height */
        this._layout = this.create_pango_layout (" ");

    }

    ~Progress() {
        _layout.unref();
	if(this.my_style != null)
		this.my_style.detach();
    }
    
    public override void style_set (Gtk.Style old_style)
    {
        /* Reset it, so it gets reloaded on redraw */
        if(this.my_style != null) {
            this.my_style.detach();
        }
        this.my_style = null;
    }

    // The size_request method Gtk+ is calling on a widget to ask
    // it the widget how large it wishes to be. It's not guaranteed
    // that gtk+ will actually give this size to the widget
    public override void size_request (out Gtk.Requisition requisition)
    {
        int width, height;
        // In this case, we say that we want to be as big as the
        // text is, plus a little border around it.
        if(this.hide_text) {
            requisition.width = 40;
            requisition.height = 10;
        } else {
            this._layout.get_size (out width, out height);
            requisition.width = width / Pango.SCALE + 6;
            requisition.height = height / Pango.SCALE + 6;
        }
    }

    private void redraw ()
    {
        if(this.window  != null)
        {
            this.window.process_updates(false);
        }
    }

    private bool on_expose2 (Progress pb, Gdk.EventExpose event) 
    {
        var ctx = Gdk.cairo_create(this.window); 
        int width = this.allocation.width-1;
        int height = this.allocation.height-1;
        int pw = width-3;
        int pwidth = (int)((this.current*pw)/(double)this.total);

        if(this.my_style == null){
            this.my_style = Gtk.rc_get_style_by_paths(this.get_settings(), null, null, typeof(ProgressBar));
            this.my_style = this.my_style.attach(this.window);
        }

        if(pwidth > pw)
        {
            pwidth = pw;
        }
        /* Draw border */
        ctx.set_line_width ( 1.0 );
        ctx.set_tolerance ( 0.2 );
        ctx.set_line_join (LineJoin.ROUND);

        //paint background
        Gdk.cairo_set_source_color(ctx, pb.style.bg[(int)Gtk.StateType.NORMAL]);
        ctx.paint();


        ctx.new_path();
        /* Stroke a white line, and clip on that */
        Gdk.cairo_set_source_color(ctx, pb.my_style.dark[(int)Gtk.StateType.NORMAL]);
        ctx.rectangle(0.5,0.5,width, height);
        ctx.stroke_preserve ();

        Gdk.cairo_set_source_color(ctx, pb.my_style.bg[(int)Gtk.StateType.NORMAL]);
        ctx.fill_preserve();
        /* Make a clip */
        ctx.clip();

        if(this.total > 0)
        {
            /* don't allow more then 100% */
            if( pwidth > width ) {
                pwidth = width;
            }
            ctx.new_path();
            Gdk.cairo_set_source_color(ctx, pb.my_style.bg[(int)Gtk.StateType.SELECTED]);
            ctx.rectangle(0.5+2,0.5+2,pwidth, (height-4));
            ctx.fill ();

        }
        /* Paint nice reflection layer on top */
        ctx.new_path();
        var pattern =  new Pattern.linear(0.0,0.0, 0.0, height);
        var start = pb.my_style.dark[(int)Gtk.StateType.NORMAL];
        var stop = pb.my_style.light[(int)Gtk.StateType.NORMAL];

        pattern.add_color_stop_rgba(0.0,start.red/(65536.0), start.green/(65536.0), start.blue/(65536.0),   0.6);
        pattern.add_color_stop_rgba(0.40,stop.red/(65536.0), stop.green/(65536.0), stop.blue/(65536.0),      0.2);
        pattern.add_color_stop_rgba(0.551,stop.red/(65536.0), stop.green/(65536.0), stop.blue/(65536.0),   0.0);
        ctx.set_source(pattern);
        ctx.rectangle(1.5,1.5,width, height);

        ctx.fill();
        ctx.reset_clip();
        /**
         * Draw text
         */
        if(this.hide_text == false)
        {

            int fontw, fonth;
            int e_hour, e_minutes, e_seconds;
            int t_hour =    (int) this.total / 3600;
            int t_minutes = (int) this.total%3600/60;
            int t_seconds = (int) this.total%60;
            string a = "";
            uint p = this.current;
            if(this.do_countdown){
                p = this.total-this.current;
                a += "-";
            }
            e_hour = (int) p/3600;
            e_minutes = (int) (p%3600)/60;
            e_seconds = (int) (p%60);
            if(e_hour>0) {
                a += "%02i".printf(e_hour);
                if(e_minutes > 0) {
                    a+=":";
                }
            }
            a += "%02i:%02i".printf(e_minutes, e_seconds);
            if(this.total > 0)
            {
                a += " -  ";
                if(t_hour>0) {
                    a += "%02i".printf(t_hour);
                    if(t_minutes > 0) {
                        a+=":";
                    }
                }
                a += "%02i:%02i".printf(t_minutes,t_seconds);
            }

            this._layout.set_text(a,-1);

            Pango.cairo_update_layout (ctx, this._layout);
            this._layout.get_pixel_size (out fontw, out fonth);

            if(this.total > 0)
            {
                if(pwidth >= ((width-fontw)/2+1))
                {
                    ctx.new_path();
                    Gdk.cairo_set_source_color(ctx, pb.my_style.fg[(int)Gtk.StateType.SELECTED]);
                    ctx.rectangle(3.5, 0.5,pwidth, height);
                    ctx.clip();
                    ctx.move_to ((width - fontw)/2+0.5,
                            (height - fonth)/2+0.5);
                    Pango.cairo_show_layout ( ctx, this._layout);
                    ctx.reset_clip();
                }
                if(pwidth < ((width-fontw)/2+1+fontw))
                {
                    ctx.new_path();
                    Gdk.cairo_set_source_color(ctx, pb.my_style.fg[(int)Gtk.StateType.NORMAL]);
                    ctx.rectangle(pwidth+3.5, 0.5,width, height);
                    ctx.clip();
                    ctx.move_to ((width - fontw)/2+0.5,
                            (height - fonth)/2+0.5);
                    Pango.cairo_show_layout ( ctx, this._layout);
                }
            }
            else
            {
                ctx.new_path();
                Gdk.cairo_set_source_color(ctx, pb.my_style.fg[(int)Gtk.StateType.NORMAL]);
                ctx.move_to ((width - fontw)/2+0.5,
                        (height - fonth)/2+0.5);
                Pango.cairo_show_layout ( ctx, this._layout);
            }
        }
        return true;
    }

    public void set_time(uint total, uint current)
    {
        if(this.total != total || this.current != current)
        {
            this.total = total;
            this.current = current;
            this.queue_draw();
        }
    }
}
