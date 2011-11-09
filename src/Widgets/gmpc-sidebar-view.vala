
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

using Config;
using Gtk;
using Gmpc;
using Cairo;

private const bool use_transition_sbv = Gmpc.use_transition;
private const string some_unique_name_sbv = Config.VERSION;

public class MyCellRenderer : Gtk.CellRenderer
{
	public bool show_number { set; get; default=false;} 
	public int number { set; get; default=0;}
	private CellRendererPixbuf cr_pb = new Gtk.CellRendererPixbuf();
	private CellRendererText cr_text = new Gtk.CellRendererText();
	public string icon_name {
			set{
				cr_pb.icon_name = value;
			}
	}

	public int weight { set { cr_text.weight = value;}}
	public bool weight_set { set { cr_text.weight_set = value;}}

	public uint stock_size { 
		get {
			return cr_pb.stock_size;
		} 
		set {
			cr_pb.stock_size = value;
		}
	}
	public string text { 
			set {
				cr_text.text = value;
			}
	} 
	public bool show_text
	{
			set;
			get;
			default = true;
	}
	public int image_width {
			set;
			get;
			default = 16;
	}
	public new float xalign {
		set{
			cr_pb.xalign = value;
		}
	}
	/* dumb constructor */
	public MyCellRenderer () {}

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
	public override void render (Gdk.Window    window,
			Gtk.Widget    widget,
			Gdk.Rectangle background_area,
			Gdk.Rectangle cell_area,
			Gdk.Rectangle expose_area,
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
		ca.width = image_width+6;//cell_area.height;
		ca.height = cell_area.height;
		
		if(cr_pb.icon_name != null )
		{
			cr_pb.render(window, widget, background_area, ca, expose_area, flags);

			ca.x+=6+image_width;
			ca.width-=6+image_width;
		}
		if(show_text)
			cr_text.render(window, widget, background_area, ca, expose_area, flags);

		if(show_number)
		{
			int pw,ph;
			var l = widget.create_pango_layout("%0i".printf(number));
			var ct = Gdk.cairo_create(window);
			l.get_pixel_size(out pw, out ph);

//			Gdk.cairo_set_source_color(ct, widget.style.bg[Gtk.StateType.SELECTED]);
			ct.set_source_rgb(0.8,0,0);
			ct.rectangle(cell_area.x, cell_area.y+cell_area.height/2-ph/2-0.5, pw+4, ph);
			ct.fill_preserve();
			Gdk.cairo_set_source_color(ct, widget.style.text[Gtk.StateType.SELECTED]);
			ct.stroke();

			ct.move_to(cell_area.x+2, cell_area.y+cell_area.height/2-ph/2-1.5);
			Gdk.cairo_set_source_color(ct, widget.style.fg[Gtk.StateType.SELECTED]);
			Pango.cairo_show_layout(ct, l);
			ct.stroke();
		}
		return;
	}
}
