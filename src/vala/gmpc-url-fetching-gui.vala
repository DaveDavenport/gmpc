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
using Gtk;
using Gdk;
using Module;


private const bool use_transition_ufg = Gmpc.use_transition;
namespace Gmpc.UrlFetching
{
	public class Gui : GLib.Object 
	{
		private Gtk.Builder builder = new Gtk.Builder();
		private ParseUrl parse_callback = null;
		private ValidateUrl validate_callback = null;
		private GLib.DestroyNotify destroy_cb;

		private enum State {
			NORMAL,
			PROCESSING,
			ERROR,
			DONE
		}

		/* Called when an url needs to be validated.
		 * returns bool is successfull
		 */
		public delegate bool ValidateUrl(Gui gui, string url);

		/* Function you needs to parse setting */
		public delegate void ParseUrl (Gui gui, string url); 

		private void add_url_dialog_response( int response_id)
		{
			if(response_id == 1) {
				weak Gtk.Entry entry = (Gtk.Entry)this.builder.get_object("url_entry");
				string url = entry.get_text();
				this.parse_callback(this, url);
				return;
			}
			stdout.printf("destroy callback\n");
			this.destroy_cb(this);
		}
		private void url_entry_changed(Gtk.Editable editable)
		{
			var add_button = (Gtk.Button) builder.get_object("add_button");
			var text = ((Gtk.Entry)editable).get_text();
			if(text != null && this.validate_callback != null && this.validate_callback(this, text)) {
				add_button.sensitive = true;
			}else{
				add_button.sensitive = false;
			}
		}

		~Gui ()
		{
			stdout.printf("~Gui\n");
			if(builder != null)
			{
				var dialog = (Gtk.Dialog) builder.get_object("add_url_dialog");
				if(dialog != null) {
					dialog.destroy();
				}
			}
		}

		public Gui (ParseUrl parse_callback, ValidateUrl validate_callback, GLib.DestroyNotify destroy_cb)
		{
			this.parse_callback = parse_callback;
			this.validate_callback = validate_callback;
			this.destroy_cb = destroy_cb;

			try {
				this.builder.add_from_file(Gmpc.data_path("gmpc-add-url.ui"));
			}catch (GLib.Error e)
			{
				GLib.error("Failed to load GtkBuilder file: %s", e.message);
			}
			var dialog = (Gtk.Dialog) builder.get_object("add_url_dialog");
			dialog.set_transient_for(Gmpc.Playlist.get_window());
			dialog.show();

			var entry = (Gtk.Entry) builder.get_object("url_entry");
			/* Connect by hand as connect_signals fails utterly */
			dialog.response.connect(add_url_dialog_response);
			entry.changed.connect(url_entry_changed);

		}

		private State state_counter = State.NORMAL;


		private void sensitive(bool state)
		{
			if(this.builder == null) return;
			var entry = (Gtk.Entry) builder.get_object("url_entry");
			entry.sensitive = state;

			var add_button = (Gtk.Button) builder.get_object("add_button");
			add_button.sensitive = state;

			var close_button = (Gtk.Button) builder.get_object("close_button");
			close_button.sensitive = state;

			var progress = (Gtk.ProgressBar) builder.get_object("url_progress");
			if(!state){
				progress.show();
			}
			else
				progress.hide();
		}
		/* Tell the dialog that we started processing stuff.
		 * This should make the window insensitive */
		public void set_processing()
		{
			this.state_counter = State.PROCESSING;

			sensitive(false);
		}

		/* Set progress 
		 * This can only be set after set_processing
		 * double -1 is a pulse.
		 */
		public void set_progress(double progress) 
		{
			GLib.log("GUFG", GLib.LogLevelFlags.LEVEL_DEBUG, "Set progress: %f", progress);
			if(this.state_counter != State.PROCESSING) return;


			var progressw = (Gtk.ProgressBar) builder.get_object("url_progress");
			if(progress < 0) progressw.pulse();
			else progressw.set_fraction(progress);


		}		
		/* Tell the dialog dialog we successfully parsed the pls. */
		public void set_completed()
		{
			GLib.log("GUFG", GLib.LogLevelFlags.LEVEL_DEBUG, "Completed");
			this.state_counter = State.DONE;

			sensitive(true);
			/* for now, destroy the window */
			this.destroy_cb(this);
		}
		/* Set error, this also undo's set_processing*/
		public void set_error(string error_message)
		{
			GLib.log("GUFG", GLib.LogLevelFlags.LEVEL_DEBUG, "Error: %s", error_message);
			this.state_counter = State.ERROR;

			sensitive(true);
		}
	}
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
