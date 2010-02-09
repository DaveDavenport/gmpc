
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
using Config;
using GLib;
using Gtk;
using Gdk;
using MPD;

/* trick to get config.h included */
static const string some_unique_name_fav = Config.VERSION;
private const bool use_transition_fav = Gmpc.use_transition;
static Gmpc.Favorites.List favorites = null;

namespace Gmpc.Favorites{
    /**
     * This class is created, and stays active until the last GmpcFavoritesButton gets removed
     * POSSIBLE ISSUE: setting favorites list back to NULL seems to fail. It is no issue as 
     * I know atleast one will be active.
     */
    private class List : GLib.Object {
        private MPD.Data.Item? list = null; 
        construct {
            gmpcconn.connection_changed += con_changed;
            gmpcconn.status_changed += status_changed;
        }

        /**
         * If disconnected from mpd, clear the list.
         * On connect fill 
         */
        private
        void 
        con_changed(Gmpc.Connection conn, MPD.Server server, int connect)
        {
            if(connect == 1){
                list = MPD.Database.get_playlist_content(server, _("Favorites"));
                this.updated();
            }else{
                list = null;
            }
        }
        /**
         * If playlist changed update the list
         */
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
        /****************************************************************************************
         * "Public" api.  
         ****************************************************************************************/
        /**
         * Signal for the widget using the list to see if it needs to recheck status
         */
        public signal void updated();

        /**
         * Check if the song (specified by path) is favored
         */
        public
        bool
        is_favorite(string path)
        {
            weak MPD.Data.Item iter = this.list.get_first();
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
        /**
         * Favor, or unfavor a song
         */
        public
        void
        set_favorite(string path, bool favorite)
        {
            bool current = this.is_favorite(path);
            /* Do nothing if state does not change */
            if(current != favorite)
            {
                if(favorite){
                    /* Add it */
                    MPD.Database.playlist_list_add(server, _("Favorites"), path);
                }else{
                    /* Remove it */
                    /* To be able to remove it we have to first lookup the position */
                    /* This needs libmpd 0.18.1 */
                    weak MPD.Data.Item iter = this.list.get_first();
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
    /**
     * The actual favorite button
     */
    public class Button : Gtk.EventBox {
        /* the song the button show show the state for */
        private MPD.Song? song;
        /* The image displaying the state */
        private Gtk.Image image;
        /* The current state */
        private bool fstate = false;
        /* The pixbuf holding the unmodified  image */
        private Gdk.Pixbuf pb = null;

        /**
         * Creation of the object 
         */
        construct {
            this.no_show_all = true;
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
                favorites.set_favorite(this.song.file, !this.fstate);
                this.fstate = !this.fstate;
            }
            else if (event.button == 3 && this.song != null) {
                var menu = new Gtk.Menu();
                MPD.Data.Item ? item = MPD.Database.get_playlist_list(server);
                while(item != null)
                {
                    string pp = item.playlist.path;
                    var entry = new Gtk.ImageMenuItem.with_label(pp);
                    if(pp == _("Favorites")) {
                        entry.set_image(new Gtk.Image.from_icon_name("emblem-favorite", Gtk.IconSize.MENU));
                    }else{
                        entry.set_image(new Gtk.Image.from_icon_name("media-playlist", Gtk.IconSize.MENU));
                    }
                    entry.activate.connect((source) => {
                            MPD.Database.playlist_list_add(server,pp, this.song.file);
                            });
                    menu.append(entry);
                    item.next_free();
                }
                menu.show_all();
                menu.popup(null, null, null, event.button, event.time);

            }
            return false;
        }

        /* on mouse over, do some pre-highlighting */
        private
        bool
        enter_notify_event_callback(Gmpc.Favorites.Button button, Gdk.EventCrossing motion)
        {
            var pb2 = pb.copy();
            if(this.fstate){
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
                this.fstate =  favorites.is_favorite(this.song.file);
            }else{
                /* Hide the widget and do nothing */
                this.hide();
                return;
            }
            /* Copy the pixbuf */
            var pb2 = pb.copy();
            /* Depending on the state colorshift the pixbuf */
            if(this.fstate) {
                Gmpc.Misc.colorshift_pixbuf(pb2, pb, 30);
            }else{
                Gmpc.Misc.colorshift_pixbuf(pb2, pb, -80);
            }
            this.image.set_from_pixbuf(pb2);
            this.image.show();
            this.show();
        }
        /**********************************************************************
         * Public api
         *********************************************************************/
         /* Set the song the button should watch. or NULL to watch non */
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
