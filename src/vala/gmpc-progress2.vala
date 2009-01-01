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
    private ProgressBar bar = new ProgressBar();

    public bool hide_text {
        get { 
        return _hide_text; }
        set {
            if(value){
                this.set_size_request(-1, 10);        
                this.bar.set_text("");
            }
            else
                this.set_size_request(-1, -1);        
            _hide_text = value; 
        }
    }

    public bool do_countdown {
        get { 
            return _do_countdown; }
        set {
            _do_countdown = value; 
            this.queue_resize();
        }
    }

    /* Construct function */
    construct {
        this.add(this.bar);
        this.show_all(); 
    }
    // The size_request method Gtk+ is calling on a widget to ask
    // it the widget how large it wishes to be. It's not guaranteed
    // that gtk+ will actually give this size to the widget
    /*
    public override void size_request (Gtk.Requisition requisition)
    {
        int width, height;
        // In this case, we say that we want to be as big as the
        // text is, plus a little border around it.
        if(this.hide_text)
        {
            requisition.width = 40;
            requisition.height = 10;
        }else{
            requisition.width = -1; 
            requisition.height = -1;
        }
    }
*/
    public void set_time(uint total, uint current)
    {
        if(this.total != total || this.current != current)
        {
            this.total = total;
            this.current = current;
            if(this.total > 0)
            {
                this.bar.set_fraction((double)this.current/(double)this.total);
            }
            else
            {
                this.bar.set_fraction(0.0);
            }
            /**
             * Draw text
             */
            if(this.hide_text == false)
            {

                int fontw, fonth;
                if(this.total == 0) {
                    string a;
                    if(this.current/60 > 99 ) {
                        a = "%02i:%02i".printf(
                                (int)this.current/3600,
                                (int)(this.current)%60);
                    } else {
                        a = "%02i:%02i".printf( 
                                (int)this.current/60,
                                (int)(this.current)%60);
                    }
                    this.bar.set_text(a);
                } else {
                    string a;
                    uint p = this.current;
                    if(this.do_countdown){
                        p = this.total-this.current;
                    }
                    if(this.current/60 > 99 ) {
                        a  = "%c%02u:%02u - %02u:%02u".printf( 
                                (this.do_countdown)?'-':' ',
                                p/3600,
                                (p)%60,
                                this.total/3600,
                                (this.total)%60 
                                );
                    } else {
                        a = "%c%02u:%02u - %02u:%02u".printf( 
                                (this.do_countdown)?'-':' ',
                                p/60,
                                (p)%60,
                                this.total/60,
                                (this.total)%60 
                                );
                    }                                       
                    this.bar.set_text(a);
                }
            } else {
                this.bar.set_text(" ");
            }
        }
    }
}
