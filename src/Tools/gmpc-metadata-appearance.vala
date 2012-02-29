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

/**
 * This plugin offers the refinement of metadata appearances
 */

using Config;
using Gtk;
using Gmpc;

private const bool use_transition_tma = Gmpc.use_transition;
private const string some_unique_name_tma = Config.VERSION;


public class Gmpc.Tools.MetadataAppearance : Gmpc.Plugin.Base, Gmpc.Plugin.PreferencesIface {

    private const int[] version = {0,0,0};
    public override unowned int[] get_version() {
        return this.version;
    }
    
    public override unowned string get_name() {
        return "Metadata Appearance";
    }
    
    construct {
        /* Mark the plugin as an internal and dummy */
        this.plugin_type = 8+4;
    }
    
    /* preferences pane */
    public static void
    on_checkbutton_show_lyrics_toggled(CheckButton source)
    {
        config.set_int("MetaData", "show-lyrics", (int)source.get_active());
    }

    public static void
    on_checkbutton_show_artist_information_toggled(CheckButton source)
    {
        config.set_int("MetaData", "show-artist-information", (int)source.get_active());
    }

    public static void
    on_checkbutton_show_web_links_toggled(CheckButton source)
    {
        config.set_int("MetaData", "show-web-links", (int)source.get_active());
    }
    
    public static void
    on_checkbutton_show_similar_artists_toggled(CheckButton source)
    {
        config.set_int("MetaData", "show-similar-artist", (int)source.get_active());
    }
    
    public static void
    on_checkbutton_show_similar_songs_toggled(CheckButton source)
    {
        config.set_int("MetaData", "show-similar-songs", (int)source.get_active());
    }
    
    public static void
    on_checkbutton_show_guitar_tabs_toggled(CheckButton source)
    {
        config.set_int("MetaData", "show-guitar-tabs", (int)source.get_active());
    }

    public static void
    on_checkbutton_show_songs_from_album_toggled(CheckButton source)
    {
        config.set_int("MetaData", "show-songs-from-album", (int)source.get_active());
    }
    
    public void
    preferences_pane_construct(Gtk.Container container)
    {
        try {
            var builder = new Builder();
            string preferences_ui_file = Gmpc.data_path("preferences-metadata-appearance.ui");
            builder.add_from_file(preferences_ui_file);
            builder.connect_signals(null);
            Widget builderWidget = builder.get_object("frame_metadata_appearance_settings") as Frame;
            container.add(builderWidget);
            builderWidget.show_all();
            
            builderWidget = builder.get_object("checkbutton_show_lyrics") as CheckButton;
            ((CheckButton)builderWidget).set_active((bool)config.get_int_with_default("MetaData", "show-lyrics", 1));
            
            builderWidget = builder.get_object("checkbutton_show_artist_information") as CheckButton;
            ((CheckButton)builderWidget).set_active((bool)config.get_int_with_default("MetaData", "show-artist-information", 1));
            
            builderWidget = builder.get_object("checkbutton_show_web_links") as CheckButton;
            ((CheckButton)builderWidget).set_active((bool)config.get_int_with_default("MetaData", "show-web-links", 1));
            
            builderWidget = builder.get_object("checkbutton_show_similar_artists") as CheckButton;
            ((CheckButton)builderWidget).set_active((bool)config.get_int_with_default("MetaData", "show-similar-artist", 1));
            
            builderWidget = builder.get_object("checkbutton_show_similar_songs") as CheckButton;
            ((CheckButton)builderWidget).set_active((bool)config.get_int_with_default("MetaData", "show-similar-songs", 1));
            
            builderWidget = builder.get_object("checkbutton_show_guitar_tabs") as CheckButton;
            ((CheckButton)builderWidget).set_active((bool)config.get_int_with_default("MetaData", "show-guitar-tabs", 1));

            builderWidget = builder.get_object("checkbutton_show_songs_from_album") as CheckButton;
            ((CheckButton)builderWidget).set_active((bool)config.get_int_with_default("MetaData", "show-songs-from-album", 1));
        } catch (Error e) {
            stderr.printf("Could not load UI: %s\n", e.message);
        }
    }
    
    public void
    preferences_pane_destroy(Gtk.Container container)
    {
       foreach(Gtk.Widget child in container.get_children())
       {
           container.remove(child);
       }
    }
}
