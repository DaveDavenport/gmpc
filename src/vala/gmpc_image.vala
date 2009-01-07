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

/**
 * Widget that shows a pixbuf by nicely fadeing in and out.
 * Draws a nice border.
 */

using GLib;
using Gtk;
using Cairo;

public class Gmpc.Image : Gtk.EventBox {
	private Gdk.Pixbuf cover = null;
	private bool cover_border = true;
	private Gdk.Pixbuf temp = null;
	private bool temp_border = true;
	private double fade = 0.0;
	private uint fade_timeout = 0;

	~Image() {
		if(fade_timeout > 0)
		{
			GLib.Source.remove(fade_timeout);
			fade_timeout = 0;
		}
	}
	construct {
		this.app_paintable = true;
		this.visible_window = false;
		this.expose_event += this.on_expose;
	}
		private bool on_expose (Image img, Gdk.EventExpose event) {
			var ctx = Gdk.cairo_create(img.window);
			int width=0;
			int height = 0;
			int x = img.allocation.x;
			int y = img.allocation.y;
			int ww = img.allocation.width;
			int wh = img.allocation.height;
            ctx.set_line_width ( 0.8);
			ctx.set_tolerance (0.1);
			if(cover != null)
			{
				width = cover.get_width();
				height = cover.get_height();

				ctx.set_line_join (LineJoin.ROUND);

				// Make the path
				ctx.new_path();
                ctx.rectangle( x+(ww-width)/2+0.5,y+(wh-height)/2+0.5,width-1, height-1);

                double fade2 = (fade <= 0)?1:fade;
				Gdk.cairo_set_source_pixbuf(ctx, cover, x+(ww-width)/2,y+(wh-height)/2);

				if(cover_border)
					ctx.clip_preserve();
				else
					ctx.clip();
				ctx.paint_with_alpha(fade2);
				ctx.reset_clip();
				if(cover_border){
					ctx.set_source_rgba(0,0,0,fade2);
					ctx.stroke();
				}
			}


			if(temp != null)
			{
				ctx.new_path();
				width = temp.get_width();
				height = temp.get_height();

                ctx.rectangle( x+(ww-width)/2+0.5,y+(wh-height)/2+0.5,width-1, height-1);
                Gdk.cairo_set_source_pixbuf(ctx, temp, x+(ww-width)/2,y+(wh-height)/2);


				if(temp_border)
					ctx.clip_preserve();
				else
					ctx.clip();
				ctx.paint_with_alpha(1-fade);
				ctx.reset_clip();
				if(temp_border){
					ctx.set_source_rgba(0,0,0,1-fade);
					ctx.stroke();
				}
			}
			return true;
		}
		private bool timeout_test()
		{
			fade -= 0.10;
			if(fade <= 0.0){
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
		public void set_pixbuf(Gdk.Pixbuf buf, bool border)
		{
            if(this.temp == null && this.cover == null) {
                this.cover_border = border;
                this.cover = buf;
                this.queue_draw();
                return;
            }
			fade = 1.0;
			this.temp= buf;
			this.temp_border = border;
                this.queue_draw();
			if(fade_timeout>0) {
				GLib.Source.remove(fade_timeout);
			}
			fade_timeout = Timeout.add(50, this.timeout_test);
		}
		public void clear_pixbuf()
		{
			fade = 0.0;
			this.temp = null;
			if(fade_timeout>0) {
				GLib.Source.remove(fade_timeout);
				fade_timeout = 0;
			}
			this.cover = null;
			this.cover_border = false;
			this.queue_draw();
		}
	}
