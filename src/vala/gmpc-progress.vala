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
    private Gtk.ProgressBar bar     = null;
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
    ~Progress() {
        if(this.my_style != null)
            this.my_style.detach();
    }
    public override void style_set (Gtk.Style? old_style)
    {
        /* Reset it, so it gets reloaded on redraw */
        if(this.my_style != null) {
            this.my_style.detach();
      }
        this.my_style = null;
    }


    /* Construct function */
    construct {
        //this.add_events((int)Gdk.EventMask.EXPOSURE_MASK);

        this.app_paintable = true;

        this.bar = new Gtk.ProgressBar();
        this.bar.set_parent(this);
        this.expose_event += this.on_expose;
    }
    
    // The size_request method Gtk+ is calling on a widget to ask
    // it the widget how large it wishes to be. It's not guaranteed
    // that gtk+ will actually give this size to the widget
    public override void size_request (out Gtk.Requisition requisition)
    {
        Gtk.Widget widget = this;
        Pango.Rectangle logical_rect;
        int width, height;
        int xspacing=0, yspacing=0;

        this.bar.set_style(this.my_style);
        this.bar.style_get (
                "xspacing", out xspacing,
                "yspacing", out yspacing,
                null);

        width = 2 * widget.style.xthickness + xspacing;
        height = 2 * widget.style.ythickness + yspacing;

        if (!this.hide_text)
        {
            var layout = widget.create_pango_layout (" ");

            layout.get_pixel_extents (null, out logical_rect);
            width += logical_rect.width;

            height += logical_rect.height;
        } 
         requisition.width = width; 
         requisition.height = height; 
         /*     int width, height;

        // In this case, we say that we want to be as big as the
        // text is, plus a little border around it.
        if(this.hide_text) {
        requisition.width = 40;
        requisition.height = 10;
        } else {
        var layout = this.create_pango_layout(" ");
        layout.get_size (out width, out height);
        requisition.width = width / Pango.SCALE + 6;
        requisition.height = height / Pango.SCALE + 6;
        }
         */
    }

    private void redraw ()
    {
        if(this.window  != null)
        {
            this.window.process_updates(false);
        }
    }

    private bool on_expose (Progress pb, Gdk.EventExpose event) 
    {
        Pango.Rectangle logical_rect;
        int x, y, w, h, perc_w, pos;
        Gdk.Rectangle clip = {0,0,0,0};

        if(this.my_style == null){
            this.my_style = Gtk.rc_get_style_by_paths(this.get_settings(), 
                        "GtkProgressBar","GtkProgressBar",
                        typeof(Gtk.ProgressBar));
            this.my_style = this.my_style.attach(event.window);
        }


        x = 0;
        y = 0; 
      /*  Gtk.paint_box (this.my_style, event.window, Gtk.StateType.NORMAL, Gtk.ShadowType.NONE,
                        null, this.bar, "", 0,0,this.allocation.width, this.allocation.height);
                        */
        w = this.allocation.width; 
        h = this.allocation.height; 
        Gtk.paint_box (this.my_style,
                    event.window,
                    Gtk.StateType.NORMAL,Gtk.ShadowType.IN,
                    event.area, this.bar,"trough",
                    x,y,w,h);

        x += this.my_style.xthickness;
        y += this.my_style.ythickness;
        w -= this.my_style.xthickness * 2;
        h -= this.my_style.ythickness * 2;
        perc_w = 0;
        if(this.total > 0) 
            perc_w =(int) (w * (this.current/(double)this.total));
        if(perc_w > w) perc_w = w ;
        if(perc_w > 0)
        {
            Gtk.paint_box (this.my_style,event.window,
            Gtk.StateType.PRELIGHT,Gtk.ShadowType.OUT,
            event.area, this.bar,"bar",
            x,y,perc_w,h);
        }
        if(this.hide_text == false)
        {
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
            var layout = pb.create_pango_layout (a);
            layout.get_pixel_extents (null, out logical_rect);

            pos = (w - logical_rect.width)/2;

            clip.x = x;
            clip.y = y;
            clip.width =  perc_w;
            clip.height = h; 

            Gtk.paint_layout (this.my_style, window, 
                    Gtk.StateType.SELECTED,
                    false, clip, this.bar, "progressbar",
                    x + pos, y + (h - logical_rect.height)/2,
                    layout);

            clip.x = clip.x + clip.width;
            clip.width = w - clip.width;

            Gtk.paint_layout (this.my_style, window, 
                    Gtk.StateType.NORMAL,
                    false, clip, this.bar, "progressbar",
                    x + pos, y + (h - logical_rect.height)/2,
                    layout);
        }
        return false;
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
