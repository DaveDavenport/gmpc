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
using GLib;
using Gtk;
using Gmpc;

namespace Gmpc
{
    public class ExportHTML : Gmpc.Plugin.Base, Gmpc.Plugin.ToolMenuIface
    {
        private const int[] version = {0,0,0};
        public override unowned int[] get_version()
        {
            return this.version;
        }

        public override unowned string get_name ()
        {
            return "Export HTML";
        }

        private void export()
        {



        }

        public int tool_menu_integration(Gtk.Menu menu)
        {
            var item = new Gtk.ImageMenuItem.with_label("Export Database HTML");

            menu.append(item);
            return 1;
        }
    }
}
