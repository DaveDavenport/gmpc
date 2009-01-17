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
    public bool _hide_text          = false;

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
        this.expose_event += this.on_expose;
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
            var layout = this.create_pango_layout(" ");
            layout.get_size (out width, out height);
            requisition.width = width / Pango.SCALE + 6;
            requisition.height = height / Pango.SCALE + 6;
        }
  //      this.cell.width = requisition.width;
    //    this.cell.height = requisition.height;
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
        Gdk.GC gc;
        Pango.Rectangle logical_rect;
        int x, y, w, h, perc_w, pos;
        Gdk.Rectangle clip = {0,0,0,0};

        gc = new Gdk.GC(event.window);

        x = 0;
        y = 0; 

        w = pb.allocation.width; 
        h = pb.allocation.height; 

        var style = pb.style;//this.my_style;
        gc.set_rgb_fg_color(style.fg[Gtk.StateType.NORMAL]);

        Gtk.paint_box (style,event.window,Gtk.StateType.NORMAL,Gtk.ShadowType.IN,event.area, pb,"through",x,y,w,h);

        x += style.xthickness;
        y += style.ythickness;
        w -= style.xthickness * 2;
        h -= style.ythickness * 2;
        gc.set_rgb_fg_color (style.bg[Gtk.StateType.SELECTED]);
        perc_w = 0;
        if(this.total > 0) 
            perc_w =(int) (w * (this.current/(double)this.total));
        if(perc_w > w) perc_w = w ;
        if(perc_w > 0)
        {
            Gtk.paint_box (style,event.window,Gtk.StateType.NORMAL,Gtk.ShadowType.IN,event.area, pb,"bar",x,y,perc_w,h);
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

            Gtk.paint_layout (style, window, 
                    Gtk.StateType.SELECTED,
                    false, clip, pb, "progressbar",
                    x + pos, y + (h - logical_rect.height)/2,
                    layout);

            clip.x = clip.x + clip.width;
            clip.width = w - clip.width;

            Gtk.paint_layout (style, window, 
                    Gtk.StateType.NORMAL,
                    false, clip, pb, "progressbar",
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
