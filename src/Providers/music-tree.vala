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
 * Artist art:
 *          Looks file: ^(<artist>|artist)\.(jpg|png|jpeg|gif)$;
			In directory:  <path>
						   <path>/../
						   <path>/../../
 * Artist txt:
 *          Looks file: ^BIOGRAPHY(\.txt)?$;
			In directory:  <path>
						   <path>/../
						   <path>/../
 * Album art:
 *          Looks file: ^(folder|cover|.*<album>.*)\.(jpg|png|jpeg|gif);
			In directory:  <path>
						   <path>/../
						   <path>/../
 * Album txt:
			Looks file: ^(<album>)\.(info|txt)$
			In directory:  <path>
						   <path>/../
						   <path>/../
 */
using Config;
using Gtk;
using Gmpc;
using Gmpc.Plugin;

private const bool use_transition_lp = Gmpc.use_transition;
private const string some_unique_name_lp = Config.VERSION;
private const string log_domain_cp = "Gmpc.Providers.MusicTree";

public class Gmpc.Provider.MusicTree : Gmpc.Plugin.Base, Gmpc.Plugin.MetaDataIface
{
	private const int[] version = {0,0,2};
	private string file_type_string = "(jpg|jpeg|png|gif)";

	public override unowned int[] get_version()
	{
		return this.version;
	}

	public override unowned string get_name()
	{
		return N_("Music Tree Provider");
	}

	construct
	{
		this.plugin_type = 8+32;
		/* Todo get list from gdk? */

	}

	public void set_priority(int priority)
	{
		config.set_int(this.get_name(),"priority",priority);
	}
	public int get_priority()
	{
		return config.get_int_with_default(this.get_name(),"priority",0);
	}

	/********************************
	 * metadata code                *
	 ********************************/
	public void get_metadata(MPD.Song song,
		Gmpc.MetaData.Type type,
		MetaDataCallback callback)
	{
		string id =  profiles.get_current_id();
		unowned string directory = null;
		if( id != null )
		{
			directory = profiles.get_music_directory(id);
		}

		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Got directory: %s", (directory == null)?"(null)":directory);

		if(directory == null || directory.length == 0)
		{
			log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
				"No Music directory specified");
			/* With no directory specified, I cannot find anything */
			/* Signal this back */
			callback(null);
			/* and stop trying */
			return;
		}

		/* Check song path */
		if(song == null || song.file == null)
		{
			log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
				"No Song or no song path specified");
			callback(null);
			/* and stop trying */
			return;
		}

		/* Check if it is a db file or not */
		string scheme = GLib.Uri.parse_scheme(song.file);
		if(scheme != null)
		{
			log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Scheme '%s' indicates no local file.",scheme);
			callback(null);
			/* and stop trying */
			return;
		}
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Music directory: %s", directory);

		/**
		 * Handle the different metadata types
		 */
		switch(type)
		{
			case Gmpc.MetaData.Type.ALBUM_ART:
				this.get_album_cover(directory, song, callback);
				return;
			case Gmpc.MetaData.Type.ARTIST_ART:
				this.get_artist_art(directory, song, callback);
				return;
			case Gmpc.MetaData.Type.ARTIST_TXT:
				this.get_biography(directory, song, callback);
				return;
			case  Gmpc.MetaData.Type.ALBUM_TXT:
				this.get_album_info(directory,song,callback);
				return;
				/*  Not yet supported  */
			case  Gmpc.MetaData.Type.SONG_TXT:
			case  Gmpc.MetaData.Type.SONG_GUITAR_TAB:
				break;
				/*  Unsupported by this type */
			case  Gmpc.MetaData.Type.ARTIST_SIMILAR:
			case  Gmpc.MetaData.Type.SONG_SIMILAR:
			case  Gmpc.MetaData.Type.GENRE_SIMILAR:
				break;
				/*  To stop the compiler */
			case  Gmpc.MetaData.Type.QUERY_DATA_TYPES:
			case  Gmpc.MetaData.Type.QUERY_NO_CACHE:
			default:
				break;
		}

		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Unsupported metadata, doing nothing ");
		/* Tell that we found nothing */
		callback(null);
	}

	/**
	 * Helper functions
	 */
	private async void check_directory_for_files(
		out List<MetaData.Item> list,
		GLib.File dir,
		GLib.Regex query,
		Gmpc.MetaData.Type type)
	{
        var path = dir.get_path();
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Checking %s with pattern: %s",path,
			query.get_pattern());
		/* Start async directory walking */
		try
		{
			var e = yield dir.enumerate_children_async ("standard,access::*", 0, Priority.DEFAULT, null);
			while (true)
			{
				var files = yield e.next_files_async (10, Priority.DEFAULT, null);
				if (files == null)
				{
					break;
				}
				foreach (var info in files)
				{
					string name = info.get_name();
					log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
						"got file: %s", name);
					if(name[0] != '.' && query.match(name, 0, null))
					{
						log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
							"Got path: %s. Have info", name);
						if(info.get_attribute_boolean("access::can-read"))
						{
							MetaData.Item item = new MetaData.Item();
							item.type = type;
							item.plugin_name = this.get_name();
							item.content_type = MetaData.ContentType.URI;
							item.set_uri(GLib.Path.build_filename(path,name));
							list.append((owned)item);
							log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
								"Found %s/%s, adding",path,name);
							/* Match filename against rules */
						}
					}
				}
			}
		}
		catch (GLib.Error err)
		{
			log(log_domain_cp, GLib.LogLevelFlags.LEVEL_WARNING,
				"Error trying to walk directory '%s': %s\n",path, err.message);
		}
	}
	/**
	 * Walks back the directory 3 levels.
	 */
	private async void walk_back_directory(GLib.Regex regex_query,
		string mpd_directory,
		string filename,
		out List<Gmpc.MetaData.Item> list,
		Gmpc.MetaData.Type type)
	{
		string base_path = GLib.Path.get_dirname(filename);
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Got basename: %s", base_path);
		/* Start looking */
		string path = GLib.Path.build_filename(mpd_directory, base_path);
		var dir = File.new_for_path (path);
		List<MetaData.Item> temp = null;
		yield check_directory_for_files(out temp, dir, regex_query,type);
		list.concat((owned)temp);
		/* one directory up */
		dir = dir.get_parent();
		if(dir != null)
		{
			yield check_directory_for_files(out temp, dir, regex_query,type);
			list.concat((owned)temp);
			/* one directory up */
			dir = dir.get_parent();
			if(dir != null)
			{
				yield check_directory_for_files(out temp, dir, regex_query,type);
				list.concat((owned)temp);
			}
		}
	}
	/**
	 * Get artist art
	 */
	private async void get_artist_art(string directory, MPD.Song song, MetaDataCallback callback)
	{
		List<Gmpc.MetaData.Item> list = null;
        log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query artist art ");

		// Create search query
		GLib.Regex regex_query = null;
		if(song.artist != null)
		{
			try
			{
				regex_query = new GLib.Regex(
					"^(artist|%s)\\.%s$".printf(GLib.Regex.escape_string(song.artist),file_type_string),
					GLib.RegexCompileFlags.CASELESS|GLib.RegexCompileFlags.DOTALL,
					0);
			}
			catch  (Error erro)
			{
				log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Failed to create regex: %s",erro.message);
				callback(null);
				return;
			}
		}
		else
		{
			try
			{
				regex_query = new GLib.Regex(
					"^(artist)\\.%s$".printf(file_type_string),
					GLib.RegexCompileFlags.CASELESS|GLib.RegexCompileFlags.DOTALL,
					0);
			}
			catch (Error err1)
			{
				log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Failed to create regex: %s",err1.message);
				callback(null);
				return;
			}
		}
		yield walk_back_directory(regex_query, directory, song.file,
			out list, Gmpc.MetaData.Type.ARTIST_ART);
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query done, %u results", list.length());
		list.first();
		callback((owned)list);
		return ;
	}
	/**
	 * Get album cover
	 */
	private async void get_album_cover(string directory, MPD.Song song, MetaDataCallback callback)
	{
		List<Gmpc.MetaData.Item> list = null;
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query album cover");
		// Create search query
		GLib.Regex regex_query = null;
		if(song.album != null)
		{
			try
			{
				regex_query = new GLib.Regex(
					"^(folder|cover|.*%s.*)\\.%s$".
					printf(GLib.Regex.escape_string(song.album),file_type_string),
					GLib.RegexCompileFlags.CASELESS|GLib.RegexCompileFlags.DOTALL,
					0);
			}
			catch  (Error erro)
			{
				log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Failed to create regex: %s",erro.message);
				callback(null);
				return;
			}
		}
		else
		{
			try
			{
				regex_query = new GLib.Regex(
					"^(folder|cover)\\.%s$".printf(file_type_string),
					GLib.RegexCompileFlags.CASELESS|GLib.RegexCompileFlags.DOTALL,
					0);
			}
			catch  (Error err1)
			{
				log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
					"Failed to create regex: %s",err1.message);
				callback(null);
				return;
			}
		}
		yield walk_back_directory(regex_query, directory, song.file,
			out list, Gmpc.MetaData.Type.ALBUM_ART);
		list.first();
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query done, %u results", list.length());
		callback((owned)list);
		return ;
	}
	/**
	 * Get biography
	 */
	private async void get_biography(string directory, MPD.Song song, MetaDataCallback callback)
	{
		List<Gmpc.MetaData.Item> list = null;
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query biography");
		// Create search query
		GLib.Regex regex_query = null;
		try
		{
			regex_query = new GLib.Regex(
				"^BIOGRAPHY(\\.txt)?$",
				GLib.RegexCompileFlags.CASELESS|GLib.RegexCompileFlags.DOTALL,
				0);
		}
		catch  (Error erro)
		{
			log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Failed to create regex: %s",erro.message);
			callback(null);
			return;
		}
		yield walk_back_directory(regex_query, directory, song.file,
			out list, Gmpc.MetaData.Type.ARTIST_TXT);
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query done, %u results", list.length());
		list.first();
		callback((owned)list);
		return ;
	}
	/**
	 * Get album information
	 */
	private async void get_album_info(string directory, MPD.Song song, MetaDataCallback callback)
	{
		List<Gmpc.MetaData.Item> list = null;
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query biography");
		// Create search query
		GLib.Regex regex_query = null;
		if(song.album == null)
		{
			log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Album is not set.");
			callback(null);
			return;
		}
		try
		{
			regex_query = new GLib.Regex(
				"^.*%s.*\\.(txt|info)$".printf(song.album),
				GLib.RegexCompileFlags.CASELESS|GLib.RegexCompileFlags.DOTALL,
				0);
		}
		catch  (Error erro)
		{
			log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Failed to create regex: %s",erro.message);
			callback(null);
			return;
		}
		yield walk_back_directory(regex_query, directory, song.file,
			out list, Gmpc.MetaData.Type.ARTIST_TXT);
		log(log_domain_cp, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query done, %u results", list.length());
		list.first();
		callback((owned)list);
		return ;
	}
}
