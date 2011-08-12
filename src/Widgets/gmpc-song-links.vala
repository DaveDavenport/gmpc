/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
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
using GLib;
using Gtk;
using Gdk;
using Cairo;
using MPD;
using Gmpc;

private const bool use_transition_sl = Gmpc.use_transition;

static bool initialized = false;
public class Gmpc.MetaData.Widgets.SongLinks: Gtk.Frame
{
    private const string some_unique_name = Config.VERSION;
    public enum Type {
        ARTIST,
        ALBUM,
        SONG
    }
    private Type type = Type.ARTIST;
    private MPD.Song song = null;

    ~SongLinks()
    {
    }

    static void open_uri_function(Gtk.LinkButton but, string uri)
    {
        Gmpc.open_uri(uri);
    }

    public SongLinks(Type type, MPD.Song song)
    {
        debug("Links created");
        this.type = type;
        this.song = song;
        var event = new Gtk.EventBox();
        var label = new Gtk.Label("");
        event.add(label);
        event.visible_window = false;
        this.label_widget = event;
        label.set_markup("<b>%s:</b>".printf(_("Web Links")));
        this.shadow = Gtk.ShadowType.NONE;

        parse_uris();

        if(!initialized) {
            Gtk.LinkButton.set_uri_hook(open_uri_function);
            initialized = true;
        }
    }

    private void parse_uris()
    {
        var child = this.get_child();
        if(child != null) child.destroy();

        var file = new GLib.KeyFile();
        var path = Gmpc.user_path("weblinks.list");
        if(! FileUtils.test(path, FileTest.EXISTS))
        {
            path = Gmpc.data_path("weblinks.list");
            if(! FileUtils.test(path, FileTest.EXISTS))
            {
                return;
            }
        }
        try {
            file.load_from_file(path, GLib.KeyFileFlags.NONE);
        }
        catch  (Error e) {
            return;
        }

        var ali = new Gtk.Alignment(0.0f, 0.0f, 0.0f,0.0f);
        ali.set_padding(8,8,12,6);
        this.add(ali);

        var vbox = new Gtk.VBox(false, 0);
        ali.add(vbox);

        var groups = file.get_groups();
        foreach(string entry in groups)
        {
            try{
                string typestr = file.get_string(entry,"type");
                string uri = file.get_string(entry, "url");

                Type type;
                switch(typestr) {
                    case "artist":
                        type = Type.ARTIST;
                        if(this.song.artist != null)
                            uri = uri.replace("%ARTIST%", Gmpc.AsyncDownload.escape_uri(this.song.artist));
                        break;
                    case "album":
                        type = Type.ALBUM;

                        if(this.song.album != null)
                            uri = uri.replace("%ALBUM%", Gmpc.AsyncDownload.escape_uri(this.song.album));

                        if(this.song.artist != null)
                            uri = uri.replace("%ARTIST%", Gmpc.AsyncDownload.escape_uri(this.song.artist));
                        break;
                    case "song":
                    default:
                        type = Type.SONG;

                        if(this.song.title != null)
                            uri = uri.replace("%TITLE%", Gmpc.AsyncDownload.escape_uri(this.song.title));

                        if(this.song.album != null)
                            uri = uri.replace("%ALBUM%", Gmpc.AsyncDownload.escape_uri(this.song.album));

                        if(this.song.artist != null)
                            uri = uri.replace("%ARTIST%", Gmpc.AsyncDownload.escape_uri(this.song.artist));
                        break;
                }
                try{
                string sar = file.get_string(entry, "search-and-replace");
                if(sar != null) {
                    string[] s = sar.split("::");
                    if(s.length == 2){
                        try{
                        var regex =  new GLib.Regex (s[0]);
                        uri = regex.replace_literal(uri,-1,0, s[1]);
                        } catch (GLib.RegexError e) {
                            GLib.debug("Failed to compile regex: '%s'\n", e.message);
                        }
                    }
                }
                }catch(Error e) {

                }

                if((int)type <= (int)this.type)
                {
                    var label = new Gtk.LinkButton(uri);
                    label.set_label(_("Lookup %s on %s").printf(_(typestr),entry));
                    label.set_alignment(0.0f, 0.5f);
                    vbox.pack_start(label, false, true, 0);
                }
            }catch(Error e){
                GLib.error("Failed to get entry from %s: '%s'\n", path, e.message);
            }

        }
    }

}
