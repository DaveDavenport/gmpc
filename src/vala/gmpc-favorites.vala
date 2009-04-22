
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

static const string some_unique_name = Config.VERSION;
static Gmpc.Favorites.List favorites = null;

namespace Gmpc.Favorites{
    public class List : GLib.Object {
        private MPD.Data.Item? list = null; 
        construct {
            stdout.printf("Create list\n");
            gmpcconn.connection_changed += con_changed;
            gmpcconn.status_changed += status_changed;
        }

        ~List() {
            stdout.printf("Destroy\n");
        }
        
        private
        void 
        con_changed(Gmpc.Connection conn, MPD.Server server, int connect)
        {
            if(connect == 1){
                stdout.printf("fill list\n");
                list = MPD.Database.get_playlist_content(server, _("Favorites"));
                this.updated();
            }else{
                list = null;
            }
        }
        private 
        void
        status_changed(Gmpc.Connection conn, MPD.Server server, MPD.Status.Changed what)
        {
            if((what&MPD.Status.Changed.STORED_PLAYLIST) == MPD.Status.Changed.STORED_PLAYLIST)
            {
                list = MPD.Database.get_playlist_content(server, _("Favorites"));
                this.updated();
            }
        }

        signal void updated();
        /* Public access functions */
        public
        bool
        is_favorite(string path)
        {
            weak MPD.Data.Item iter = this.list.first();
            while(iter != null)
            {
                if(iter.type == MPD.Data.Type.SONG)
                {
                    if(iter.song.file == path){
                        return true;
                    }
                }
                iter = iter.next(false);
            }
            return false;
        }
        public
        void
        set_favorite(string path, bool favorite)
        {
            bool current = this.is_favorite(path);
            if(current != favorite)
            {
                if(favorite){
                    MPD.Database.playlist_list_add(server, _("Favorites"), path);
                }else{
                    weak MPD.Data.Item iter = this.list.first();
                    while(iter != null)
                    {
                        if(iter.type == MPD.Data.Type.SONG)
                        {
                            if(iter.song.file == path){
                                stdout.printf("remove: %i\n", iter.song.pos);
                                MPD.Database.playlist_list_delete(server, _("Favorites"), iter.song.pos);
                                return;
                            }
                        }
                        iter = iter.next(false);
                    }
                }
            }
        }
    }

    public class Button : Gtk.EventBox {
        private MPD.Song? song;
        private Gtk.Image image;
        construct {
            this.visible_window = false;
            if(favorites == null){
                favorites = new List();
            }
            else
            {
                favorites.ref();
            }
            favorites.updated += update;
            this.image = new Gtk.Image();
            this.image.set_from_icon_name("emblem-favorite", Gtk.IconSize.BUTTON);
            this.image.sensitive = false;
            this.add(this.image);
            this.button_press_event += button_press_event_callback;
        }
        ~Button() {
            stdout.printf("Button destroy\n");
            if(favorites != null)
                favorites.unref();
        }
        private
        bool
        button_press_event_callback(Gmpc.Favorites.Button button,Gdk.EventButton event)
        {
            if(event.button == 1 && this.song != null)
            {
                stdout.printf("Set favorites: %s", (this.image.sensitive)?"off":"on");
                favorites.set_favorite(this.song.file, !this.image.sensitive);
            }
            return false;
        }

        private
        void
        update(Gmpc.Favorites.List list)
        {
            stdout.printf("set song\n");
            if(this.song != null)
            {
                this.image.sensitive = favorites.is_favorite(this.song.file);
            }else{
                this.image.sensitive = false; 
            }
        }

        public
        void
        set_song(MPD.Song? song)
        {
            if(this.song == null && song == null ) return;
            if(this.song != null  && song != null && this.song.file == song.file) return;
            this.song = song;
            this.update(favorites);
        }
    }
}
