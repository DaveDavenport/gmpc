using Gtk;
using FixGtk;

namespace Gmpc
{
	namespace Tools
	{
		public class BindingOverlayNotify : GLib.Object, FixGtk.Buildable
		{
			private Gdk.ModifierType keys = 0;
			public string name {get; set; default="";}
			// Buildable stuff
			public void set_name(string name)
			{
				this.name= name;
			}
			public unowned string get_name()
			{
				return this.name;
			}
			public void key_pressed(Gdk.ModifierType key)
			{
				keys |= key;
				state_changed(keys);
			}
			public void key_released(Gdk.ModifierType key)
			{

				keys &= ~key;
				state_changed(keys);
			}

			public signal void state_changed(Gdk.ModifierType cur_state);
		}


	}
	namespace Widgets
	{
		public class Overlay : Gtk.EventBox, Gtk.Buildable
		{
			public Gdk.ModifierType modifier {get; set; default = Gdk.ModifierType.MOD1_MASK;}
			public bool show_overlay {get; set; default = false;}
			public string overlay_text {get; set; default= "left"; }
			construct{
				this.set_app_paintable(true);
			}
			private Tools.BindingOverlayNotify notifier = null;
			public Tools.BindingOverlayNotify binding_overlay_notifier {
				get{
					return notifier;
				}
				set {
					if(notifier != null) {
						notifier.state_changed.disconnect(key_changed);
					}
					notifier = value;
					notifier.state_changed.connect(key_changed);
				}
			}

			private void key_changed(Gdk.ModifierType cur_state)
			{
				if((cur_state) == modifier) {
					show_overlay = true;
				}else{
					show_overlay = false;
				}
				this.queue_draw();
			}

			public Overlay()
			{
			}

			public override bool expose_event(Gdk.EventExpose event)
			{
				// Draw the child of this box.
				var w = (this as Gtk.Bin).get_child();
				w.expose_event(event);
				// 
				if(show_overlay)
				{
					var ct = Gdk.cairo_create(window);
					int pw,ph;
					var l = this.create_pango_layout(overlay_text);
					Gtk.Allocation cell_area = this.allocation;
					l.get_pixel_size(out pw, out ph);

					ct.set_source_rgb(0.8,0,0);
//					Gdk.cairo_set_source_color(ct, this.style.bg[Gtk.StateType.SELECTED]);
					ct.rectangle(1, cell_area.height/2-ph/2-0.5, pw+4, ph);
					ct.fill_preserve();
					Gdk.cairo_set_source_color(ct, this.style.text[Gtk.StateType.SELECTED]);
					ct.stroke();

					ct.move_to(3, cell_area.height/2-ph/2-1.5);
					Gdk.cairo_set_source_color(ct,this.style.fg[Gtk.StateType.SELECTED]);
					Pango.cairo_show_layout(ct, l);
					ct.stroke();
				}
				return false;
			}

		}
	}
}
