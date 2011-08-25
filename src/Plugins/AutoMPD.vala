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

/**
 * This plugin queries HTBackdrops.com for artist images (backdrops)
 */

using Config;
using Gmpc;
using Gmpc.Plugin;

private const bool use_transition_autompd = Gmpc.use_transition;
private const string some_unique_name_autompd = Config.VERSION;
private const string log_domain_autompd = "Gmpc.Plugins.AutoMPD";

public class Gmpc.Plugins.AutoMPD:
            Gmpc.Plugin.Base,
            Gmpc.Plugin.PreferencesIface
{
	private const string auto_mpd_id = "autompdprofileid834724";
    private string mpd_path = null;
	private bool stop_mpd_on_close = true;
	private bool ao_generate_pulse = true;
    /**
     * Gmpc.Plugin.Base
     */
    private const int[] version = {0,0,2};

    /** Return the plugin version. For an internal plugin this is not that interresting.
     * But we implement it anyway
     */
    public override unowned int[] get_version()
    {
        return this.version;
    }

    /**
     * The name of the plugin
     */
    public override unowned string get_name()
    {
        return N_("Automatic MPD");
    }

	/****************************************************************************
	 * START, STOPPING,  CHECKING MPD											*
	 ****************************************************************************/
	/**
	 * Start MPD
	 */
    private void start_mpd()
    {
		if(check_mpd()){
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Not starting MPD, allready running");
			return;
		}
		GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Starting MPD");
		create_config_file();

		var c_dir = GLib.Environment.get_user_cache_dir();
		var full_path = GLib.Path.build_filename(c_dir,"gmpc", auto_mpd_id, "mpd.conf");
		try{
			GLib.Process.spawn_command_line_sync("mpd '%s'".printf(full_path));
		}catch(SpawnError e) {
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_WARNING,
				"Failed to start mpd: %s\n", e.message);
		}
    }
	/**
	 * Stop MPD
	 * This is done by the mpd --kill command
	 */
    private void stop_mpd()
    {
		if(!check_mpd()){
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Not stopping MPD, allready stopped.");
			return;
		}
		GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Stopping MPD");

		var c_dir = GLib.Environment.get_user_cache_dir();
		var full_path = GLib.Path.build_filename(c_dir,"gmpc", auto_mpd_id, "mpd.conf");
		try{
			GLib.Process.spawn_command_line_sync("mpd --kill '%s'".printf(full_path));
		}catch(SpawnError e) {
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_WARNING,
				"Failed to stop mpd: %s\n", e.message);
		}
    }
	/**
	 * Check if MPD is running
	 * First we read the pid file.
	 * Then check if this pid is the MPD we started.
	 */
	private bool check_mpd()
	{
		string contents = null;
		size_t length = 0;
		var c_dir = GLib.Environment.get_user_cache_dir();
		var full_path = GLib.Path.build_filename(c_dir,"gmpc", auto_mpd_id, "mpd.pid");
		try{
			GLib.FileUtils.get_contents(full_path, out contents, out length);
			int pd = int.parse(contents);
			// TODO: This will not work on windows.
			// Check proc if this pid is running.
			var filename = GLib.Path.build_filename(GLib.Path.DIR_SEPARATOR_S, "proc", "%d".printf(pd), "cmdline");
			GLib.FileUtils.get_contents(filename, out contents, out length);
			if(contents == "mpd") {
				if(length > 4 && contents[3] == '\0') {
					// Vala trick to get to 2nd element
					unowned char *test = (char *)contents;
					unowned string? snd = (string)(&test[4]);
					stdout.printf("Path is: %s\n", snd);
					if(snd == GLib.Path.build_filename(c_dir,"gmpc", auto_mpd_id, "mpd.conf"))
					{
						/* MPD is running */
						return true;
					}
				}
			}
			return false;
		}catch(FileError e) {
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Error getting MPD pid: %s", e.message);
			return false;
		}
	}



	/**
	 * Create the config file MPD should use.
	 * This should be re-created every time
	 */
	private void create_config_file()
	{
		/* TODO: Check if profile exists */
		if(profiles.get_id(auto_mpd_id) == null) {
			/* no profile exists */
			return;
		}
		var c_dir = GLib.Environment.get_user_cache_dir();
		var full_path = GLib.Path.build_filename(c_dir,"gmpc", auto_mpd_id);
		var conf_file = GLib.Path.build_filename(full_path, "mpd.conf");
		GLib.FileStream? fp = GLib.FileStream.open(conf_file, "w");

		/* music directory */
		var md = profiles.get_music_directory(auto_mpd_id);
		fp.printf("# The directory MPD will search to discover music\n");
		fp.printf("music_directory \"%s\"\n", md);

		/* Playlist directory */
		var pl_dir = GLib.Path.build_filename(full_path, "playlists");
		fp.printf("# The directory MPD will use for internal playlists\n");
		fp.printf("playlist_directory \"%s\"\n", pl_dir);

		/* Log */
		var log_file = GLib.Path.build_filename(full_path, "log","log.txt");
		fp.printf("# MPD log file\n");
		fp.printf("log_file \"%s\"\n", log_file);

		/* DB file*/
		var database_file = GLib.Path.build_filename(full_path, "database.txt");
		fp.printf("# MPD database file\n");
		fp.printf("db_file \"%s\"\n", database_file);

		/* State file*/
		var state_file = GLib.Path.build_filename(full_path, "state.txt");
		fp.printf("# MPD state file\n");
		fp.printf("state_file \"%s\"\n", state_file);

		/* Bind to address */
		var hostname = profiles.get_profile_hostname(auto_mpd_id);
		fp.printf("# Host to bind to\n");
		fp.printf("bind_to_address  \"%s\"\n", hostname);

		/* Write pid file, so we can use mpd --kill */
		var pid_file = GLib.Path.build_filename(full_path, "mpd.pid");
		fp.printf("# MPD pid file\n");
		fp.printf("pid_file \"%s\"\n", pid_file);

		/* audio output, default to pulse */
		if(ao_generate_pulse)
		{
			fp.printf("audio_output {\n");
			fp.printf("	type	\"pulse\"\n");
			fp.printf("	name	\"AutoMPD pulse device\"\n");
			fp.printf("}\n");
		}
	}
	/**
	 * Check if we found an MPD binary to use
	 */
	inline bool have_mpd_binary()
	{
		return (mpd_path != null);
	}
    private void find_mpd_binary()
    {
        var path = GLib.Environment.get_variable("PATH");
        foreach(var str in path.split(":"))
        {
            var total_path = GLib.Path.build_filename(str, "mpd");
            if ( GLib.FileUtils.test(total_path,GLib.FileTest.IS_EXECUTABLE))
            {
                mpd_path = total_path;
                GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
                        "Found MPD Path: %s", mpd_path);
                return;
            }
        }
        GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
                "No MPD found in path.");
    }
	private void check_local_profile()
	{
		var c_dir = GLib.Environment.get_user_cache_dir();
		var full_path = GLib.Path.build_filename(c_dir,"gmpc", auto_mpd_id);

		/* Check if profile exists */
		if(profiles.get_id(auto_mpd_id) == null)
		{
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"AutoMPD profile not found: %s", auto_mpd_id);
			/* Create the profile */
			/* FIXME: This exposes a bug in playlist3-messages */
			profiles.create_new_item_with_name(auto_mpd_id, _("Auto MPD"));
			/* set hostname to socket*/
			profiles.set_hostname(auto_mpd_id,
					GLib.Path.build_filename(full_path, "socket"));
			/* set music directory */
			var md = GLib.Environment.get_user_special_dir(GLib.UserDirectory.MUSIC);
			if(md != null) {
				profiles.set_music_directory(auto_mpd_id, md);
			}else{
				// sane fallback?
				profiles.set_music_directory(auto_mpd_id, "~/Music");
			}
		}

		/* Check if MPD directory exists */
		if(!GLib.FileUtils.test(full_path, GLib.FileTest.IS_DIR))
		{
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"AutoMPD directory not found: %s", full_path);
			GLib.DirUtils.create_with_parents(full_path, 0755);
		}
		/* Log dir */
		var log_dir = GLib.Path.build_filename(full_path, "log");
		if(!GLib.FileUtils.test(log_dir, GLib.FileTest.IS_DIR))
		{
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"AutoMPD directory not found: %s", log_dir);
			GLib.DirUtils.create_with_parents(log_dir, 0755);
		}

		/* playlists dir */
		var playlists_dir = GLib.Path.build_filename(full_path, "playlists");
		if(!GLib.FileUtils.test(playlists_dir, GLib.FileTest.IS_DIR))
		{
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"AutoMPD directory not found: %s", playlists_dir);
			GLib.DirUtils.create_with_parents(playlists_dir, 0755);
		}
	}

	/* Handle changing profiles. */
	private void current_profile_changed(Profiles prof, string id)
	{
		if(have_mpd_binary())
		{
			if(id == auto_mpd_id) {
				start_mpd();
			}else{
				stop_mpd();
			}
		}
	}

	// Constructor
    construct {
		// Check for the MPD binary
        find_mpd_binary();

		// If binary is found, check profile and add connection.
		if(have_mpd_binary())
		{
			check_local_profile();
			// For testing
			create_config_file();
			profiles.set_current.connect(current_profile_changed);
			// Start mpd if current profile is AutoMPD profile.
			current_profile_changed(profiles, profiles.get_current_id());
		}
	}
	/* Destructor */
	~AutoMPD()
	{
		/* Close mpd if desired */
		if(have_mpd_binary()){
			if(stop_mpd_on_close)
				stop_mpd();
		}
	}
    /**
     * Preferences pane
     */
     public void preferences_pane_construct(Gtk.Container container)
     {

     }
     public void preferences_pane_destroy(Gtk.Container container)
     {

     }
}
