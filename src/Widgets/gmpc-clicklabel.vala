/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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

using Gtk;
using Gmpc;

public class Gmpc.Clicklabel : Gtk.EventBox
{
    private Gtk.Label label                 = null;
    private int size                        = 10*Pango.SCALE; 
    private bool italic                     = false;
    private Gdk.Cursor hand_cursor          = new Gdk.Cursor(Gdk.CursorType.HAND2);
    private Pango.AttrList attributes       = null; 
    /**
     * Public functions
     */

    /**
     * Constructor 
     */
    public Clicklabel(string value)
    {
        /**
         * Attribute list 
         */
        attributes = new Pango.AttrList();

        /* evenbox has no window, otherwise it shows ugly 
         * with gradient themes
         */
        this.set_visible_window(false);

        /* Label to display the info */
        label = new Gtk.Label(value);
        /* align label to the left */
        label.set_alignment(0, 0.5f);

        /* get pango context */
        var fd = label.get_pango_context();
        /* get font description */
        var pfd = fd.get_font_description();
        /* set size */
        this.size = pfd.get_size();
          

        /* Set our own attributes */
        label.set_attributes(attributes);

        /* pack label */
        this.add(label);


        /**
         * Change mouse cursor to hand when focus in
         */
        this.enter_notify_event.connect((source, event) => {
            this.window.set_cursor(hand_cursor);
            return false;
        });

        /**
         * Change back when focus out 
         */
        this.leave_notify_event.connect((source, event) => {
            this.window.set_cursor(null);
            return false;
        });

        this.button_release_event.connect((source, event) => {
            if(event.button == 1) {
                clicked();
            }
            return false;
        });
    }

    /**
     * Set font size 
     */
    public void font_size(int nsize)
    {
        this.size = nsize*Pango.SCALE; 
        update();
    }

    /**
     * set ellipsize
     */
    public void set_ellipsize(Pango.EllipsizeMode mode)
    {
        label.set_ellipsize(mode);
    }

    /**
     * Set label text
     */
    public void set_text(string value)
    {
        label.set_text(value);
    }

    /**
     * Set do italic
     */
    public void set_do_italic(bool val)
    {
        italic = val;
        update();
    }

    public signal void clicked ();

    /**
     * Private functions
     */
     private void update()
     {
        /* TODO: Broken by vala */
        Pango.Attribute attr    = null;

        /* Set style  */
        if(this.italic) {
            attr                = Pango.attr_style_new(Pango.Style.ITALIC); 
        }else {
            attr                = Pango.attr_style_new(Pango.Style.NORMAL); 
        }
        attr.start_index        = 0;
        attr.end_index          = -1;
        Gmpc.Fix.change(attributes,(owned)attr);

        /* Set size */
        attr                    = new Pango.AttrSize(this.size);
        attr.start_index        = 0;
        attr.end_index          = -1;
        //attributes.change((owned)attr);
        Gmpc.Fix.change(attributes,(owned)attr);

     }
}
