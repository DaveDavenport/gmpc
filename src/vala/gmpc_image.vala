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
	private void draw_curved_rectangle(Context ctx, double rect_x0, double rect_y0, double rect_width, double rect_height) {
		double rect_x1,rect_y1;
		double radius = 15;//rect_width/5;
		rect_x1=rect_x0+rect_width;
		rect_y1=rect_y0+rect_height;
		if (rect_width == 0 || rect_height == 0)
			return;
		if (rect_width/2<radius) {
			if (rect_height/2<radius) {
				ctx.move_to  (rect_x0, (rect_y0 + rect_y1)/2);
				ctx.curve_to (rect_x0 ,rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1)/2, rect_y0);
				ctx.curve_to (rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1)/2);
				ctx.curve_to (rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0)/2, rect_y1);
				ctx.curve_to (rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1)/2);
			} else {
				ctx.move_to  ( rect_x0, rect_y0 + radius);
				ctx.curve_to ( rect_x0 ,rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1)/2, rect_y0);
				ctx.curve_to ( rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
				ctx.line_to ( rect_x1 , rect_y1 - radius);
				ctx.curve_to ( rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0)/2, rect_y1);
				ctx.curve_to ( rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1- radius);
			}
		}
		else 
		{
			if (rect_height/2<radius) {
				ctx.move_to  ( rect_x0, (rect_y0 + rect_y1)/2);
				ctx.curve_to ( rect_x0 , rect_y0, rect_x0 , rect_y0, rect_x0 + radius, rect_y0);
				ctx.line_to ( rect_x1 - radius, rect_y0);
				ctx.curve_to ( rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1)/2);
				ctx.curve_to ( rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
				ctx.line_to ( rect_x0 + radius, rect_y1);
				ctx.curve_to ( rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1)/2);
			} else {
				ctx.move_to  ( rect_x0, rect_y0 + radius);
				ctx.curve_to ( rect_x0 , rect_y0, rect_x0 , rect_y0, rect_x0 + radius, rect_y0);
				ctx.line_to ( rect_x1 - radius, rect_y0);
				ctx.curve_to ( rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
				ctx.line_to ( rect_x1 , rect_y1 - radius);
				ctx.curve_to ( rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
				ctx.line_to ( rect_x0 + radius, rect_y1);
				ctx.curve_to ( rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1- radius);
			}
		}

		ctx.close_path();
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
				draw_curved_rectangle(ctx, x+(ww-width)/2+0.5,y+(wh-height)/2+0.5,width-2, height-2);

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

				draw_curved_rectangle(ctx, x+(ww-width)/2+0.5,y+(wh-height)/2+0.5,width-2, height-2);
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
