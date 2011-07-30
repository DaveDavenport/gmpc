/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/
 
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
    private new bool sensitive              = false;
    private int size                        = 10*Pango.SCALE; 
    private bool italic                     = false;
    private bool bold                       = false;
    private bool underline                  = false;
    private Pango.AttrList attributes       = null;
    private bool mouseCursorInside          = false;
    
    private enum Sighandler {
        ENTER_NOTIFY,
        LEAVE_NOTIFY,
        BUTTON_RELEASE,
        KEY_RELEASE,
        FOCUS_IN,
        FOCUS_OUT,
        NUM_SIGNALS
    }
    /* 6 == Sighandler.NUM_SIGNALS  vala stupidity. */
    private ulong handlers[6];
    

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
          

        update();
        /* Set our own attributes */
        label.set_attributes(attributes);

        /* pack label */
        this.add(label);
        
        /* keep track of cursor */
        this.enter_notify_event.connect( (source, event) => {
            this.mouseCursorInside = true;
            return false;
        });
        this.leave_notify_event.connect( (source, event) => {
            this.mouseCursorInside = false;
            return false;
        });
        
        /* Set widget sensitive */
        this.set_sensitive(true);

    }
    
    /**
     * Set sensitive
     */
    public new void set_sensitive(bool sensitive_state) {
        if (this.sensitive == sensitive_state) {
            return;
        }
        else if (sensitive_state == true){
            /**
             * Underline text when focus in
             */
            handlers[Sighandler.ENTER_NOTIFY] = this.enter_notify_event.connect(
                (source, event) => {
                    this.set_do_underline(true);
                    return false;
                });

            /**
             * Change back when focus out 
             */
            handlers[Sighandler.LEAVE_NOTIFY] = this.leave_notify_event.connect(
                (source, event) => {
                    this.set_do_underline(false);
                    return false;
                });

            handlers[Sighandler.BUTTON_RELEASE] = this.button_release_event.connect(
                (source, event) => {
                    if(event.button == 1) {
                        clicked((event.state&Gdk.ModifierType.MOD1_MASK) 
                                            == Gdk.ModifierType.MOD1_MASK);
                    }
                    return false;
                });

            handlers[Sighandler.KEY_RELEASE] = this.key_release_event.connect(
                (source, event) => {
                    if(event.keyval == 65293 /* enter */ ) {
                        clicked((event.state&Gdk.ModifierType.MOD1_MASK) 
                                            == Gdk.ModifierType.MOD1_MASK);
                    }
                    return false;
                });

            handlers[Sighandler.FOCUS_IN] = this.focus_in_event.connect(
                (source, event) => {
                    this.set_do_underline(true);
                    return false;
                });
            handlers[Sighandler.FOCUS_OUT] = this.focus_out_event.connect(
                (source, event) => {
                    this.set_do_underline(false);
                    return false;
                });
            
            /* Show underline, if mouse already inside */
            if (this.mouseCursorInside) {
                this.set_do_underline(true);
            }

            this.sensitive = true;
        }
        else {
            /**
             * disconnect all events
             */
            for (int i = 0; i < Sighandler.NUM_SIGNALS; i++)
                this.disconnect(handlers[i]);
                
            /* don't show underline if insensitive  */
            this.set_do_underline(false);
                
            this.sensitive = false;
        }
    }

    /**
     * Get sensitivity state
     */
    public new bool get_sensitive() {
        return this.sensitive;
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

    public void set_do_underline(bool val)
    {
        underline = val;
        update();
    }
    /**
     * Set do italic
     */
    public void set_do_italic(bool val)
    {
        italic = val;
        update();
    }

    public void set_do_bold(bool val)
    {
        bold = val;
        update();
    }

    public signal void clicked (bool alt = false);

    /**
     * Private functions
     */
     private void update()
     {
        /* TODO: Broken by vala */
        Pango.Attribute attr    = null;

        /* Set style  */
        /* italic */
        if(this.italic) {
            attr                = Pango.attr_style_new(Pango.Style.ITALIC); 
        }else {
            attr                = Pango.attr_style_new(Pango.Style.NORMAL); 
        }
        attr.start_index        = 0;
        attr.end_index          = -1;
        Gmpc.Fix.change(attributes,(owned)attr);

        /* bold */
        if(this.bold) {
            attr                = Pango.attr_weight_new(Pango.Weight.BOLD); 
        }else {
            attr                = Pango.attr_weight_new(Pango.Weight.NORMAL); 
        }
        attr.start_index        = 0;
        attr.end_index          = -1;
        Gmpc.Fix.change(attributes,(owned)attr);

        /* underline */
        if(this.underline) {
            attr                = Pango.attr_underline_new(Pango.Underline.SINGLE); 
        }else {
            attr                = Pango.attr_underline_new(Pango.Underline.NONE); 
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
        label.set_attributes(attributes);
     }
}
