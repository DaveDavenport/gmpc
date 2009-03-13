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
using Config;
using GLib;
using Gtk;
using Gdk;
using Cairo;
using MPD;
using Gmpc;

public class Gmpc.Easy.Command:Gmpc.PluginBase{
	/* hack to make gettext happy */
    private const string some_unique_name = Config.VERSION;

	private Gtk.EntryCompletion completion = null;
	private Gtk.ListStore store = null;
	private uint signals = 0;
	private Gtk.Window window = null;

	/***
	 * plugin setup
	 */
	private int[3] version = {0,0,1};


	/**
	 * Required plugin implementation
	 */
	 public override weak string get_name() 
	 {
		return _("Gmpc Easy Command");
	 }
	 public override weak int[] get_version()
	 {
		return this.version;
	 }
    /**
     * Tells the plugin to save itself
     */
    public override void save_yourself()
    {
        stdout.printf("Vala plugin save myself\n");
        /* nothing to save */
    }
    /**
     * Get set enabled
     */
    public override bool get_enabled() {
        return (bool)config.get_int_with_default(this.get_name(), "enabled", 1);
    }
    public override void set_enabled(bool state) {
		/* if disabling and popup is open, close it */
		if(!state && this.window != null) {
			this.window.destroy();
			this.window = null;
		}
       config.set_int(this.get_name(), "enabled", (int)state); 
    }

	/************************************************
	 * private
	 */
	private bool completion_function(Gtk.EntryCompletion comp, string key, Gtk.TreeIter iter) {
		string value;
		var model = comp.model;

		 model.get(iter, 1, out value);
		if (value != null) {
			string a = "^%s.*".printf(key);
			 return GLib.Regex.match_simple(a, value, GLib.RegexCompileFlags.CASELESS, 0);
		}

		return false;
	}
	construct {
		this.store =
			new Gtk.ListStore(6, typeof(uint), typeof(string), typeof(string), typeof(void *), typeof(void *),
							  typeof(string));
		this.completion = new Gtk.EntryCompletion();
		this.completion.model = this.store;
		this.completion.text_column = 1;
		this.completion.inline_completion = true;
		this.completion.inline_selection = true;
		this.completion.popup_completion = true;

		this.completion.set_match_func(completion_function, null);

		var renderer = new Gtk.CellRendererText();
		this.completion.pack_end(renderer, false);
		this.completion.add_attribute(renderer, "text", 5);
		renderer.set("foreground", "grey", null);
	}

	/**
     * This function is called when the user entered a line matching this entry.
     * param data the user data passed.
     * param param a string with the extra parameters passed to the command
     */
	public delegate void Callback(void *data, string param);

	/**
     * Add a match entry to the Easy command object.
     * param self the GmpcEasyCommand object.
     * param name the name of the command. This is the "prefix" that needs to be matched.
     * param pattern the pattern where the parameters need to match.
     * param callback a GmpcEasyCommandCallback that returns when a entry is matched.
     * param userdata a pointer that is passed to callback.
     *
     * return an unique id for the entry.
     */
	public uint add_entry(string name, string pattern, string hint, Callback * callback, void *userdata) {
		Gtk.TreeIter iter;
		this.signals++;
		this.store.append(out iter);
		this.store.set(iter, 0, this.signals, 1, name, 2, pattern, 3, callback, 4, userdata, 5, hint, -1);
		return this.signals;
	}

	private void activate(Gtk.Entry entry) {
		weak Gtk.TreeModel model = this.store;
		string value_unsplit = entry.get_text();
		Gtk.TreeIter iter;
		if (value_unsplit.length == 0) {
			this.window.destroy();
			this.window = null;
			return;
		}
		foreach(string value in value_unsplit.split(";")) {
            bool found = false;
            /* ToDo: Make this nicer... maybe some fancy parsing */
			if (model.get_iter_first(out iter)) {
				do {
					string name, pattern, test;
					Callback callback = null;
					void *data;
					model.get(iter, 1, out name, 2, out pattern, 3, out callback, 4, out data);

					test = "%s[ ]*%s$".printf(name, pattern);
					if (GLib.Regex.match_simple(test, value.strip(), GLib.RegexCompileFlags.CASELESS, 0)) {
						string param;
						stdout.printf("matched: %s to %s\n", test, value.strip());
						if (value.length > name.length)
							param = value.substring(name.length, -1);
						else
							param = "";
						var param_str = param.strip();
						callback(data, param_str);
						found = true;
					}
				} while (model.iter_next(ref iter) && !found);
			}
            /* If now exact match is found, use the partial matching that is
             * also used by the completion popup.
             * First, partial, match is taken.
             */
            if(!found) {
                if (model.get_iter_first(out iter)) {
                    do {
                        string name, pattern, test;
                        Callback callback = null;
                        void *data;
                        model.get(iter, 1, out name, 2, out pattern, 3, out callback, 4, out data);

                        test = "^%s.*".printf(value.strip());
                        if (GLib.Regex.match_simple(test, name,GLib.RegexCompileFlags.CASELESS, 0)) {
                            string param;
                            stdout.printf("matched: %s to %s\n", test, name);
                            if (value.length > name.length)
                                param = value.substring(name.length, -1);
                            else
                                param = "";
                            var param_str = param.strip();
                            callback(data, param_str);
                            found = true;
                        }
                        else
                            stdout.printf("!matched: %s to %s\n", test, name);
                    } while (model.iter_next(ref iter) && !found);
                }
            }
            /* If we still cannot match it, give a message */
            if (!found)
                Gmpc.Messages.show("Unknown command: '%s'".printf(value.strip()), Gmpc.Messages.Level.INFO);
		}

		window.destroy();
		window = null;
	}
	private bool key_press_event(Gtk.Entry widget, Gdk.EventKey event) {
		/* Escape */
		if (event.keyval == 0xff1b) {
			this.window.destroy();
			this.window = null;
			return true;
		}
		/* Tab key */
		else if (event.keyval == 0xff09) {
			((Gtk.Editable) widget).set_position(-1);
			return true;
		}
		return false;
	}

	private bool popup_expose_handler(Gtk.Window widget, Gdk.EventExpose event) {
		var ctx = Gdk.cairo_create(widget.window);
		int width = widget.allocation.width;
		int height = widget.allocation.height;

		if (widget.is_composited()) {
			ctx.set_operator(Cairo.Operator.SOURCE);
			ctx.set_source_rgba(1.0, 1.0, 1.0, 0.0);
		} else {
			ctx.set_source_rgb(1.0, 1.0, 1.0);
		}

		ctx.paint();
		/* */

		ctx.rectangle(1.0, 1.0, width - 2, height - 2);
		var pattern = new Cairo.Pattern.linear(0.0, 0.0, 0.0, height);

		pattern.add_color_stop_rgba(0.0, 0.0, 0.0, 0.2, 0.5);
		pattern.add_color_stop_rgba(0.5, 0.0, 0.0, 0.0, 1.0);
		pattern.add_color_stop_rgba(1.0, 0.0, 0.0, 0.2, 0.5);
		ctx.set_source(pattern);
		ctx.fill_preserve();
		ctx.set_source_rgba(1.0, 1.0, 1.0, 1.0);
		ctx.stroke();

		ctx.rectangle(0.0, 0.0, width, height);
		ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
		ctx.stroke();

		return false;
	}

	/** 
     * Tell gmpc-easy-command to popup.
     * @param self The GmpcEasyCommand object to popup
     *
     * This function will popup GmpcEasyCommand, or if allready open, preset it to the user.
     */
	public void
	 popup() {
		/* if not enabled, don't popup */
		if(!this.get_enabled()) return;

		if (this.window == null) {
			this.window = new Gtk.Window(Gtk.WindowType.TOPLEVEL);
			var entry = new Gtk.Entry();

			/* Setup window */
			window.role = "easy command";
			window.type_hint = Gdk.WindowTypeHint.DIALOG;
			window.decorated = false;
			window.modal = true;
			window.set_keep_above(true);

			window.border_width = 24;
			entry.width_chars = 50;

			window.add(entry);

			/* Composite */
			if (window.is_composited()) {
				var screen = window.get_screen();
				var colormap = screen.get_rgba_colormap();
				window.set_colormap(colormap);
			}
			window.app_paintable = true;
			window.expose_event += popup_expose_handler;

			if (!Gmpc.Playlist.is_hidden()) {
				window.set_transient_for(Gmpc.Playlist.get_window());
				window.position = Gtk.WindowPosition.CENTER_ON_PARENT;
			}

			/* setup entry */
			entry.set_completion(this.completion);
			entry.activate += this.activate;
			entry.key_press_event += this.key_press_event;

			window.show_all();
			window.present();
			entry.grab_focus();
		} else {
			this.window.present();
		}
	}
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
