/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
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

public class Gmpc.Song.Links: Gtk.Frame 
{
    private const string some_unique_name = Config.VERSION;
    public enum Type {
        ARTIST,
        ALBUM,
        SONG
    }
    private Type type = Type.ARTIST;
    private MPD.Song song = null;
    private Gmpc.AsyncDownload.Handle handle = null;

    ~Links()
    {
        if(this.handle != null)
        {
            this.handle.cancel();
        }
    }
    private void download(Gtk.MenuItem item)
    {
        var child = this.get_child();
        if(child != null) child.destroy();
        /* now try to download */
        this.add(new Gtk.ProgressBar());
        this.show_all();
        this.handle = Gmpc.AsyncDownload.download("http://gmpc.wikia.com/index.php?title=GMPC_METADATA_WEBLINKLIST&action=raw",download_file);

    }
    private bool button_press_event(Gtk.Widget label, Gdk.EventButton event)
    {
        if(event.button == 3)
        {
            var menu = new Gtk.Menu();
            var item = new Gtk.ImageMenuItem.with_label(_("Update list from internet")); 
            item.set_image(new Gtk.Image.from_stock(Gtk.STOCK_REFRESH, Gtk.IconSize.MENU));
            item.activate += download;
            menu.append(item);
            menu.show_all();
            menu.popup(null, null, null, event.button, event.time);
        }
        return false;
    }

    public Links(Type type, MPD.Song song)
    {
        this.type = type;
        this.song = song;
        var event = new Gtk.EventBox();
        var label = new Gtk.Label(""); 
        event.add(label);
        event.visible_window = false;
        this.label_widget = event;
        label.set_markup("<b>%s:</b>".printf(_("Links")));
        this.shadow = Gtk.ShadowType.NONE;

        event.button_press_event += button_press_event;
        parse_uris();
    }
    private void open_uri(Gtk.LinkButton button)
    {
        Gtk.LinkButton lb = button;
        stdout.printf("open uri: %s\n", lb.get_uri());
        Gmpc.open_uri(lb.get_uri());
    }
    private void download_file(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status)
    {
        if(status == AsyncDownload.Status.PROGRESS) {
            Gtk.ProgressBar pb = (Gtk.ProgressBar)this.get_child();
            if(pb != null){
                pb.pulse();
            }
            return;
        }
        if(status == AsyncDownload.Status.DONE) {
            int64 length=0;
            var a = handle.get_data(out length); 
            var path = Gmpc.user_path("weblinks.list");
            try{
                GLib.FileUtils.set_contents(path, a, (long)length);
                this.parse_uris();
                this.show_all();
            }
            catch(Error e)
            {
                stdout.printf("Error: %s\n", e.message);
            }
        }
        else 
        {
            var path = Gmpc.user_path("weblinks.list");
            /* set dummy file */
            try{
                string a = " ";
                GLib.FileUtils.set_contents(path, a, a.length);
                this.parse_uris();
                this.show_all();
            }
            catch(Error e)
            {
                stdout.printf("Error: %s\n", e.message);
            }
        }
        this.handle.free();
        this.handle = null;
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
                /* now try to download */
                this.add(new Gtk.ProgressBar());
                this.show_all();
                this.handle = Gmpc.AsyncDownload.download("http://download.sarine.nl/weblinks.list",download_file);
                return;
            }
        }
        try {
            file.load_from_file(path, GLib.KeyFileFlags.NONE);
        } 
        catch  (Error e) {
            stdout.printf("Failed to load file: %s\n", path);
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
                string  typestr = file.get_string(entry,"type");
                string  uri = file.get_string(entry, "url");
                Type type;
                switch(typestr) {
                    case "artist":
                        type = Type.ARTIST;
                        if(this.song.artist != null)
                            uri = uri.replace("%ARTIST%", this.song.artist);
                        break;
                    case "album":
                        type = Type.ALBUM;

                        if(this.song.album != null)
                            uri = uri.replace("%ALBUM%", this.song.album);

                        if(this.song.artist != null)
                            uri = uri.replace("%ARTIST%", this.song.artist);
                        break;
                    default:
                        type = Type.SONG;

                        if(this.song.title != null)
                            uri = uri.replace("%TITLE%", this.song.title);

                        if(this.song.album != null)
                            uri = uri.replace("%ALBUM%", this.song.album);

                        if(this.song.artist != null)
                        uri = uri.replace("%ARTIST%", this.song.artist);
                        break;
                }
                if((int)type <= (int)this.type)
                {
                    var label = new Gtk.LinkButton(uri);
                    label.set_label(_("Lookup %s in %s").printf(typestr,entry));
                    label.set_alignment(0.0f, 0.5f);
                    vbox.pack_start(label, false, true, 0);
                    label.clicked += open_uri;
                }
            }catch(Error e){
                stdout.printf("Failed to get entry from %s: '%s'\n", path, e.message);
            }

        }
    }

}
