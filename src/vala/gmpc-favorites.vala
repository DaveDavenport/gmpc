
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
        private bool state = false;
        private Gdk.Pixbuf pb = null;
        construct {
            this.visible_window = false;
            var it = Gtk.IconTheme.get_default();
            try {
                pb = it.load_icon("emblem-favorite",24, 0);
            }catch(Error e) {
                stdout.printf("error: %s\n", e.message);
            }
            if(favorites == null){
                favorites = new List();
            } else {
                favorites.ref();
            }
            favorites.updated += update;
            this.image = new Gtk.Image();
            this.update(favorites);
            this.add(this.image);
            this.button_press_event += button_press_event_callback;
            this.enter_notify_event += enter_notify_event_callback;
            this.leave_notify_event += leave_notify_event_callback;

        }
        ~Button() {
            if(favorites != null)
                favorites.unref();
        }
        private
        bool
        button_press_event_callback(Gmpc.Favorites.Button button,Gdk.EventButton event)
        {
            if(event.button == 1 && this.song != null) {
                favorites.set_favorite(this.song.file, !this.state);
                this.state = !this.state;
            }
            return false;
        }

        /* on mouse over, do some pre-highlighting */
        private
        bool
        enter_notify_event_callback(Gmpc.Favorites.Button button, Gdk.EventCrossing motion)
        {
            var pb2 = pb.copy();
            if(this.state){
                Gmpc.Misc.colorshift_pixbuf(pb2, pb, 10);
            }else{
                Gmpc.Misc.colorshift_pixbuf(pb2, pb,-50);
            }
            this.image.set_from_pixbuf(pb2);
            return false;
        }
        /* Reset default highlighting */
        private
        bool
        leave_notify_event_callback(Gmpc.Favorites.Button button, Gdk.EventCrossing motion)
        {
            this.update(favorites);
            return false;
        }
        /* Update the icon according to state */
        private
        void
        update(Gmpc.Favorites.List list)
        {
            if(this.song != null){
                this.state =  favorites.is_favorite(this.song.file);
            }else{
                this.state= false;
            }
            var pb2 = pb.copy();
            if(this.state) {
                Gmpc.Misc.colorshift_pixbuf(pb2, pb, 30);
            }else{
                Gmpc.Misc.colorshift_pixbuf(pb2, pb, -80);
            }
            this.image.set_from_pixbuf(pb2);
            this.show();
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
