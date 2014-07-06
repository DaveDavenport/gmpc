
/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
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

using Config;
using Gtk;
using Gmpc;
using Cairo;

private const bool use_transition_sbv = Gmpc.use_transition;
private const string some_unique_name_sbv = Config.VERSION;

public class MyCellRenderer : Gtk.CellRenderer
{
    private CellRendererPixbuf cr_pb = new Gtk.CellRendererPixbuf();
    private CellRendererText cr_text = new Gtk.CellRendererText();
    public string icon_name
    {
        set{
            cr_pb.icon_name = value;
        }
    }
    public string stock_id
    {
        set{
            cr_pb.stock_id = value;
        }
    }

    public int weight { set { cr_text.weight = value;}}
    public bool weight_set { set { cr_text.weight_set = value;}}

    public uint stock_size
    {
        get {
            return cr_pb.stock_size;
        }
        set {
            cr_pb.stock_size = value;
        }
    }
    public string text
    {
        set {
            cr_text.text = value;
        }
    }

    private bool _show_text = true;
    public bool show_text
    {
        set {
            _show_text = value;
        }
        get{
            return _show_text;
        }
    }
    public int image_width
    {
        set;
        get;
        default = 16;
    }
    public new float xalign
    {
        set{
            cr_pb.xalign = value;
        }
    }
    /* dumb constructor */
    public MyCellRenderer () {}

    public override Gtk.SizeRequestMode get_request_mode()
    {
        return Gtk.SizeRequestMode.CONSTANT_SIZE;
    }

    public override void get_preferred_width(Gtk.Widget widget,
            out int min_size,
            out int nat_size)
    {

        int ms = 0, ns = 0;
        cr_pb.get_preferred_width(widget, out ms, out ns);
        if(_show_text)
        {
            int tms = 0, tns =0;
            cr_text.get_preferred_width(widget, out tms, out tns);
            ms+= tms+6;
            ns+= tns+6;
        }
        if(&min_size != null) min_size = ms;
        if(&nat_size != null) nat_size = ns;
    }

    /* get_size method, always request a 50x50 area */
    public override void get_size (Gtk.Widget widget,
            Gdk.Rectangle? cell_area,
            out int x_offset,
            out int y_offset,
            out int width,
            out int height)
    {
        int x_o=0;
        int y_o =0;
        int w=0;
        int h=0;
        int tx_o=0;
        int ty_o =0;
        int tw=0;
        int th=0;
        stdout.printf("get size\n");

        cr_pb.get_size(widget, null, out x_o, out y_o, out w, out h);
        if(show_text)
            cr_text.get_size(widget, null, out tx_o, out ty_o, out tw, out th);
        /* The bindings miss the nullable property, so we need to check if the
         * values are null.
         */
        if (&x_offset != null) x_offset = 0;
        if (&y_offset != null) y_offset = 0;
        if (&width != null) width = image_width+6+tw;
        if (&height != null) height = int.max(h, th);
        return;
    }

    /* render method */
    public override void render (Cairo.Context ct,
            Gtk.Widget    widget,
            Gdk.Rectangle background_area,
            Gdk.Rectangle cell_area,
            Gtk.CellRendererState flags)
    {
        int x_o=0;
        int y_o =0;
        int w=0;
        int h=0;
        cr_pb.get_size(widget, null, out x_o, out y_o, out w, out h);
        Gdk.Rectangle ca = Gdk.Rectangle();
        ca.x = cell_area.x;
        ca.y = cell_area.y;
        ca.width = w+6;//cell_area.height;
        ca.height = cell_area.height;

        if(cr_pb.icon_name != null || cr_pb.stock_id != null )
        {
            cr_pb.render(ct, widget, background_area, ca, flags);

            ca.x+=6+image_width;
            ca.width = cell_area.width-ca.width;
        }
        else
        {
            ca.width = cell_area.width;
        }
        if(_show_text)
            cr_text.render(ct, widget, background_area, ca, flags);

        // Draw arrow.
/*
        if((flags&Gtk.CellRendererState.SELECTED) == Gtk.CellRendererState.SELECTED)
        {
            double cax = background_area.width*0.9;
            double bld = background_area.width*0.1;
            if(!show_text) cax = background_area.x+image_width+3;

            ct.move_to(background_area.x+cax, background_area.y);
            ct.line_to(background_area.x+cax+bld, background_area.y+background_area.height/2);
            ct.line_to(background_area.x+cax, background_area.y+background_area.height);
            ct.line_to(background_area.x+cax+bld, background_area.y+background_area.height);
            ct.line_to(background_area.x+cax+bld, background_area.y);
            ct.line_to(background_area.x+cax, background_area.y);
            Gdk.cairo_set_source_color(ct, widget.style.bg[Gtk.StateType.NORMAL]);
            ct.fill();
        }*/
        return;
    }
}
