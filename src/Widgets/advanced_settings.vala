/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2011-2011 Qball Cow <qball@gmpclient.org>
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

private const bool use_transition_as = Gmpc.use_transition;
private const string some_unique_name_as = Config.VERSION;
/**
 * Show a dialog with some advanced interface settings 
 */
public static void advanced_settings()
{
	var win = new Gtk.Dialog.with_buttons(
			_("Advanced settings"),
			null,
            0,	
            "gtk-close", Gtk.ResponseType.CLOSE);
	
	/* Settings */
	var vbox = new Gmpc.Widgets.Qtable();
    win.set_size_request(450, -1);
    vbox.header_height = 32;
	win.vbox.add(vbox);
	/* TODO: Warning */

	vbox.border_width=8;
	/* Interface */	
	var label = new Gtk.Label(_("Interface"));
    label.set_ellipsize(Pango.EllipsizeMode.END);
	label.set_markup(GLib.Markup.printf_escaped("<b>%s</b>", _("Interface")));
	label.set_alignment(0.0f, 0.5f);
//	vbox.pack_start(label, false, false, 0);
    vbox.add_header(label);
	/* Album art */
	var ck = new Gtk.CheckButton.with_label("Hide album art");	
	ck.set_active((bool)config.get_int_with_default("Interface", "hide-album-art", 0));
	ck.toggled.connect((source) => {
		config.set_int("Interface", "hide-album-art",(int)source.get_active());
			});
//	vbox.pack_start(ck, false, false, 0);
    vbox.add(ck);
	/* Favorites icon */
	ck = new Gtk.CheckButton.with_label("Hide favorite icon");	
	ck.set_active((bool)config.get_int_with_default("Interface", "hide-favorites-icon", 0));
	ck.toggled.connect((source) => {
		config.set_int("Interface", "hide-favorites-icon",(int)source.get_active());
			});
	//vbox.pack_start(ck, false, false, 0);
    vbox.add(ck);
    /** use backdrops  */
	ck = new Gtk.CheckButton.with_label("Use backdrops (restart required)");
	ck.set_active((bool)config.get_int_with_default("Now Playing", "use-backdrop", 0));
	ck.toggled.connect((source) => {
		config.set_int("Now Playing", "use-backdrop",(int)source.get_active());
        });
    vbox.add(ck);

    /** Use legacy tray icon  */
    if (Gmpc.TrayIcon2.have_appindicator_support()) {
	    ck = new Gtk.CheckButton.with_label("Use legacy tray icon");
        ck.set_active(!(bool)config.get_int_with_default("tray-icon2", "use_appindicator", 1));
	    ck.toggled.connect((source) => {
            Gmpc.TrayIcon2.toggle_use_appindicator();
        });
        vbox.add(ck);
    }
	/* Browsers */
	label = new Gtk.Label(_("Browsers"));
    label.set_ellipsize(Pango.EllipsizeMode.END);
	label.set_markup(GLib.Markup.printf_escaped("<b>%s</b>", _("Browsers")));
	label.set_alignment(0.0f, 0.5f);
//	vbox.pack_start(label, false, false, 0);
    vbox.add_header(label);	

	for(int i =0; i< Gmpc.num_plugins;i++) {
		unowned Gmpc.parentPlugin p = Gmpc.plugins[i];
		if(p.is_browser() && p.has_enabled())
		{
			ck = new Gtk.CheckButton.with_label(p.get_name());	
			ck.set_active((bool)p.get_enabled());
			ck.toggled.connect((source) => {
					p.set_enabled((int)source.get_active());
                    Gmpc.Playlist3.update_go_menu();
                    Gmpc.Preferences.update();
					});
	//		vbox.pack_start(ck, false, false, 0);
            vbox.add(ck);
        }
	}
	win.show_all();
	win.run();
	win.destroy();
	return;
}
