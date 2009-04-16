/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
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

/**
 * Widget that shows a pixbuf by nicely fadeing in and out.
 * Draws a nice border.
 */

using GLib;
using Gtk;
using Cairo;

public class Gmpc.Image:Gtk.EventBox {
	private Gdk.Pixbuf cover = null;
	private bool cover_border = true;
	private Gdk.Pixbuf temp = null;
	private bool temp_border = true;
	private double fade = 0.0;
	private uint fade_timeout = 0;
	public string? text {get; set;} 
	private Pango.FontDescription fd = null; 
	~Image() {
		if (fade_timeout > 0) {
			GLib.Source.remove(fade_timeout);
			fade_timeout = 0;
		}
	}
	construct {
		this.app_paintable = true;
		this.visible_window = false;
		this.expose_event += this.on_expose;
		this.fd = new Pango.FontDescription();//from_string("sans mono"); 
		fd.set_family("sans mono");

	}
	private bool on_expose(Image img, Gdk.EventExpose event) {
		var ctx = Gdk.cairo_create(img.window);
		int width = 0;
		int height = 0;
		int x = img.allocation.x;
		int y = img.allocation.y;
		int ww = img.allocation.width;
		int wh = img.allocation.height;

		ctx.set_antialias(Cairo.Antialias.NONE);
		ctx.rectangle(event.area.x, event.area.y, event.area.width, event.area.height);
		ctx.clip();
		ctx.save();

		ctx.set_line_width(1.0);
		ctx.set_tolerance(0.0);
		if (cover != null) {
			width = cover.get_width();
			height = cover.get_height();
			var x_start = x+Math.ceil((ww-width)/2.0);
			var y_start = y+Math.ceil((wh-height)/2.0);
			ctx.set_line_join(LineJoin.ROUND);
			// Make the path
			ctx.new_path();
			ctx.rectangle(x_start, y_start, width-1, height-1);

			double fade2 = (fade <= 0) ? 1 : fade;
			Gdk.cairo_set_source_pixbuf(ctx, cover, x_start, y_start);
/*
			if (cover_border)
				ctx.clip_preserve();
			else
				ctx.clip();
*/			ctx.paint_with_alpha(fade2);
			if (cover_border) {
				ctx.set_source_rgba(0, 0, 0, fade2);
				ctx.stroke();
			}
			else ctx.new_path();
/*			ctx.reset_clip();
			ctx.restore();
*/		}

		if (temp != null) {
			ctx.new_path();
			width = temp.get_width();
			height = temp.get_height();
			var x_start = x+Math.ceil((ww-width)/2.0);
			var y_start = y+Math.ceil((wh-height)/2.0);
			ctx.set_line_join(LineJoin.ROUND);
			ctx.rectangle(x_start,y_start, width-1, height-1);
			Gdk.cairo_set_source_pixbuf(ctx, temp, x_start,y_start);
/*
			if (temp_border)
				ctx.clip_preserve();
			else
				ctx.clip();
*/
			double fade2 = (fade <= 0) ? 1 : fade;
			ctx.paint_with_alpha(1 - fade2);
			if (temp_border) {
				ctx.set_source_rgba(0, 0, 0, 1 - fade2);
				ctx.stroke();
			}
			else ctx.new_path();
//			ctx.reset_clip();
		}

		if (this.cover != null && this.text != null && this.text.length > 0){
			var layout = Pango.cairo_create_layout(ctx);
			int tw, th;

			ctx.set_antialias(Cairo.Antialias.DEFAULT);
			int size = (cover.width)/(int)this.text.length;
			stdout.printf("%i-%i-%i\n", size, ww,(int)this.text.length);
			fd.set_absolute_size(size*1024);
			layout.set_font_description(fd);
			layout.set_text(this.text,-1);
			layout.get_pixel_size(out tw, out th);

			ctx.move_to(x+(ww-tw)/2.0, y+(wh-th)/2.0);

			Pango.cairo_layout_path(ctx,layout);

			ctx.set_source_rgba(0, 0, 0, 1);
			ctx.stroke_preserve();
			ctx.set_source_rgba(1, 1, 1, 1);
			ctx.fill();
		}
		return false;
	}
	private bool timeout_test() {
		fade -= 0.10;
		if (fade <= 0.0) {
			fade = 0;
			this.cover = this.temp;
			this.cover_border = this.temp_border;
			this.temp = null;
			this.queue_draw();
			fade_timeout = 0;
			return false;
		}

		this.queue_draw();
		return true;
	}
	/**
     * Set a new pixbuf to be displayed next.
     * param self a GmpcImage to set the pixbuf on.
     * param bug the new GdkPixbuf to display.
     * param border flag that indicates if a border should be drawn.
     *
     * Queues the pixbuf buf to be drawn next. If a pixbuf is allready shown, it will fade out and buf 
     * will fade in.
     */
	public void set_pixbuf(Gdk.Pixbuf buf, bool border) {
		if (this.temp == null && this.cover == null) {
			this.cover_border = border;
			this.cover = null;
			this.cover = buf;
			this.queue_draw();
			return;
		}
		fade = 1.0 - fade;
		if (this.temp != null) {
			this.cover = this.temp;
		}
		this.temp = null;
		this.temp = buf;
		this.temp_border = border;
		this.queue_draw();
		if (fade_timeout > 0) {
			GLib.Source.remove(fade_timeout);
		}
		fade_timeout = Timeout.add(50, this.timeout_test);
	}
	/**
     * Clears the image.
     * param self a GmpcImage to clear
     * 
     * Clears the image. Next set_pixbuf won't cause a fade.
     */
	public void clear_pixbuf() {
		fade = 0.0;
		this.temp = null;
		if (fade_timeout > 0) {
			GLib.Source.remove(fade_timeout);
			fade_timeout = 0;
		}
		this.cover = null;
		this.cover_border = false;
		this.queue_draw();
	}
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
