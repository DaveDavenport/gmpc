/* Gnome Music Player Client
 * Copyright (C) 2011 Qball Cow <qball@gmpclient.org>
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
using Gmpc;
using Gmpc.Plugin;
using Gmpc.Favorites;

private const bool use_transition_mb2 = Gmpc.use_transition;
private const string some_unique_name_mb2 = Config.VERSION;
private const string np2_LOG_DOMAIN = "NowPlaying";

/**
 * This plugin implements an 'improved' now playing interface, modeled after the mockup found here
 * http://gmpclient.org/wiki/GMPC_Mocup
 */

/* create teh plugin of type Gmpc.Plugin.Base that implements the Browser interface */
namespace Gmpc {

    namespace Browsers {
        public class Nowplaying : Gmpc.Plugin.Base, Gmpc.Plugin.BrowserIface {
            private bool    theme_colors    = (bool) config.get_int_with_default("Now Playing", "use-theme-color",1);
            private string  title_color     = config.get_string_with_default("Now Playing", "title-color", "#4d90dd");
            private string  item_color      = config.get_string_with_default("Now Playing", "item-color", "#304ab8");
            private Gdk.Color background;
            private Gdk.Color foreground;

            private Gtk.Label bitrate_label = null;
            private Gtk.TreeRowReference np_ref = null;

			private bool use_backdrop = (bool) config.get_int_with_default("Now Playing", "use-backdrop",0);


            construct {
                /* Set the plugin as Browser type*/
                this.plugin_type = 2|8;
                /* Track changed status */
                gmpcconn.status_changed.connect(status_changed);
                /* Track connect/disconnect */
                gmpcconn.connection_changed.connect((source, connect) => {
                        /* If disconnect update the page */
                        if(connect == 0 && this.paned != null)
                        this.update_not_playing();
                        });

                var background = config.get_string_with_default("Now Playing",
                        "background-color", "#000");
                var foreground = config.get_string_with_default("Now Playing",
                        "foreground-color", "#FFF");
                Gdk.Color.parse(background,out this.background);
                Gdk.Color.parse(foreground,out this.foreground);

				/* Register easycommand to switch to this browser */
				easy_command.add_entry(
                        _("switch now playing"),
                        "",
                        _("Switch to Now Playing"),
                        (Gmpc.Easy.Command.Callback *)select_now_playing_browser,
                        this);
            }
            /* Version of the plugin*/
            private const int[] version =  {0,0,0};
            public override  unowned int[] get_version() {
                return version;
            }
            /* Name */
            public override unowned string get_name() {
                return N_("Now Playing");
            }

            public override void set_enabled(bool state) {
                if(state) {
                    if(paned == null) {
                        browser_add( Gmpc.Playlist3.get_category_tree_view());
                        browser_init();
                    }
                }else {
                    if(this.np_ref != null) {
                        var path = np_ref.get_path();
                        if(path != null) {
                            unowned int[] indices  = path.get_indices();
                            config.set_int(this.get_name(), "position", indices[0]);
                            Gtk.ListStore model = (Gtk.ListStore) np_ref.get_model();
                            Gtk.TreeIter iter;
                            if(model.get_iter(out iter, path))
                            {
                                model.remove(iter);
                            }
                        }
                    }
                    if(this.paned != null) {
                        this.paned.destroy();
                        this.paned = null;
                        this.song_checksum = null;
                    }
                }

                if(this.get_name() != null)
                    Gmpc.config.set_int(this.get_name(), "enabled", (int)state);
            }
            /* Save our position in the side-bar */
            public override void save_yourself() {
                if(this.np_ref != null) {
                    var path = np_ref.get_path();
                    if(path != null) {
                        unowned int[] indices  = path.get_indices();
                        config.set_int(this.get_name(), "position", indices[0]);
                    }
                }
            }

            /* React MPD's status changed */
            private
                void
                status_changed(Gmpc.Connection conn, MPD.Server server, MPD.Status.Changed what)
                {
                    if(!this.get_enabled())return;
                    if(!this.selected) return;
                    /* If the state changes, update. */
                    if((what&MPD.Status.Changed.STATE) == MPD.Status.Changed.STATE)
                    {
                        this.update();
                    }
                    /* If the playlist changed, this might mean that the metadata of the currently playing song changed, update. */
                    else if ((what&(MPD.Status.Changed.SONGID|MPD.Status.Changed.PLAYLIST)) > 0)
                    {
                        this.update();
                    }
                    /* If the bitrate changed, update the bitrate label. */
                    if((what&(MPD.Status.Changed.BITRATE|MPD.Status.Changed.AUDIOFORMAT)) > 0)
                    {
                        if(bitrate_label != null)
                        {
                            var channels = MPD.Status.get_channels(Gmpc.server);
                            debug("bitrate changed");
                            var bitrate = MPD.Status.get_bitrate(Gmpc.server);
                            bitrate_label.set_markup(GLib.Markup.printf_escaped(
                                        "<span color='%s' weight='bold'>%s:</span> %i %s, %.1f %s, %i %s",
                                        this.item_color, _("Format"),
                                        channels , GLib.ngettext(N_("Channel"),N_("Channels"), channels),
                                        MPD.Status.get_samplerate(Gmpc.server)/1000.0, "kHz",
                                        bitrate, "kbps"
                                        ));
                        }
                    }

                }

            /* Browser */
            private Gtk.ScrolledWindow paned = null;
            private Gtk.EventBox container = null;
            private bool selected = false;
            /**
             * Browser Interface bindings
             */
            public void browser_add (Gtk.Widget category_tree)
            {
                Gtk.TreeView tree = (Gtk.TreeView)category_tree;
                Gtk.ListStore store = (Gtk.ListStore)tree.get_model();
                Gtk.TreeModel model = tree.get_model();
                Gtk.TreeIter iter;
                Gmpc.Browser.insert(out iter, config.get_int_with_default(this.get_name(), "position", 0));
                store.set(iter, 0, this.id, 1, this.get_name(), 3, "media-audiofile");
                /* Create a row reference */
                this.np_ref = new Gtk.TreeRowReference(model,  model.get_path(iter));
            }
            /* Called by gmpc, telling the plugin to embed itself in container */
            public void browser_selected (Gtk.Container container)
            {



                this.selected = true;
                this.browser_init();
                container.add(this.paned);

                container.show_all();
                container.ensure_style();
                if(this.theme_colors) {
                    this.title_color = this.paned.style.text[Gtk.StateType.PRELIGHT].to_string();
                    this.item_color = this.paned.style.text[Gtk.StateType.PRELIGHT].to_string();
                }
				if(use_backdrop) {
					this.title_color = "#fff";
					this.item_color = "#fff";
					this.theme_colors = false;
				}
                this.update();
            }

            /* Called by gmpc, telling the plugin to remove itself from the container */
            public void browser_unselected(Gtk.Container container)
            {
                this.selected = false;
                container.remove(this.paned);
            }

            /**
             * If the style changed because f.e. the user switched theme, make sure the correct colouring is kept preserved.
             */

            private void browser_bg_style_changed(Gtk.Widget bg,Gtk.Style? style)
            {
                debug("Change style signal");
                if(this.theme_colors) {
                    this.title_color = this.paned.style.text[Gtk.StateType.PRELIGHT].to_string();
                    this.item_color = this.paned.style.text[Gtk.StateType.PRELIGHT].to_string();
                }
                this.change_color_style(this.container);
            }
            /**
             * Recursively force  a style on widget bg and children.
             */
            private void change_color_style(Gtk.Widget bg)
            {
                debug("change style");
                if(bg is Gtk.Separator || bg is Gtk.Notebook || bg is Gtk.CheckButton){
                    /* Do nothing */
                }else{
                    if(theme_colors)
                    {
                        bg.modify_bg(Gtk.StateType.NORMAL,this.paned.style.base[Gtk.StateType.NORMAL]);
                        /*bg.modify_base(Gtk.StateType.NORMAL,this.paned.style.mid[Gtk.StateType.NORMAL]);/*
//                        bg.modify_text(Gtk.StateType.NORMAL,this.paned.style.text[Gtk.StateType.NORMAL]);
                        bg.modify_fg(Gtk.StateType.NORMAL,this.paned.style.text[Gtk.StateType.NORMAL]);
                       bg.modify_text(Gtk.StateType.ACTIVE,this.paned.style.light[Gtk.StateType.NORMAL]);
                        bg.modify_fg(Gtk.StateType.ACTIVE,this.paned.style.light[Gtk.StateType.NORMAL]);*/
                    }else{
                        bg.modify_bg(Gtk.StateType.NORMAL,this.background);
                        bg.modify_base(Gtk.StateType.NORMAL,this.background);
                        bg.modify_text(Gtk.StateType.NORMAL,this.foreground);
                        bg.modify_fg(Gtk.StateType.NORMAL,this.foreground);
                        bg.modify_text(Gtk.StateType.ACTIVE,this.foreground);
                        bg.modify_fg(Gtk.StateType.ACTIVE,this.foreground);
                    }
                }
                /* Recurse into children, if the widget can hold children (so is a container */
                if(bg is Gtk.Container){
                    foreach(Gtk.Widget child in ((Gtk.Container)bg).get_children())
                    {
                        change_color_style(child);
                    }
                }
            }
            /* Create the basic gui. on initializing */
            private void browser_init() {
                if(this.paned == null)
                {

                    this.paned = new Gtk.ScrolledWindow(null,null);
                    this.paned.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
                    this.paned.set_shadow_type(Gtk.ShadowType.NONE);
					if(use_backdrop) {
	                    this.container = new Gmpc.MetaData.Widgets.Backdrop(Gmpc.MetaData.Type.ARTIST_ART);
					}else{
						this.container = new Gtk.EventBox();
					}
                    /*
					this.container.set_focus_hadjustment(this.paned.get_hadjustment());
					this.container.set_focus_vadjustment(this.paned.get_vadjustment());
                    */
                    this.paned.style_set.connect(browser_bg_style_changed);
                    this.paned.add_with_viewport(this.container);
                    this.paned.get_vadjustment().set("step-increment", 20.0);
                    this.paned.show_all();

                }
            }

            /* Clear the view inside the scrolled window*/
            private void clear()
            {
                /* Clear */
                var list = this.container.get_children();
                foreach(Gtk.Widget child in list){
                    child.destroy();
                }
                bitrate_label = null;
            }


            private string get_extension(string path)
            {
                long length = path.length;
                long i=length;
                string retv = null;
                for(;i>0 && (length-i) <8;i--){
                    if(path[i] == '.') {
                        retv = path.substring(i+1);
                        return retv;
                    }
                }
                return retv;
            }
            /**
             * Show the page when playing
             */
            private string song_checksum = null;
            private void update_playing()
            {
                MPD.Song song = server.playlist_get_current_song();
                if(song == null) {
                    debug("GMPC Is playing, cannot get this");
       				if(use_backdrop)
    					(this.container as Gmpc.MetaData.Widgets.Backdrop).set_song(null);
                    update_not_playing();
                    return;
                }
                /* Force it so we won't update when not needed */
                var checksum = Gmpc.Misc.song_checksum(song);
                if(checksum == this.song_checksum)
                {
                    /* No need to update. */
                    return;
                }
                GLib.Timer t = new GLib.Timer();
                this.clear();
				if(use_backdrop)
					(this.container as Gmpc.MetaData.Widgets.Backdrop).set_song(song);
                this.song_checksum = checksum;

                var vbox = new Gtk.VBox (false,6);
                vbox.border_width = 8;
                /* Start building the gui */
                /* Artist image */
                var hbox = new Gtk.HBox(false, 6);
		Gtk.Alignment ali = null;

		int meta_size = (int)(this.container.allocation.width*0.20);
		/* get size based on alloc */
		meta_size = int.min(int.max(100, meta_size), 250);
		if(config.get_int_with_default("Interface", "hide-album-art", 0) == 0)
		{
			/* Album image */
			ali = new Gtk.Alignment(0f,0f,0f,0f);
			//ali.set_size_request(meta_size,meta_size);
			var album_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, meta_size);
			album_image.set_scale_up(true);
			album_image.set_squared(false);
			ali.add(album_image);
			album_image.update_from_song(song);
			hbox.pack_start(ali, false, false, 0);
		}

                /* Artist image */
		if(config.get_int_with_default("Interface", "hide-album-art", 0) == 0 && !use_backdrop)
		{
			ali = new Gtk.Alignment(1f,0f,0f,0f);
			var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, meta_size);
			artist_image.set_scale_up(true);
			artist_image.set_squared(false);
			artist_image.update_from_song(song);
			ali.add(artist_image);
			hbox.pack_end(ali, false, false, 0);
		}

                /* Information box */
                var info_vbox = new Gtk.VBox(false, 6);
                /* Title */
                if(song.title != null) {
                    var box = new Gtk.HBox(false, 6);
                    /* Favored button */

                    if(config.get_int_with_default("Interface", "hide-favorites-icon",0) == 0){
                        var fav_button = new Gmpc.Favorites.Button();
                        fav_button.set_song(song);
                        ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
                        ali.add(fav_button);
                        box.pack_start(ali, false, false, 0);
                    }
                    var label = new Gtk.Label(song.title);
                    label.selectable = true;
                    label.set_markup(GLib.Markup.printf_escaped("<span color='%s' size='%i' weight='bold'>%s</span>",
                                this.title_color,Pango.SCALE*20 ,song.title));
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_alignment(0.0f, 0.5f);
                    box.pack_start(label, true, true, 0);
                    info_vbox.pack_start(box, false, false, 0);

                }else if (song.name!= null) {
                    var label = new Gtk.Label(song.name);
                    label.selectable = true;
                    label.set_markup(GLib.Markup.printf_escaped("<span color='%s' size='%i' weight='bold'>%s</span>",this.
                                title_color, Pango.SCALE*20, song.name));
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_alignment(0.0f, 0.5f);
                    info_vbox.pack_start(label, false, false, 0);
                }
                else if (song.file != null){
                    var filename = GLib.Path.get_basename(song.file);
                    var label = new Gtk.Label(song.name);
                    label.selectable = true;
                    try {
                    var regex_a = new GLib.Regex ("\\.[0-9a-zA-Z]*$");
                        filename = regex_a.replace_literal (filename, -1, 0, "");
                    } catch (GLib.RegexError e) {
                        GLib.error("%s", e.message);
                    }
                    try {
                    var regex_b = new GLib.Regex ("_");
                        filename = regex_b.replace_literal (filename, -1, 0, " ");
                    } catch (GLib.RegexError e) {
                        GLib.error("%s", e.message);
                    }
                    label.set_markup(GLib.Markup.printf_escaped("<span color='%s' size='%i' weight='bold'>%s</span>",this.
                                title_color, Pango.SCALE*20, filename));
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_alignment(0.0f, 0.5f);
                    info_vbox.pack_start(label, false, false, 0);

                }
                /* Artist */
                if(song.artist != null) {
                    var event = new Gtk.EventBox();
                    var box = new Gtk.HBox(false, 6);
                    var label = new Gtk.Label(song.artist);
					event.set_visible_window(false);
					label.selectable = true;
                    var image = new Gtk.Image.from_icon_name("media-artist", Gtk.IconSize.MENU);
                    event.add(image);
                    box.pack_start(event, false, false, 0);
                    label.set_markup(GLib.Markup.printf_escaped("<span size='xx-large' weight='bold'>%s</span>", song.artist));
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_alignment(0.0f, 0.5f);
                    box.pack_start(label, true, true, 0);
                    info_vbox.pack_start(box, false, false, 0);

                    event.set_data_full("artist",(void *)"%s".printf(song.artist), (GLib.DestroyNotify) g_free);
                    event.button_press_event.connect((widget, event) => {
                        string artist = (string)widget.get_data<string>("artist");
                        Gmpc.Browser.Metadata.show_artist(artist);
                        return false;
                        });
                }
                /* Album */
                if(song.album != null) {
                    var event = new Gtk.EventBox();
                    var box = new Gtk.HBox(false, 6);
                    var label = new Gtk.Label(song.album);
					event.set_visible_window(false);
					label.selectable = true;
                    var image = new Gtk.Image.from_icon_name("media-album", Gtk.IconSize.MENU);
                    event.add(image);
                    box.pack_start(event, false, false, 0);
                    label.set_markup(GLib.Markup.printf_escaped("<span size='x-large' weight='bold'>%s %s</span>", song.album,
                        (song.date != null)? "(%s)".printf(song.date):""));
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_alignment(0.0f, 0.5f);
                    box.pack_start(label, true, true, 0);
                    info_vbox.pack_start(box, false, false, 0);



                    event.set_data_full("artist",(void *)"%s".printf(song.artist), (GLib.DestroyNotify) g_free);
                    event.set_data_full("album",(void *)"%s".printf(song.album), (GLib.DestroyNotify) g_free);
                    event.button_press_event.connect((widget, event) => {
                        string artist = (string)widget.get_data<string>("artist");
                        string album = (string)widget.get_data<string>("album");
                        if(artist != null && album != null) {
                            Gmpc.Browser.Metadata.show_album(artist,album);
                            return true;
                       }
                       return false;
                   });
                }
                /* Genre */
                if(song.genre != null) {
                    var box = new Gtk.HBox(false, 6);
                    var label = new Gtk.Label(song.title);
                    label.selectable = true;
                    var image = new Gtk.Image.from_icon_name("media-genre", Gtk.IconSize.MENU);
                    box.pack_start(image, false, false, 0);
                    label.set_markup(GLib.Markup.printf_escaped("<span color='%s' weight='bold'>%s:</span> %s",
                                this.item_color,_("Genre"), song.genre));
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_alignment(0.0f, 0.5f);
                    box.pack_start(label, true, true, 0);
                    info_vbox.pack_start(box, false, false, 0);

                }
                /* Format */
                {
                    var box = new Gtk.HBox(false, 6);
                    var image = new Gtk.Image.from_icon_name("media-format", Gtk.IconSize.MENU);
                    box.pack_start(image, false, false, 0);

                    bitrate_label = new Gtk.Label(song.title);
                    bitrate_label.selectable = true;
                    bitrate_label.set_ellipsize(Pango.EllipsizeMode.END);
                    bitrate_label.set_alignment(0.0f, 0.5f);

                    box.pack_start(bitrate_label, true, true, 0);

                    var bitrate = MPD.Status.get_bitrate(Gmpc.server);
                    var channels = MPD.Status.get_channels(Gmpc.server);
                    bitrate_label.set_markup(GLib.Markup.printf_escaped(
                                "<span color='%s' weight='bold'>%s:</span> %i %s, %.1f %s, %i %s",
                                this.item_color, _("Format"),
                                channels , GLib.ngettext(N_("Channel"),N_("Channels"), channels),
                                MPD.Status.get_samplerate(Gmpc.server)/1000.0, "kHz",
                                bitrate, "kbps"
                                ));

                    info_vbox.pack_start(box, false, false, 0);
                }
                if(song.file != null)
                {

                    string extension = null;
                    extension = get_extension(song.file);
                    if(extension != null)
                    {
                        var box = new Gtk.HBox(false, 6);
                        var image = new Gtk.Image.from_icon_name("media-codec", Gtk.IconSize.MENU);
                        box.pack_start(image, false, false, 0);

                        var label = new Gtk.Label(song.title);
                    label.selectable = true;
                        label.set_ellipsize(Pango.EllipsizeMode.END);
                        label.set_alignment(0.0f, 0.5f);
                        box.pack_start(label, true, true, 0);
                        label.set_markup(GLib.Markup.printf_escaped("<span color='%s' weight='bold'>%s:</span> %s",
                                    this.item_color, _("Codec"),
                                    extension));
                        info_vbox.pack_start(box, false, false, 0);
                    }
                }
                /* Time*/
                if(song.time > 0) {
                    var box = new Gtk.HBox(false, 6);
                    var image = new Gtk.Image.from_icon_name("media-track-length", Gtk.IconSize.MENU);
                    box.pack_start(image, false, false, 0);
                    var label = new Gtk.Label("");
                    label.selectable = true;
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_markup(GLib.Markup.printf_escaped("<span color='%s' weight='bold'>%s:</span> %s",
                                this.item_color,_("Length"),
                                Gmpc.Misc.format_time((ulong) song.time, "")));
                    label.set_alignment(0.0f, 0.5f);
                    box.pack_start(label, true, true, 0);
                    info_vbox.pack_start(box, false, false, 0);
                }

                if(song.track != null) {
                    var box = new Gtk.HBox(false, 6);
                    var image = new Gtk.Image.from_icon_name("media-num-tracks", Gtk.IconSize.MENU);
                    box.pack_start(image, false, false, 0);
                    var label = new Gtk.Label("");
                    label.selectable = true;
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_markup(GLib.Markup.printf_escaped("<span color='%s' weight='bold'>%s:</span> %s %s",
                                this.item_color, _("Track number"),
                                song.track,
                                (song.disc != null)? "[%s]".printf(song.disc):""
                                ));
                    label.set_alignment(0.0f, 0.5f);
                    box.pack_start(label, true, true, 0);
                    info_vbox.pack_start(box, false, false, 0);
                }


                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "heading took: %.6f seconds.", t.elapsed());
                hbox.pack_start(info_vbox, true, true, 0);
                vbox.pack_start(hbox, false, false, 0);

                /* Separator */
                var sep = new Gtk.HSeparator();
                sep.set_size_request(-1, 4);
                vbox.pack_start(sep, false, false, 0);

                var hboxje = new Gtk.HBox(false, 6);

                /* Notebook where all the metadata items are kept, Override the tabs by a radiobutton list. */
                var notebook = new Gtk.Notebook();
                notebook.set_show_border(false);
                notebook.set_show_tabs(false);

                /* Lyrics */
                var i = 0;
                unowned SList<unowned Gtk.RadioButton> group  = null;
                if(config.get_int_with_default("MetaData", "show-lyrics",1) == 1)
                {
                    var alib = new Gtk.Alignment(0f,0f,1f,0f);
                    var text_view = new Gmpc.MetaData.Widgets.TextLabel(song, Gmpc.MetaData.Type.SONG_TXT);
                    alib.add(text_view);
                    notebook.append_page(alib, new Gtk.Label("Lyrics"));
                    var button = new Gtk.RadioButton.with_label(group,("Lyrics"));
                    group = button.get_group();
                    hboxje.pack_start(button, false, false, 0);
                    var j = i;
                    button.clicked.connect((source) => {
                            if((source as Gtk.CheckButton).get_active()) {
                                GLib.log(np2_LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG, "lyrics notebook page %i clicked", j);
                                notebook.set_current_page(j);
                            }
                            });
                    i++;

                    alib.show();
                }
                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "lyrics took: %.6f seconds.", t.elapsed());

                /* Guitar Tabs */
                if(config.get_int_with_default("MetaData", "show-guitar-tabs",1) == 1)
                {
                    var alib = new Gtk.Alignment(0f,0f,1f,0f);
                    var text_view_queried = false;
                    notebook.append_page(alib, new Gtk.Label(_("Guitar Tabs")));
                    var button = new Gtk.RadioButton.with_label(group,_("Guitar Tabs"));
                    group = button.get_group();
                    hboxje.pack_start(button, false, false, 0);
                    var j = i;
                    /* Only query the guitar-tab when opened or first notebook page*/
                    button.clicked.connect((source) => {
                            if((source as Gtk.CheckButton).get_active()) {
                                GLib.log(np2_LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG, "guitar tab notebook page %i clicked", j);
                                notebook.set_current_page(j);
                                if(!text_view_queried){
                                    var text_view = new Gmpc.MetaData.Widgets.TextLabel(song, Gmpc.MetaData.Type.SONG_GUITAR_TAB);
                                    alib.add(text_view);
                                    text_view_queried = true;
                                    this.change_color_style(text_view);
                                    text_view.show();
                                }
                            }
                    });
                    if(i == 0){
                        var text_view = new Gmpc.MetaData.Widgets.TextLabel(song, Gmpc.MetaData.Type.SONG_GUITAR_TAB);
                        alib.add(text_view);
                        text_view_queried = true;
                    }
                    alib.show_all();
                    i++;
                }
                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "guitar tabs took: %.6f seconds.", t.elapsed());

                /* Similar songs */

                if(config.get_int_with_default("MetaData", "show-similar-songs",1) == 1)
                {
                    var similar_songs_queried = false;
                    var similar_songs_box = new Gtk.Alignment(0f,0f,0f,0f);

                    notebook.append_page(similar_songs_box, new Gtk.Label(_("Similar Songs")));

                    var button = new Gtk.RadioButton.with_label(group,_("Similar Songs"));
                    group = button.get_group();
                    hboxje.pack_start(button, false, false, 0);

                    var j = i;
                    /* Only query when opened or first notebook page*/
                    button.clicked.connect((source) => {
                            if((source as Gtk.CheckButton).get_active()) {
                                GLib.log(np2_LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG, "similar song notebook page %i clicked", j);
                                notebook.set_current_page(j);
                                if(!similar_songs_queried){
                                    var similar_songs = new Gmpc.MetaData.Widgets.SimilarSongs(song);
                                    similar_songs.update();
                                    similar_songs_queried = true;
                                    similar_songs_box.add(similar_songs);
                                    this.change_color_style(similar_songs_box);
                                    similar_songs_box.show_all();
                                }
                            }
                            });
                    if(i == 0){
                        var similar_songs = new Gmpc.MetaData.Widgets.SimilarSongs(song);
                        similar_songs.update();
                        similar_songs_queried = true;
                        similar_songs_box.add(similar_songs);
                        similar_songs_box.show_all();
                    }
                    similar_songs_box.show();
                    i++;
                }

                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "similar songs took: %.6f seconds.", t.elapsed());
                if(config.get_int_with_default("MetaData", "show-similar-artist",1) == 1 && song.artist != null)
                {
                    var similar_artist = new Gmpc.MetaData.Widgets.SimilarArtists(Gmpc.server,song);

                    notebook.append_page(similar_artist, new Gtk.Label(_("Similar Artist")));

                    var button = new Gtk.RadioButton.with_label(group,_("Similar Artist"));
                    group = button.get_group();
                    hboxje.pack_start(button, false, false, 0);

                    var j = i;
                    button.clicked.connect((source) => {
                            if((source as Gtk.CheckButton).get_active()) {
                                GLib.log(np2_LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG, "similar artist notebook page %i clicked", j);
                                similar_artist.first_show();
                                notebook.set_current_page(j);
                            }
                    });
                    similar_artist.show();
                    i++;
                }
                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "similar Artist took: %.6f seconds.", t.elapsed());
                if(config.get_int_with_default("MetaData", "show-web-links",1) == 1)
                {

                    var song_links = new Gmpc.MetaData.Widgets.SongLinks(Gmpc.MetaData.Widgets.SongLinks.Type.SONG,song);
                    notebook.append_page(song_links, new Gtk.Label(_("Web Links")));
                    var button = new Gtk.RadioButton.with_label(group,_("Web Links"));
                    group = button.get_group();
                    hboxje.pack_start(button, false, false, 0);
                    var j = i;
                    button.clicked.connect((source) => {
                            if((source as Gtk.CheckButton).get_active()) {
                                GLib.log(np2_LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG, "song link notebook page %i clicked", j);
                                notebook.set_current_page(j);
                            }
                            });
                    song_links.show();
                    i++;
                }
                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "Web links took: %.6f seconds.", t.elapsed());
                if(config.get_int_with_default("MetaData", "show-artist-information",1) == 1)
                {
                    var alib = new Gtk.Alignment(0f,0f,1f,0f);
                    var text_view_queried = false;

                    notebook.append_page(alib, new Gtk.Label(_("Artist information")));
                    var button = new Gtk.RadioButton.with_label(group,_("Artist information"));
                    group = button.get_group();
                    hboxje.pack_start(button, false, false, 0);
                    var j = i;
                    /* Only query the guitar-tab when opened or first notebook page*/
                    button.clicked.connect((source) => {
                            if((source as Gtk.CheckButton).get_active()) {
                                GLib.log(np2_LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG, "artist info notebook page %i clicked", j);
                                notebook.set_current_page(j);
                                if(!text_view_queried){
                                    var text_view = new Gmpc.MetaData.Widgets.TextLabel(song,Gmpc.MetaData.Type.ARTIST_TXT);
                                    alib.add(text_view);
                                    text_view_queried = true;
                                    text_view.show();
                                    this.change_color_style(text_view);
                                }
                            }
                            });
                    if(i == 0){
                        var text_view = new Gmpc.MetaData.Widgets.TextLabel(song,Gmpc.MetaData.Type.ARTIST_TXT);
                        alib.add(text_view);
                        text_view_queried = true;
                    }
                    alib.show_all();
                    i++;
                }

                {
                    var alib = new Gtk.Alignment(0f,0f,1f,0f);
                    var text_view_queried = false;

                    notebook.append_page(alib, new Gtk.Label(_("Songs from album")));
                    var button = new Gtk.RadioButton.with_label(group,_("Songs from album"));
                    group = button.get_group();
                    hboxje.pack_start(button, false, false, 0);
                    var j = i;
                    var sl = new Gmpc.Widgets.Songlist();

                    sl.song_clicked.connect((source, song) => {
                            if(song.file != null) {
                                Gmpc.MpdInteraction.play_path(song.file);
                            }
                    });
                    sl.play_song_clicked.connect((source, song) => {
                            if(song.file != null) {
                                Gmpc.MpdInteraction.play_path(song.file);
                            }
                    });
                    alib.add(sl);
                    button.clicked.connect((source) => {
                            if((source as Gtk.CheckButton).get_active()) {
                                GLib.log(np2_LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG, "artist info notebook page %i clicked", j);
                                notebook.set_current_page(j);
                                if(!text_view_queried){
                                    if(song.artist != null && song.album != null)
                                    {
                                        MPD.Database.search_start(server,true);
                                        if(song.albumartist != null) {
                                            MPD.Database.search_add_constraint(server,
                                            MPD.Tag.Type.ALBUM_ARTIST, song.albumartist);
                                        }else{
                                            MPD.Database.search_add_constraint(server,
                                                MPD.Tag.Type.ARTIST, song.artist);
                                        }
                                        MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, song.album);
                                        var data = MPD.Database.search_commit(server);
                                        data.sort_album_disc_track();
                                        sl.set_from_data((owned)data, true);
                                        this.change_color_style(sl);
                                    }else{
                                        sl.destroy();
                                        alib.add(new Gtk.Label(_("Not available")));
                                    }
                                    text_view_queried = true;
                                }
                            }
                            });
                            if(i == 0){
                            if(song.artist != null && song.album != null)
                            {
                                MPD.Database.search_start(server,true);
                                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, song.artist);
                                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, song.album);
                                var data = MPD.Database.search_commit(server);
                                data.sort_album_disc_track();
                                sl.set_from_data((owned)data, true);
                            }else{
                                 sl.destroy();
                                 alib.add(new Gtk.Label(_("Not available")));
                            }
                            text_view_queried = true;
                    }
                    alib.show_all();
                    i++;
                }

                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "Artist info took: %.6f seconds.", t.elapsed());
                /* Track changed pages */
                notebook.notify["page"].connect((source,spec) => {
                        var page = notebook.get_current_page();
                        config.set_int("NowPlaying", "last-page", (int)page);

                });
                /* Restore right page */
                if(i > 0){
                    var page = config.get_int_with_default("NowPlaying", "last-page", 0);
                    if(page > i)
                    {
                        notebook.set_current_page(0);
                    }else{
                        /* The list is in reversed order, compensate for that. */
                        var w = group.nth_data(i-page-1);
                        w.set_active(true);
                        notebook.set_current_page(page);
                    }
                }

                ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
                ali.add(hboxje);

                /* Create pane in 2. */
                var bottom_hbox = new Gtk.HBox(false, 6);
                /* left pane */
                var metadata_vbox = new Gtk.VBox(false, 6);
                metadata_vbox.pack_start(ali, false, false, 0);
                sep = new Gtk.HSeparator();
                sep.set_size_request(-1, 1);
                metadata_vbox.pack_start(sep, false, false, 0);
                metadata_vbox.pack_start(notebook, false, false, 0);

                bottom_hbox.pack_start(metadata_vbox, true, true, 0);

                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "Building now playing took: %.6f seconds.", t.elapsed());
                /* Create album list */
                if(song.album != null && song.artist != null)
                {
                    var sep2 = new Gtk.VSeparator();
                    sep2.set_size_request(-1, 4);
                    bottom_hbox.pack_start(sep2, false, false, 0);
                    int albums =0;
                    /* Create album list */
                    ali = new Gtk.Alignment(0.0f, 0.0f, 0.0f, 0.0f);
                    var album_hbox = new Gtk.VBox(false, 6);
                    album_hbox.set_size_request(250, -1);
                    ali.add(album_hbox);
                    bottom_hbox.pack_start(ali, false, false, 0);

                    var label = new Gtk.Label(song.artist);
                    label.selectable = true;
                    label.set_size_request(240, -1);
                    label.set_markup(GLib.Markup.printf_escaped("<span size='x-large' weight='bold' color='%s'>%s</span><span size='x-large' weight='bold'> %s</span>",this.item_color,_("Other albums by"), song.artist));
                    label.set_line_wrap_mode(Pango.WrapMode.WORD_CHAR);
                    label.set_line_wrap(true);
                    label.set_alignment(0.0f, 0.5f);
                    album_hbox.pack_start(label, false, false,0);

                    MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM);
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, song.artist);
                    MPD.Data.Item list = null;
                    var data = MPD.Database.search_commit(server);
                    if(data != null){
                        unowned MPD.Data.Item iter = data.get_first();
                        do{
                            if(iter.tag == song.album){
                                iter.next(false);
                                continue;
                            }
                            list.append_new();
                            list.type = MPD.Data.Type.SONG;
                            list.song = new MPD.Song();
                            list.song.artist = song.artist;
                            list.song.album  = iter.tag;
                            /*
                            MPD.Database.search_field_start(server,MPD.Tag.Type.DATE);
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, song.artist);
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, iter.tag);
                            var ydata = MPD.Database.search_commit(server);
                            if(ydata != null) {
                                list.song.date = ydata.tag;
                            }
                            */
                            iter.next(false);
                        }while(iter != null);
                    }

                    list.sort_album_disc_track();
                    if(list != null) {
                        unowned MPD.Data.Item iter = list.get_first();
                        do{
                            var button = new Gtk.Button();
                            button.set_relief(Gtk.ReliefStyle.NONE);
                            var but_hbox = new Gtk.HBox(false, 6);
                            button.add(but_hbox);
                            var image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, 48);
                            var but_song = iter.song;
                            image.set_squared(true);
                            image.update_from_song_delayed(but_song);

                            but_hbox.pack_start(image, false, false, 0);

                            var but_label = new Gtk.Label(iter.song.album);
                            but_label.selectable = true;
                            but_label.set_alignment(0.0f, 0.5f);
                            var strlabel = "";
                            if(iter.song.date != null && iter.song.date.length > 0) strlabel += "%s\n".printf(iter.song.date);
                            if(iter.song.album != null) strlabel+= iter.song.album;
                            else strlabel += _("No Album");
                            but_label.set_markup(GLib.Markup.printf_escaped("<b>%s</b>",strlabel));
                            but_label.set_ellipsize(Pango.EllipsizeMode.END);
                            but_hbox.pack_start(but_label, true, true, 0);

                            album_hbox.pack_start(button, false, false,0);

                            button.clicked.connect((source) => {
                                    Gmpc.Browser.Metadata.show_album(song.artist, but_song.album);
                                    });
                            albums++;

                            iter.next(false);
                        }while(iter!= null);
                    }

                    if(albums == 0) {
                        album_hbox.destroy();
                        sep2.destroy();
                    }
                }
                vbox.pack_start(bottom_hbox, true, true, 0);

                /* show it */
                this.container.add(vbox);
                this.change_color_style(this.container);
                this.container.show_all();

                t.stop();
                GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "Building now playing took: %.6f seconds.", t.elapsed());
            }
            /**
             * This shows the page when mpd is not playing, for now it is the gmpc logo + Gnome Music Player Client
             */
            private void update_not_playing()
            {
                this.clear();
                song_checksum = null;
   				if(use_backdrop)
   					(this.container as Gmpc.MetaData.Widgets.Backdrop).set_song(null);

                var it = Gtk.IconTheme.get_default();
                Gtk.IconInfo info = it.lookup_icon("gmpc", 150, 0);
                var path = info.get_filename();
                Gtk.Image image = null;
                if(path != null)
                {
                    try {
                        var pb = new Gdk.Pixbuf.from_file_at_scale(path, 150, 150, true);
                        image = new Gtk.Image.from_pixbuf(pb);
                    } catch (Error e)
                    {
                        warning("Failed to load the gmpc logo: %s", e.message);
                        return;
                    }
                }
                if(image == null){
                    image = new Gtk.Image.from_icon_name("gmpc", Gtk.IconSize.DIALOG);
                }

                var hbox = new Gtk.HBox(false, 6);
                var label = new Gtk.Label(_("Gnome Music Player Client"));
                label.set_selectable(true);
                label.set_markup("<span size='%i' weight='bold'>%s</span>".printf(28*Pango.SCALE,_("Gnome Music Player Client")));
                hbox.pack_start(image, false, false, 0);
                hbox.pack_start(label, false, false, 0);

                var ali = new Gtk.Alignment(0.5f,0.5f,0.0f, 0.0f);
                ali.add(hbox);
                this.container.add(ali);

                this.change_color_style(this.container);
                this.container.show_all();

            }
            /**
             * Update the view according to state. If playing/paused show song info, other wise the not playing page
             */
            private void update()
            {
                switch(MPD.Player.get_state(Gmpc.server))
                {
                    case MPD.Player.State.PLAY:
                    case MPD.Player.State.PAUSE:
                        debug("Update playing");
                        update_playing();
                        break;
                    default:
                        debug("update not playing");
                        update_not_playing();
                        break;
                }
            }

            /**
             * Makes gmpc jump to the now playing browser
             */
            public void select_now_playing_browser()
            {
                unowned Gtk.TreeView tree = Gmpc.Playlist3.get_category_tree_view();
                var sel = tree.get_selection();
                var path = np_ref.get_path();
                if(path != null)
                {
                    sel.select_path(path);
                }
            }

            /**
             * Gmpc.Plugin.BrowserIface.add_go_menu
             */
            private int browser_add_go_menu(Gtk.Menu menu)
            {
                if(this.get_enabled())
                {
                    var item = new Gtk.ImageMenuItem.with_mnemonic(_("Now Playing"));
                    item.set_image(new Gtk.Image.from_icon_name("media-audiofile", Gtk.IconSize.MENU));
                    item.activate.connect(select_now_playing_browser);
                    item.add_accelerator("activate", menu.get_accel_group(),0x069, Gdk.ModifierType.CONTROL_MASK, Gtk.AccelFlags.VISIBLE);
                    menu.append(item);
                    return 1;
                }
                return 0;
            }
        }
    }
}
