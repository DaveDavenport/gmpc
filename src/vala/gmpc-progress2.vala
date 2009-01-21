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


public class Gmpc.Progress : Gtk.HBox
{
    private uint total              = 0;
    private uint current            = 0;
    private bool do_countdown      = false;
    public bool _hide_text          = false;
    private Gtk.Scale scale        = null;
    private Gtk.Label label        = null;
    private ulong set_value_handler = 0;
    private Gtk.Window tooltip          = null;
    private Gtk.Label tooltip_label = null;

    public bool hide_text {
        get { 
        return _hide_text; }
        set {
            _hide_text = value; 
            if(_hide_text) {
                this.label.hide();
            }else{
                this.label.show();
            }
        }
    }
    private bool tooltip_expose_event(Gtk.Window tooltip, Gdk.EventExpose event)
    {
        Gtk.paint_box(tooltip.style, 
                event.window,
                Gtk.StateType.NORMAL,
                Gtk.ShadowType.OUT,
                null, tooltip, 
                "tooltip", 0,0,
                tooltip.allocation.width, tooltip.allocation.height);

        return false;
    }

    private bool enter_notify_event(Gtk.Scale scale, Gdk.EventCrossing event)
    {
        if (event.type == Gdk.EventType.ENTER_NOTIFY)
        {
            tooltip = new Gtk.Window(Gtk.WindowType.POPUP);
            tooltip_label = new Gtk.Label("test");
            tooltip.add(tooltip_label);
            tooltip.border_width = 4;
            tooltip.set_app_paintable(true);
            tooltip.expose_event += tooltip_expose_event;
        }
        if (event.type == Gdk.EventType.LEAVE_NOTIFY)
        {
            if(tooltip != null)
            {
                tooltip.destroy();
                tooltip = null;
            }
        }
        return false;
    }
    private bool motion_notify_event(Gtk.Scale scale, Gdk.EventMotion event)
    {
        if(event.type == Gdk.EventType.MOTION_NOTIFY)
        {
            if(tooltip != null)
            {
                int e_hour, e_minutes, e_seconds;
                int t_hour =    (int) this.total / 3600;
                int t_minutes = (int) this.total%3600/60;
                int t_seconds = (int) this.total%60;
                string a = "";
                uint p = (uint)(this.total * (event.x/(double)(scale.allocation.width-scale.style.xthickness)));
                if(this.do_countdown){
                    p = (uint)(this.total * (event.x/(double)(scale.allocation.width-scale.style.xthickness)));
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
                    a += " - ";
                    if(t_hour>0) {
                        a += "%02i".printf(t_hour);
                        if(t_minutes > 0) {
                            a+=":";
                        }
                    }
                    a += "%02i:%02i".printf(t_minutes,t_seconds);
                }
                if(this.do_countdown)
                    this.tooltip_label.width_chars = (int)a.length;
                else 
                    this.tooltip_label.width_chars = (int)a.length+1; 
                this.tooltip_label.set_text(a);

                tooltip.show_all();
                tooltip.realize();
                tooltip.move((int)event.x_root-tooltip.allocation.width/2, (int)event.y_root+tooltip.allocation.height);
            }
        }
        return false;
    }

    /* Construct function */
    construct {

        this.scale = new Gtk.HScale(null);
        this.scale.set_range(0.0,1.0);
        this.scale.draw_value = false;
        this.set_value_handler = GLib.Signal.connect_swapped(this.scale,"value_changed",(GLib.Callback)value_changed,this);
        this.scale.update_policy = Gtk.UpdateType.DISCONTINUOUS;
        this.scale.sensitive = false;

        this.scale.add_events((int)Gdk.EventMask.SCROLL_MASK);
        this.scale.add_events((int)Gdk.EventMask.POINTER_MOTION_MASK);
        this.scale.add_events((int)Gdk.EventMask.ENTER_NOTIFY_MASK);
        this.scale.add_events((int)Gdk.EventMask.LEAVE_NOTIFY_MASK);
        this.scale.scroll_event += scroll_event;
        this.scale.button_press_event += button_press_event;
        this.scale.motion_notify_event += motion_notify_event;
        this.scale.enter_notify_event += enter_notify_event;
        this.scale.leave_notify_event += enter_notify_event;

        this.label = new Gtk.Label("");
        this.label.set_alignment(1.0f,0.5f);

        this.pack_start(this.scale, true,true,0);
        this.pack_end(this.label, false,true,0);
        this.show_all();

    }
   
    signal void seek_event (uint seek_time);


    private void value_changed (Gtk.Scale range)
    {
        if(this.total > 0)
        {
            if(this.do_countdown)
            {
                uint seconds = (uint)(this.total*(1-range.get_value()));
                seek_event(seconds);
            }else{
                uint seconds = (uint)(this.total*(range.get_value()));
                seek_event(seconds);
            }
        }
    }

    private bool button_press_event (Gtk.Scale scale, Gdk.EventButton event)
    {
        if(event.type == Gdk.EventType.BUTTON_PRESS)
        {
            if(event.button == 3)
            {
                this.do_countdown = !this.do_countdown;
                this.scale.inverted = this.do_countdown;
                var cur = this.current;
                var tot = this.total;
                this.total=this.current = 0;
                set_time(tot,cur);
            }
            if(event.button == 2 || event.button == 1)
            {
                uint p = (uint)(this.total * (event.x/(double)(scale.allocation.width-scale.style.xthickness)));
                seek_event(p);
                return true;
            }
        }
        return false;
    }

    private bool scroll_event (Gtk.Scale scale,Gdk.EventScroll event)
    {
        if(event.direction == Gdk.ScrollDirection.UP)
        {
            seek_event(this.current+5); 
        }
        else if (event.direction == Gdk.ScrollDirection.DOWN)
        {
            seek_event(this.current-5); 
        }
        return false;
    }

    public void set_time(uint total, uint current)
    {
        if(this.total != total)
        {
            this.scale.sensitive = (total > 0);
        }

        if(this.total != total || this.current != current)
        {
            this.total = total;
            this.current = current;

            GLib.SignalHandler.block(this.scale, this.set_value_handler);
            if(this.total > 0) {
                if(this.do_countdown){
                    this.scale.set_value(1-this.current/(double)this.total);
                }else{
                    this.scale.set_value(this.current/(double)this.total);
                }

            } else {
                this.scale.set_value(0.0);
            }

            GLib.SignalHandler.unblock(this.scale, this.set_value_handler);

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
                    a += " - ";
                    if(t_hour>0) {
                        a += "%02i".printf(t_hour);
                        if(t_minutes > 0) {
                            a+=":";
                        }
                    }
                    a += "%02i:%02i".printf(t_minutes,t_seconds);
                }
                if(this.do_countdown)
                    this.label.width_chars = (int)a.length;
                else 
                    this.label.width_chars = (int)a.length+1; 
                this.label.set_text(a);
            }
        }
    }
}
