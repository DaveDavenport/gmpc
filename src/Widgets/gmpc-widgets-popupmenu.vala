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


public class PopupMenu : Gtk.Window
{
    private int x=0;
    private int y=0; 
    private int width = 0;
    private int height = 0;
    construct{
        type = Gtk.WindowType.POPUP;
        // this looks ignored.
        //set_gravity(Gdk.Gravity.CENTER);
        no_show_all = true;
        resizable =false;

        this.leave_notify_event.connect((source, event) => {
            this.destroy();
        });
        this.size_allocate.connect(sig_realize);
    }

    private void sig_realize(Gdk.Rectangle alloc)
    {
        this.width = alloc.width;
        this.height = alloc.height;
    }

    private void move_it()
    {
        this.move(x-width/2, y-height/2);
    }

    public void popup(Gdk.EventButton event)
    {
        x = (int)event.x_root;
        y =(int) event.y_root;
        show();
        move_it();
    }
}
