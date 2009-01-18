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

    /* Construct function */
    construct {

        this.scale = new Gtk.HScale(null);
        this.scale.set_range(0.0,1.0);
        this.scale.draw_value = false;
        this.set_value_handler = GLib.Signal.connect_swapped(this.scale,"value_changed",(GLib.Callback)value_changed,this);
        this.scale.update_policy = Gtk.UpdateType.DISCONTINUOUS;
        this.scale.sensitive = false;

        this.scale.add_events((int)Gdk.EventMask.SCROLL_MASK);
        this.scale.scroll_event += scroll_event;
        this.scale.button_press_event += button_press_event;

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
                stdout.printf("changed: %u %u\n", seconds, this.current);
                seek_event(seconds);
            }else{
                uint seconds = (uint)(this.total*(range.get_value()));
                stdout.printf("changed: %u %u\n", seconds, this.current);
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
                stdout.printf("right button press\n");
                this.do_countdown = !this.do_countdown;
                this.scale.inverted = this.do_countdown;
                var cur = this.current;
                var tot = this.total;
                this.total=this.current = 0;
                set_time(tot,cur);
            }
        }
        return false;
    }

    private bool scroll_event (Gtk.Scale scale,Gdk.EventScroll event)
    {
        stdout.printf("scrolling\n");
        if(event.direction == Gdk.ScrollDirection.UP)
        {
            if(this.do_countdown) {
                scale.set_value(1-(this.current+5)/(double)this.total); 
            }else{
                scale.set_value((this.current+5)/(double)this.total); 
            }
        }
        else if (event.direction == Gdk.ScrollDirection.DOWN)
        {
            if(this.do_countdown) {
                scale.set_value(1-(this.current-5)/(double)this.total); 
            }else{
                scale.set_value((this.current-5)/(double)this.total); 
            }
        }
        return false;
    }

    public void set_time(uint total, uint current)
    {
        if(this.total != total || this.current != current)
        {
            this.total = total;
            this.current = current;

            GLib.SignalHandler.block(this.scale, this.set_value_handler);
            if(this.total > 0) {
                this.scale.sensitive = true;
                if(this.do_countdown){
                    this.scale.set_value(1-this.current/(double)this.total);
                }else{
                    this.scale.set_value(this.current/(double)this.total);
                }

            } else {
                this.scale.sensitive = false;
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
