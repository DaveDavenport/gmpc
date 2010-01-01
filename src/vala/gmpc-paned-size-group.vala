/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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

using GLib;
using MPD;


namespace Gmpc{
	public class PanedSizeGroup : GLib.Object {
		private List<weak Gtk.Paned> list = null;
        private int position = config.get_int_with_default("paned-size-group", "position", 150);

		public 
		PanedSizeGroup () {

		}
		~PanedSizeGroup () {
            config.set_int("paned-size-group", "position", position);

		}
		private bool 
		child_destroy_event(Gtk.Widget paned, Gdk.Event event)
		{
			list.remove((Gtk.Paned)paned);

			return false;
		}
		private bool block_changed_callback = false;
		private
		void
		child_position_changed(GLib.Object paned, ParamSpec spec)
		{
			if(block_changed_callback) return;
			block_changed_callback = true;

			var pane = (Gtk.Paned) paned;
			position = pane.get_position();
			foreach(weak Gtk.Paned p in list)
			{
				if(p != paned)
				{
					p.set_position(position);
				}
			}

			block_changed_callback = false;
		}
		public
		void
		add_paned(Gtk.Paned paned)
		{
			paned.notify["position"].connect(child_position_changed);
			paned.destroy_event.connect(child_destroy_event);

			block_changed_callback = true;
            paned.set_position(position);
			block_changed_callback = false;

			list.append(paned);
		}
	} 
}
