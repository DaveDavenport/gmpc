/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
 * Copyright (C) 2012 Quentin "Sardem FF7" Glidic
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

private const string some_unique_name_prfs = Config.VERSION;

namespace Gmpc
{
    private class Profile
    {
        public string id              = null;
        public string name            = null;
        public string hostname        = "localhost";
        public int    port            = 6600;
        public bool   do_auth         = false;
        public string password        = "";
        public string music_directory = null;
        public int    db_update_time  = 0;
    }

    public class Profiles : GLib.Object
    {
        const string LOG_DOMAIN = "Profiles";

        public enum Action
        {
            ADDED,
            REMOVED,
            COL_CHANGED,
            CURRENT
        }

        public enum Column
        {
            NAME,
            HOSTNAME,
            PORT,
            DO_AUTH,
            PASSWORD,
            MUSIC_DIRECTORY,
            DB_UPDATE_TIME,
            NUM_COLS
        }

        private Settings profiles;
        private GLib.HashTable<string, Profile> list;


        public
        uint
        get_number_of_profiles()
        {
            return this.list.size();
        }

        /* Connect on button press event */
        private
        void
        connect_to_profile_button(Gtk.Button button)
        {
            string id = button.get_data("profile-id");
            if(id != null)
            {
                this.set_current(id);
                MpdInteraction.disconnect();
                MpdInteraction.connect();
            }
        }

        public signal void changed(int changed, int col, string id);
        void
        changed_internal(Action changed, Column col, string id)
        {
            switch (changed)
            {
                case Action.ADDED:
                    var connect_button = new Gtk.Button.from_stock(Gtk.Stock.CONNECT);
                    string message = GLib.Markup.printf_escaped("<b>%s:</b> '%s'", _("Added profile"), this.get_name(id));
                    Gmpc.Messages.show(message, Gmpc.Messages.Level.INFO);

                    connect_button.set_data_full("profile-id", id, GLib.free);
                    connect_button.clicked.connect(this.connect_to_profile_button);
                    Gmpc.Messages.add_widget(connect_button);

                    GLib.log(LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG,"Item %s added\n", id);
                    break;
                case Action.REMOVED:
                    GLib.log(LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG,"Item %s removed\n", id);
                    break;
                case Action.COL_CHANGED:
                    /* Disabled because of grand spamming charges */
                    /*
                    stringmessage = GLib.Markup.printf_escaped("<b>%s:</b> '%s'", _("Changed profile"),this.get_name(id));
                    Gmpc.Messages.show(message, ERROR_INFO);
                    q_free(message);
                    */
                    GLib.log(LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG,"Item %s changed col: %i\n", id, col);
                    break;
                case Action.CURRENT:
                    break;
            }
            this.changed(changed, col, id);
        }


        construct
        {
            this.list = new GLib.HashTable<string, Profile>(GLib.str_hash, GLib.str_equal);
            string url;
            /**
             * Get Profile
             */
            url = Gmpc.user_path("profiles.cfg");
            this.profiles = new Gmpc.Settings.from_file(url);
            if(this.profiles == null)
            {
                /**
                 * Show gtk error message and quit
                 */
                GLib.log(LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_ERROR,"Failed to save/load Profile file:\n%s\n",url);
            }
            this.load_from_config();
            if(this.list.size() == 0)
            {
                this.add_default();
                this.load_from_config();
            }
        }

        /**
         * Add default values
         */
        private
        void
        add_default()
        {
            Gmpc.config.set_string("connection", "currentprofile", "Default");
            this.profiles.set_string("Default", "hostname", "localhost");
            this.profiles.set_string("Default", "name", "Default");
            this.profiles.set_string("Default", "password", "");
            this.profiles.set_int("Default", "portnumber", 6600);
            this.profiles.set_bool("Default", "useauth", false);
            this.profiles.set_string("Default", "music directory", "");
        }

        private
        void
        load_from_config()
        {
            SettingsList list = this.profiles.get_class_list();
            for (unowned SettingsList iter = list ; iter != null ; iter = iter.next)
            {
                Profile prof = new Profile();

                prof.id = iter.key;
                prof.name = this.profiles.get_string(prof.id, "name");
                prof.hostname = this.profiles.get_string(prof.id, "hostname");
                prof.password = this.profiles.get_string(prof.id, "password");
                prof.port = this.profiles.get_int(prof.id, "portnumber");
                prof.do_auth = this.profiles.get_bool(prof.id, "useauth");

                prof.music_directory = this.profiles.get_string(prof.id, "music directory");
                prof.db_update_time = this.profiles.get_int_with_default(prof.id, "db update time", 0);

                this.list.insert(prof.id, prof);
            }
        }

        /**
         * get hostname
         */
        public
        string?
        get_hostname(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return null;
            return prof.hostname;
        }
        /**
         * get name
         */
        public
        unowned string?
        get_name(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return null;
            return prof.name;
        }
        /**
         * get id
         */
        public
        unowned string?
        get_id(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return null;
            return prof.id;
        }
        /**
         * get password
         */
        public
        string?
        get_password(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return null;
            return prof.password;
        }

        /**
         * get music directory
         */
        public
        unowned string?
        get_music_directory(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return null;
            return prof.music_directory;
        }
        /**
         * get music directory
         */
        public
        int
        get_db_update_time(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return 0;
            return prof.db_update_time;
        }
        /**
         * get port
         */
        public
        int
        get_port(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return -1;
            return prof.port;
        }

        /**
         * get do_auth
         */
        public
        bool
        get_do_auth(string id)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null)
                return false;
            return prof.do_auth;
        }

        /**
        * Create new item
        */
        public
        unowned string
        create_new_item(string id)
        {
            return this.create_new_item_with_name(id, "New Profile");
        }

        public
        unowned string
        create_new_item_with_name(string? id, string? name)
        {
            var prof = new Profile();
            if(id == null)
                prof.id = GLib.Random.next_int().to_string();
            else
                prof.id = id;
            if(name == null)
                prof.name = "New Profile";
            else
                prof.name = name;

            /* safe this to the config file */
            this.profiles.set_string(prof.id, "name", prof.name);
            this.profiles.set_string(prof.id, "hostname", prof.hostname);
            this.profiles.set_string(prof.id, "password", prof.password);
            this.profiles.set_int(prof.id, "portnumber", prof.port);
            this.profiles.set_bool(prof.id, "useauth", prof.do_auth);
            this.profiles.set_string(prof.id, "music directory", prof.music_directory);

            this.profiles.get_int_with_default(prof.id, "db update time",(int)(prof.db_update_time));

            this.list.insert(prof.id, prof);

            /* propagate */
            this.changed_internal(Action.ADDED, Column.NUM_COLS, prof.id);
            return prof.id;
        }

        public
        void
        remove_item(string id)
        {
            string message = null;
            if(!this.has_profile(id))
            {
                GLib.log(LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_WARNING, "Trying to remove not-existing profile: %s\n", id);
                return;
            }
            /* Generate removal message before the actual profile is destroyed */
            message = GLib.Markup.printf_escaped("<b>%s:</b> '%s'", _("Removed profile"),this.get_name(id));

            this.list.remove(id);

            /* Display the message */
            Gmpc.Messages.show(message, Gmpc.Messages.Level.INFO);

            this.changed_internal(Action.REMOVED, Column.NUM_COLS, id);
        }

        /**
        * GET CURRENT
        */
        public
        string
        get_current()
        {
            var id = Gmpc.config.get_string_with_default("connection", "currentprofile", "Default");
            Profile prof = this.list.lookup(id);
            /* if not available get the first one */
            if(prof == null)
            {
                if(this.list.size() > 0)
                {
                    var iter = GLib.HashTableIter<string, Profile>(this.list);
                    iter.next(out id, out prof);
                    this.set_current(id);
                }
                else
                {
                    this.add_default();
                    this.load_from_config();
                    this.changed_internal(Action.ADDED, Column.NUM_COLS, "Default");
                }
                id = Gmpc.config.get_string_with_default("connection", "currentprofile", "Default");
            }
            return id;
        }

        public signal void profile_changed(string id);
        public void
        set_current(string id)
        {
            if(this.has_profile(id))
            {
                Gmpc.config.set_string("connection", "currentprofile", id);

                this.changed_internal(Action.CURRENT, 0, id);
            }
            this.profile_changed(id);
        }

        public
        GLib.List<weak string>
        get_profiles_ids()
        {
            return this.list.get_keys();
        }

        /**
         * set field
         */
        public
        void
        set_name(string id, string @value)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null) return;

            if(@value == prof.name) return;
            prof.name = @value;

            this.profiles.set_string(id, "name", prof.name);
            this.changed_internal(Action.COL_CHANGED, Column.NAME, id);
        }

        public
        void
        set_hostname(string id, string @value)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null) return;

            if(@value == prof.hostname) return;
            prof.hostname = @value;

            this.profiles.set_string(id, "hostname", prof.hostname);
            this.changed_internal(Action.COL_CHANGED, Column.HOSTNAME, id);
        }

        /**
         * Set Password
         */
        public
        void
        set_password(string id, string? @value)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null) return;

            if(@value == prof.password) return;
            prof.password = (@value != null) ? @value : "";

            this.profiles.set_string(id, "password", prof.password);
            this.changed_internal(Action.COL_CHANGED, Column.PASSWORD, id);
        }

        /**
         * Set Port
         */
        public
        void
        set_port(string id, int @value)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null) return;

            if(value == prof.port) return;
            prof.port = @value;

            this.profiles.set_int(id, "portnumber",prof.port);
            this.changed_internal(Action.COL_CHANGED, Column.PORT, id);
        }

        /**
         * Set music directory
         */
        public
        void
        set_music_directory(string id, string @value)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null) return;

            if(@value == prof.music_directory) return;
            prof.music_directory = (@value != null) ? @value : "";

            this.profiles.set_string(id, "music directory", prof.music_directory);
            this.changed_internal(Action.COL_CHANGED, Column.MUSIC_DIRECTORY,id);
        }


        public
        void
        set_db_update_time(string id, int @value)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null) return;

            if(@value == prof.db_update_time) return;
            prof.db_update_time = @value;

            this.profiles.set_int(id, "db update time", @value);
            this.changed_internal(Action.COL_CHANGED, Column.DB_UPDATE_TIME, id);
        }

        /**
         * Set do auth
         */
        public
        void
        set_do_auth(string id, bool @value)
        {
            Profile prof = this.list.lookup(id);
            if(prof == null) return;

            if(prof.do_auth == @value) return;
            prof.do_auth = @value;

            this.profiles.set_bool(id, "useauth", prof.do_auth);
            this.changed_internal(Action.COL_CHANGED, Column.DO_AUTH, id);
        }

        /**
         * Has profile with id
         */
        public
        bool
        has_profile(string id)
        {
            return ( this.list.lookup(id) != null );
        }

        public
        void
        set_profile_from_name(string name)
        {
            var prof = this.list.find((id, prof) =>
            {
                return (name == prof.name);
            });
            MpdInteraction.set_current_profile(prof.id);
        }
    }
}
