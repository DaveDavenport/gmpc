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
	private Gtk.Widget status_icon = null;
	/* Config changed signal */
	private bool _config_changed = false;
	private bool config_changed {
		get{
			return _config_changed;
		} set{
			if(profiles.get_current_id() == auto_mpd_id)
			{
				/* Send message about restarting */
				Gmpc.Messages.show(_("Auto MPD's settings have changed, restart MPD to apply changes"), Gmpc.Messages.Level.INFO); 
				var button = new Gtk.Button();
				button.set_label("Restart MPD");
				button.clicked.connect(()=>{ restart_mpd();});
				Gmpc.Messages.add_widget(button);
			}
			_config_changed = value;
		} 
	}
	/**
	 * AutoMPD Options
	 */
	public bool stop_mpd_on_close {
		get{
			return config.get_int_with_default(this.get_name(),
					"Stop On MPD", 1) == 1;
		}
		set{
			config.set_int(this.get_name(), 
					"Stop On MPD",
					(value)?1:0);  
		}
	}
	public bool ao_generate_pulse {
		get{
			return config.get_int_with_default(this.get_name(),
					"Generate Pulse",
					1) == 1;  
		}
		set{
			config.set_int(this.get_name(),
					"Generate Pulse",
					(value)?1:0);  
			config_changed = true;
		}
	}
	public bool ao_generate_alsa {
		get{
			return config.get_int_with_default(this.get_name(),
					"Generate Alsa", 0) == 1;  
		}
		set{
			config.set_int(this.get_name(),
					"Generate Alsa", (value)?1:0);  
			config_changed = true;
		}
	}

	public bool mpd_conf_remote_connections {
		get{
			return config.get_int_with_default(this.get_name(),
					"Allow remote connections", 0) == 1;  
		}
		set{
			config.set_int(this.get_name(),
					"Allow remote connections", (value)?1:0);  
			config_changed = true;
		}
	}
	public bool mpd_conf_generate_httpd_lame {
		get{
			return config.get_int_with_default(this.get_name(),
					"Generate httpd lame", 0) == 1;  
		}
		set{
			config.set_int(this.get_name(),
					"Generate httpd lame", (value)?1:0);  
			config_changed = true;
		}
	}

	public bool mpd_conf_generate_httpd_vorbis {
		get{
			return config.get_int_with_default(this.get_name(),
					"Generate httpd vorbis", 0) == 1;  
		}
		set{
			config.set_int(this.get_name(),
					"Generate httpd vorbis", (value)?1:0);  
			config_changed = true;
		}
	}


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

	/**
	 * Create status icon
	 */
	private bool create_status_icon()
	{
		if(status_icon != null){
			status_icon.show();
			return false;
		}
		status_icon = new Gtk.EventBox();
		var image =  new Gtk.Image.from_icon_name("mpd", Gtk.IconSize.MENU); 
		(status_icon as Gtk.Container).add(image);
		status_icon.show_all();
		status_icon.set_no_show_all(true);
		status_icon.set_tooltip_text(_("Local (started by GMPC) MPD is running"));
		Gmpc.Playlist3.add_status_icon(status_icon);

		status_icon.button_release_event.connect((source, event)=>{
			if(event.button == 1) {
				Gmpc.Preferences.show(this.id);

			}
			return false;
		});
		return false;	
	}

	/**
	 * Hide the status icon
	 */
	private void hide_status_icon()
	{
		if(status_icon == null) return;
		status_icon.hide();
	}

	/****************************************************************************
	 * START, STOPPING,  CHECKING MPD											*
	 ****************************************************************************/
	private void restart_mpd()
	{
		stop_mpd();
		GLib.Timeout.add(500,()=>{
				GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG, 
					"Trying to restart GMPC");
				// Check if MPD is stopped, when it is, start it again
				if(!check_mpd())
				{
					start_mpd();
					/* we are disconnected, check if it was from our MPD */
					if(profiles.get_current_id() == auto_mpd_id)
					{
						Gmpc.MpdInteraction.connect();
					}
					return true;
				}
				return false;
		});
	}
	/**
	 * Start MPD
	 */
	private void start_mpd()
	{
		if(check_mpd()){
			GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Not starting MPD, allready running");
			// create status icon.
			GLib.Idle.add(create_status_icon);
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
		if(check_mpd())
		{
			// Create status icon.
			GLib.Idle.add(create_status_icon);
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
			// destroy status icon 
			hide_status_icon();
			return;
		}
		GLib.log(log_domain_autompd, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Stopping MPD");

		var c_dir = GLib.Environment.get_user_cache_dir();
		var full_path = GLib.Path.build_filename(c_dir,"gmpc", auto_mpd_id, "mpd.conf");
		try{
			GLib.Process.spawn_command_line_sync("mpd --kill '%s'".printf(full_path));
			// destroy status icon
			hide_status_icon();
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
	private void create_config_generate_pulse(GLib.FileStream? fp)
	{
		fp.printf("# Pulse audio output\n");
		fp.printf("audio_output {\n");
		fp.printf("	type	\"pulse\"\n");
		fp.printf("	name	\"AutoMPD pulse device\"\n");
		fp.printf("}\n\n");
	}
	/**
	 * Create the config file MPD should use.
	 * This should be re-created every time
	 */
	private void create_config_generate_alsa(GLib.FileStream? fp)
	{
		fp.printf("# Alsa audio output\n");
		fp.printf("audio_output {\n");
		fp.printf("	type	\"alsa\"\n");
		fp.printf("	name	\"AutoMPD alsa device\"\n");
		fp.printf("}\n\n");
	}
	private void create_config_generate_http_lame(GLib.FileStream? fp)
	{
		fp.printf("# HTTP mp3audio output\n");
		fp.printf("audio_output {\n");
		fp.printf("	type		\"httpd\"\n");
		fp.printf("	name		\"AutoMPD httpd-mp3 device\"\n");
		fp.printf(" encoder		\"lame\"\n");
		fp.printf(" port		\"8000\"\n");
		fp.printf(" bitrate		\"128\"\n");
		fp.printf(" format		\"44100:16:2\"\n");
		fp.printf("}\n\n");
	}
	private void create_config_generate_http_vorbis(GLib.FileStream? fp)
	{
		fp.printf("# HTTP mp3audio output\n");
		fp.printf("audio_output {\n");
		fp.printf("	type		\"httpd\"\n");
		fp.printf("	name		\"AutoMPD httpd-ogg device\"\n");
		fp.printf(" encoder		\"vorbis\"\n");
		fp.printf(" port		\"8001\"\n");
		fp.printf(" bitrate		\"128\"\n");
		fp.printf(" format		\"44100:16:2\"\n");
		fp.printf("}\n\n");
	}
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
		fp.printf("music_directory \"%s\"\n\n", md);

		/* Playlist directory */
		var pl_dir = GLib.Path.build_filename(full_path, "playlists");
		fp.printf("# The directory MPD will use for internal playlists\n");
		fp.printf("playlist_directory \"%s\"\n\n", pl_dir);

		/* Log */
		var log_file = GLib.Path.build_filename(full_path, "log","log.txt");
		fp.printf("# MPD log file\n");
		fp.printf("log_file \"%s\"\n\n", log_file);

		/* DB file*/
		var database_file = GLib.Path.build_filename(full_path, "database.txt");
		fp.printf("# MPD database file\n");
		fp.printf("db_file \"%s\"\n\n", database_file);

		/* State file*/
		var state_file = GLib.Path.build_filename(full_path, "state.txt");
		fp.printf("# MPD state file\n");
		fp.printf("state_file \"%s\"\n\n", state_file);

		/* Bind to address */
		var hostname = profiles.get_profile_hostname(auto_mpd_id);
		fp.printf("# Host to bind to\n");
		fp.printf("bind_to_address  \"%s\"\n\n", hostname);
		if(mpd_conf_remote_connections)
		{
			fp.printf("# Allow remote connections \n");
			fp.printf("bind_to_address  \"any\"\n\n");
		}

		/* Write pid file, so we can use mpd --kill */
		var pid_file = GLib.Path.build_filename(full_path, "mpd.pid");
		fp.printf("# MPD pid file\n");
		fp.printf("pid_file \"%s\"\n\n", pid_file);

		/* audio output, default to pulse */
		if(ao_generate_pulse)
		{
			create_config_generate_pulse(fp);      
		}
		/* audio output, default to alsa */
		if(ao_generate_alsa)
		{
			create_config_generate_alsa(fp);      
		}
		if(mpd_conf_generate_httpd_lame)
		{
			create_config_generate_http_lame(fp);
		}
		if(mpd_conf_generate_httpd_vorbis)
		{
			create_config_generate_http_vorbis(fp);
		}
	}

	/**
	 * Check if we found an MPD binary to use
	 */
	inline bool have_mpd_binary()
	{
		return (mpd_path != null);
	}

	/**
	 * Look through the paths, and try to find the MPD binary.
	 */
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

	/**
	 * Check lif a profile exists for the local MPD.
	 */
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

	/**
	 * Constructor ()
	 */
	construct {

        // and internal plugin.
        this.plugin_type = 8;

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
			gmpcconn.connection_changed.connect(connection_changed);
		}
		this.notify["config_changed"].connect(()=>{
				});
	}

	/**
	 * Handle changing profiles.
	 *
	 * If the user changes the profile to us, start_mpd().
	 * If he moves away, stop_mpd().	
	 */
	private void current_profile_changed(Profiles prof, string id)
	{
		if(!this.get_enabled()) return;
		if(have_mpd_binary())
		{
			if(id == auto_mpd_id) {
				start_mpd();
			}else{
				stop_mpd();
			}
		}
	}

	/**
	 * Handle a connection changed.
	 * If we get disconnected, check if mpd is still running, if not.
	 * Remove the status icon
	 */
	private void connection_changed(Connection gc, MPD.Server server, int connection)
	{
		if(connection == 0)
		{
			/* we are disconnected, check if it was from our MPD */
			if(profiles.get_current_id() == auto_mpd_id)
			{
				/* check if mpd is still runing */
				if(!check_mpd())
				{
					/* not running, update status icon */
					hide_status_icon();
					// Restart MPD!!
					start_mpd();
				}
			}
		}
		else
		{
			/* we are disconnected, check if it was from our MPD */
			if(profiles.get_current_id() == auto_mpd_id)
			{
				/* check if mpd is still runing */
				if(check_mpd())
				{
					/* not running, update status icon */
					create_status_icon();
				}
			}
		}
	}
	/***
	 * Destructor
	 */
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
	private Gtk.Builder pref_builder = null;

	/* Destroy preferences construct */
	public void preferences_pane_construct(Gtk.Container container)
	{
		if(pref_builder != null) return;
		pref_builder = new Gtk.Builder(); 
		try {
			pref_builder.add_from_file(Gmpc.data_path("preferences-autompd.ui"));
		}catch (GLib.Error e)
		{
			GLib.error("Failed to load GtkBuilder file: %s", e.message);
		}

		var b = pref_builder.get_object("autompdcontainer") as Gtk.Widget;
		container.add(b);

		// Close mpd on close.
		var smoc = pref_builder.get_object("cb_stop_mpd_on_close") as Gtk.CheckButton;
		smoc.active = stop_mpd_on_close;
		smoc.toggled.connect((source)=> {
				stop_mpd_on_close = source.active;
				});

		// Generate pulse output 
		var cpo = pref_builder.get_object("cb_pulse_output") as Gtk.CheckButton;
		cpo.active = ao_generate_pulse;
		cpo.toggled.connect((source)=> {
				ao_generate_pulse = source.active;
				});

		// Generate alsa output 
		var cao = pref_builder.get_object("cb_alsa_output") as Gtk.CheckButton;
		cao.active = ao_generate_alsa;
		cao.toggled.connect((source)=> {
				ao_generate_alsa = source.active;
				});

		// Allow remote connections 
		var arc = pref_builder.get_object("cb_allow_remote_connections") as Gtk.CheckButton;
		arc.active = mpd_conf_remote_connections;
		arc.toggled.connect((source)=> {
				mpd_conf_remote_connections = source.active;
		});

		var auhv = pref_builder.get_object("cb_audio_output_httpd_vorbis") as Gtk.CheckButton;
		auhv.active = mpd_conf_generate_httpd_vorbis;
		auhv.toggled.connect((source)=>{
				mpd_conf_generate_httpd_vorbis = source.active;
		});

		var auhl = pref_builder.get_object("cb_audio_output_httpd_lame") as Gtk.CheckButton;
		auhl.active = mpd_conf_generate_httpd_lame;
		auhl.toggled.connect((source)=>{
				mpd_conf_generate_httpd_lame = source.active;
		});

		// Restart button
		var rb = pref_builder.get_object("b_restart") as Gtk.Button;
		rb.sensitive = have_mpd_binary();
		rb.clicked.connect((source)=>{
				restart_mpd();
				});


		var hb = pref_builder.get_object("autompd_help_button") as Gtk.Button;
		hb.clicked.connect((source)=>{
			Gmpc.open_help("ghelp:gmpc?AutoMPD");	
			});
		b.show_all();
	}
	/* Destroy preferences pane */
	public void preferences_pane_destroy(Gtk.Container container)
	{
		container.remove((container as Gtk.Bin).get_child());
		pref_builder = null;
	}
}
