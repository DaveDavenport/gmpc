
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
using MPD;

/* trick to get config.h included */
static const string some_unique_name_fav = Config.VERSION;
private const bool use_transition_fav = Gmpc.use_transition;
public static Gmpc.Favorites.List favorites = null;

private const string LOG_DOMAIN_FAV = "Gmpc.Favorites";
namespace Gmpc.Favorites{
    /**
     * This class is created, and stays active until the last GmpcFavoritesButton gets removed
     */
    public class List : GLib.Object {
        private MPD.Data.Item?  list = null; 
        public  bool            disable {get; set; default = false;}
        construct {
            gmpcconn.connection_changed.connect(con_changed);
            gmpcconn.status_changed.connect(status_changed);
            GLib.log(LOG_DOMAIN_FAV, GLib.LogLevelFlags.LEVEL_DEBUG, "Favorites object created");
            if(Gmpc.server.connected) {
                con_changed(gmpcconn, server, 1);
            }
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
                GLib.log(LOG_DOMAIN_FAV, GLib.LogLevelFlags.LEVEL_DEBUG, "Update list");
                list = MPD.Database.get_playlist_content(server, _("Favorites"));
                // Enable plugin.
                disable = false;
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
            if(!disable && (what&MPD.Status.Changed.STORED_PLAYLIST) == MPD.Status.Changed.STORED_PLAYLIST)
            {
                GLib.log(LOG_DOMAIN_FAV, GLib.LogLevelFlags.LEVEL_DEBUG, "Update list");
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
            unowned MPD.Data.Item iter = this.list.get_first();
            while(iter != null)
            {
                if(iter.type == MPD.Data.Type.SONG)
                {
                    if(iter.song.file == path){
                        return true;
                    }
                }
                iter.next(false);
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
            if(!disable && current != favorite)
            {
                if(favorite){
                    /* Add it */
                    MPD.Database.playlist_list_add(server, _("Favorites"), path);
                }else{
                    /* Remove it */
                    /* To be able to remove it we have to first lookup the position */
                    /* This needs libmpd 0.18.1 */
                    unowned MPD.Data.Item iter = this.list.get_first();
                    while(iter != null)
                    {
                        if(iter.type == MPD.Data.Type.SONG)
                        {
                            if(iter.song.file == path){
                                MPD.Database.playlist_list_delete(server, _("Favorites"), iter.song.pos);
                                return;
                            }
                        }
                        iter.next(false);
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
                GLib.error("error: %s\n", e.message);
            }
            if(favorites == null){
                favorites = new List();
                /* make sure favorites is set to NULL again when destroyed */
                favorites.add_weak_pointer(&favorites);
            } else {
                favorites.ref();
            }
            favorites.notify["disable"].connect((source)=>{
                if(favorites.disable) {
                    this.set_sensitive(false);
                }else{
                    this.set_sensitive(true);
                }
            });
            favorites.updated.connect(update);
            this.image = new Gtk.Image();
            this.update(favorites);
            this.add(this.image);
            this.button_press_event.connect(button_press_event_callback);
            this.enter_notify_event.connect(enter_notify_event_callback);
            this.leave_notify_event.connect(leave_notify_event_callback);

        }
        ~Button() {
            if(favorites != null)
                favorites.unref();
        }
        private
        bool
        button_press_event_callback(Gtk.Widget button,Gdk.EventButton event)
        {
            if(event.button == 1 && this.song != null) {
                favorites.set_favorite(this.song.file, !this.fstate);
                this.fstate = !this.fstate;
            }
            else if (event.button == 3 && this.song != null) {
                var menu = new Gtk.Menu();
				int items = 0;
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
					items++;
                }
				if(items > 0)
				{
	                menu.show_all();
    	            menu.popup(null, null, null, event.button, event.time);
				}
				else menu.destroy();
				return true;
			}
            return false;
        }

        /* on mouse over, do some pre-highlighting */
        private
        bool
        enter_notify_event_callback(Gtk.Widget button, Gdk.EventCrossing motion)
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
        leave_notify_event_callback(Gtk.Widget button, Gdk.EventCrossing motion)
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
            this.song = song.copy();
            this.update(favorites);
        }
    }
}
