#include "main.h"
#include "gmpc-extras.h"
#include "mpd-easy-commands.h"
#include "mpdinteraction.h"
#include "advanced-search.h"

extern int current_volume;
/**
 * Easy command interface
 */
static void output_set(gpointer data, const char *param)
{
	gchar *str_param = g_strstrip(g_strdup(param));
	gchar **split = g_strsplit(str_param, " ", 2);
	if (split)
	{
		int output = -1;
		if (split[0])
		{
			output = (int)strtol(split[0], NULL, 10);
		}
		if (output >= 0)
		{
			/* Default to 'select' if no specific action given */
			if (!split[1] || g_utf8_collate(split[1], _("select")) == 0)
			{
				MpdData *devices = mpd_server_get_output_devices(connection);
				MpdData *d = devices;
				gboolean found = FALSE;

				while (d != NULL && d->output_dev->id != -10)
				{
					if (d->output_dev->id == output)
					{
						found = TRUE;
						break;
					}
					d = mpd_data_get_next(d);
				}

				if (found)
				{
					g_debug("select output %i\n", output);
					d = devices;
					while (d != NULL && d->output_dev->id != -10)
					{
						mpd_server_set_output_device(connection, d->output_dev->id,
													 d->output_dev->id == output ? 1 : 0);
						d = mpd_data_get_next(d);
					}
				} else
				{
					/* Let user know their request failed, otherwise they will likely be mystified */
					gchar *str = g_strdup_printf("Can't select output %i because it does not exist", output);
					g_debug("%s\n", str);
					playlist3_message_show(pl3_messages, str, ERROR_WARNING);
					g_free(str);
				}
			} else if (g_utf8_collate(split[1], _("enable")) == 0)
			{
				g_debug("enable output %i\n", output);
				mpd_server_set_output_device(connection, output, 1);
			} else if (g_utf8_collate(split[1], _("disable")) == 0)
			{
				mpd_server_set_output_device(connection, output, 0);
				g_debug("disable output %i\n", output);
			}
		}
		g_free(split);
	}
	g_free(str_param);
}
static void crossfade_set(gpointer data, const char *param)
{
	gchar *param_c = g_utf8_casefold(param, -1);
	gchar *key_c = g_utf8_casefold(_("Off"), -1);
	if (g_utf8_collate(param_c, key_c) == 0)
	{
		mpd_status_set_crossfade(connection, 0);
	} else
	{
		int i = (int)strtol(param, NULL, 10);
		if (i >= 0 && i < 60)
		{
			mpd_status_set_crossfade(connection, i);
		}
	}
	g_free(key_c);
	g_free(param_c);
}
static void volume_set(gpointer data, const char *param)
{
	int cur_volume = mpd_status_get_volume(connection);
	/* if volume is disabled (current_volume < 0) ignore this command */
	if (strlen(param) > 0 && current_volume >= 0)
	{
		int volume = 0;
		if (param[0] == '-' || param[0] == '+')
		{
			volume = cur_volume;
		}
		volume += atoi(param);
		mpd_status_set_volume(connection, volume);
	}
}
static void set_random(gpointer data, const char *param)
{
	if (strncmp(param, "on", 2) == 0)
	{
		mpd_player_set_random(connection, TRUE);
	} else if (strncmp(param, "off", 3) == 0)
	{
		mpd_player_set_random(connection, FALSE);
	} else
	{
		random_toggle();
	}
}
static void update_database_command(gpointer user_data, const char *param)
{
	int val = mpd_server_check_command_allowed(connection, "update");
	if (val == MPD_SERVER_COMMAND_NOT_SUPPORTED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Update database"),
                _("The used MPD server is too old and does not support this."));
		playlist3_message_show(pl3_messages,
                mesg,
                ERROR_CRITICAL);
        g_free(mesg);
    } else if (val == MPD_SERVER_COMMAND_NOT_ALLOWED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Update database"),
                _("You have insufficient permission to use this option."));
		playlist3_message_show(pl3_messages,mesg, ERROR_WARNING);
        g_free(mesg);
	} else if (val == MPD_SERVER_COMMAND_ALLOWED)
	{
		mpd_database_update_dir(connection, "/");
	}
}
static void repeat_current_song_command(gpointer user_data, const char *param)
{
	int val = mpd_server_check_command_allowed(connection, "single");
	if (val == MPD_SERVER_COMMAND_NOT_SUPPORTED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Repeat current song"),
                _("The used MPD server is too old and does not support this."));
		playlist3_message_show(pl3_messages,
                mesg,
                ERROR_CRITICAL);
        g_free(mesg);
    } else if (val == MPD_SERVER_COMMAND_NOT_ALLOWED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Repeat current song"),
                _("You have insufficient permission to use this option."));
        playlist3_message_show(pl3_messages,mesg, ERROR_WARNING);
        g_free(mesg);
    } else if (val == MPD_SERVER_COMMAND_ALLOWED)
	{
		playlist3_message_show(pl3_messages, _("The current song will be forever repeated."), ERROR_INFO);
		mpd_player_set_repeat(connection, TRUE);
		mpd_player_set_single(connection, TRUE);
	}
}
static void stop_after_current_song_command(gpointer user_data, const char *param)
{
	int val = mpd_server_check_command_allowed(connection, "single");
	if (val == MPD_SERVER_COMMAND_NOT_SUPPORTED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Stop after current song"),
                _("The used MPD server is too old and does not support this."));
		playlist3_message_show(pl3_messages,mesg,ERROR_CRITICAL);
        g_free(mesg);
    } else if (val == MPD_SERVER_COMMAND_NOT_ALLOWED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Stop after current song"),
                _("You have insufficient permission to use this option."));
		playlist3_message_show(pl3_messages,mesg,ERROR_WARNING);
        g_free(mesg);
	} else if (val == MPD_SERVER_COMMAND_ALLOWED)
	{
		playlist3_message_show(pl3_messages, _("Playback will be stopped after the current playing song."), ERROR_INFO);
		mpd_player_set_repeat(connection, FALSE);
		mpd_player_set_single(connection, TRUE);
	}
}
static void seek_command(gpointer user_data, const char *param)
{
	int i = 0, j = 0;
	gchar **fields;
	if (!param)
		return;
	if (!mpd_check_connected(connection))
		return;
	printf("seek: '%s'\n", param);
	fields = g_strsplit(param, ":", -1);
	/* Calculate time */
	for (j = 0; fields && fields[j]; j++)
	{
		i = atoi(fields[j]) + i * 60;
	}
	if (param[0] == '+' || param[0] == '-')
	{
		/* seek relative */
		mpd_player_seek(connection, mpd_status_get_elapsed_song_time(connection) + i);
	} else
	{
		/* seek absolute */
		mpd_player_seek(connection, i);
	}
	g_strfreev(fields);
}
static void play_command(gpointer user_data, const char *param)
{
	MpdData *data = advanced_search(param, TRUE);
	if (data)
	{
		play_path(data->song->file);
		mpd_data_free(data);
	}
}
static void add_command(gpointer user_data, const char *param)
{
	gulong songs = 0;
	MpdData *data = advanced_search(param, FALSE);
	for (; data; data = mpd_data_get_next(data))
	{

		if ((songs & 16383) == 16383)
		{
			mpd_playlist_queue_commit(connection);
		}
		if (data->type == MPD_DATA_TYPE_SONG)
		{
			mpd_playlist_queue_add(connection, data->song->file);
			songs++;
		}
	}
	mpd_playlist_queue_commit(connection);
}
static void replace_command(gpointer user_data, const char *param)
{
	gulong songs = 0;
	MpdData *data = advanced_search(param, FALSE);
	mpd_playlist_clear(connection);
	for (; data; data = mpd_data_get_next(data))
	{

		if ((songs & 16383) == 16383)
		{
			mpd_playlist_queue_commit(connection);
		}
		if (data->type == MPD_DATA_TYPE_SONG)
		{
			mpd_playlist_queue_add(connection, data->song->file);
			songs++;
		}
	}
	mpd_playlist_queue_commit(connection);
}
static void set_consume(gpointer data, const char *param)
{
	int val = mpd_server_check_command_allowed(connection, "consume");
	if (val == MPD_SERVER_COMMAND_NOT_SUPPORTED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Consume"),
                _("The used MPD server is too old and does not support this."));
		playlist3_message_show(pl3_messages,mesg,ERROR_CRITICAL);
        g_free(mesg);
    } else if (val == MPD_SERVER_COMMAND_NOT_ALLOWED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Consume"),
                _("You have insufficient permission to use this option."));
		playlist3_message_show(pl3_messages,mesg,ERROR_WARNING);
        g_free(mesg);
	} else if (val == MPD_SERVER_COMMAND_ALLOWED)
	{
		if (g_utf8_collate(param, "on") == 0)
		{
			mpd_player_set_consume(connection, TRUE);
		} else if (g_utf8_collate(param, "off") == 0)
		{
			mpd_player_set_consume(connection, FALSE);
		} else
		{
			mpd_player_set_consume(connection, !mpd_player_get_consume(connection));
		}
	}
}
static void set_single(gpointer data, const char *param)
{
	int val = mpd_server_check_command_allowed(connection, "single");
	if (val == MPD_SERVER_COMMAND_NOT_SUPPORTED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Single"),
                _("The used MPD server is too old and does not support this."));
        playlist3_message_show(pl3_messages,mesg,ERROR_CRITICAL);
        g_free(mesg);
    } else if (val == MPD_SERVER_COMMAND_NOT_ALLOWED)
	{
        char * mesg = g_strdup_printf("%s: %s",
                _("Single"),
                _("You have insufficient permission to use this option."));
        playlist3_message_show(pl3_messages, mesg, ERROR_WARNING);
        g_free(mesg);
	} else if (val == MPD_SERVER_COMMAND_ALLOWED)
	{
		if (g_utf8_collate(param, "on") == 0)
		{
			mpd_player_set_single(connection, TRUE);
		} else if (g_utf8_collate(param, "off") == 0)
		{
			mpd_player_set_single(connection, FALSE);
		} else
		{
			mpd_player_set_single(connection, !mpd_player_get_single(connection));
		}
	}
}
static void set_repeat(gpointer data, const char *param)
{
	if (strncmp(param, "on", 2) == 0)
	{
		mpd_player_set_repeat(connection, TRUE);
	} else if (strncmp(param, "off", 3) == 0)
	{
		mpd_player_set_repeat(connection, FALSE);
	} else
	{
		repeat_toggle();
	}
}
static void disconnect(gpointer data, const char *param)
{
    if(mpd_check_connected(connection)) {
        disconnect_from_mpd();
    }
}
static void connect(gpointer data, const char *param)
{
    if(mpd_check_connected(connection)) {
        disconnect_from_mpd();
    }
    if(param == NULL || param[0] == '\0') {
        connect_to_mpd();
    }else{
        gmpc_profiles_set_profile_from_name(gmpc_profiles,
                param);
        connect_to_mpd();
    }
}

static void reset_playmode(gpointer data, const char *param)
{
	mpd_player_set_repeat(connection, FALSE);
	mpd_player_set_random(connection, FALSE);
	mpd_player_set_single(connection, FALSE);
	mpd_player_set_consume(connection, FALSE);
}

void mpd_easy_commands_init(void)
{
	/* Player control */
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("play"), "", _("start playback"),
								(GmpcEasyCommandCallback *) play_song, connection,
								"gtk-media-play");
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("pause"), "", _("pause playback"),
								(GmpcEasyCommandCallback *) pause_song, connection,
								"gtk-media-pause");
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("next"), "", _("next song"), (GmpcEasyCommandCallback *) next_song,
								NULL,
								GTK_STOCK_MEDIA_NEXT);
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("prev"), "", _("previous song"),
								(GmpcEasyCommandCallback *) prev_song, NULL,
								GTK_STOCK_MEDIA_PREVIOUS);
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("stop"), "", _("stop playback"),
								(GmpcEasyCommandCallback *) stop_song, NULL,
								"gtk-media-stop");

	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command,
								_("reset playmode"),
								"",
								_("Reset the play mode."), (GmpcEasyCommandCallback *) reset_playmode, NULL, GTK_STOCK_CLEAR);

	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command, _("random"), "(on|off|)", _("Random (on|off)"),
								(GmpcEasyCommandCallback *) set_random, NULL, "stock_shuffle");
	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command, _("repeat"), "(on|off|)", _("Repeat (on|off)"),
								(GmpcEasyCommandCallback *) set_repeat, NULL, "stock_repeat");

	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command, _("single"), "(on|off|)", _("Single (on|off)"),
								(GmpcEasyCommandCallback *) set_single, NULL, "media-repeat-single");
	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command, _("consume"), "(on|off|)", _("Consume (on|off)"),
								(GmpcEasyCommandCallback *) set_consume, NULL,"media-consume");

	/* volume commands */
	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command, _("volume"), "[+-]?[0-9]+", _("Volume (+-)<level>"),
								(GmpcEasyCommandCallback *) volume_set, NULL, "audio-volume-high");
	gmpc_easy_command_add_entry_icon_name(gmpc_easy_command, _("mute"), "", _("Mute"),
								(GmpcEasyCommandCallback *) volume_toggle_mute, NULL, "audio-volume-muted");

	gmpc_easy_command_add_entry(gmpc_easy_command, _("crossfade"),
								C_("Regex for matching crossfade, translate off", "([0-9]+|Off)"),
								_("Set Crossfade <seconds>"), (GmpcEasyCommandCallback *) crossfade_set, NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command, _("output"),
								C_("Regex for matching output", "[0-9]+[ ]*(Enable|Disable|Select|)"),
								_("output X enable or disable or select"), (GmpcEasyCommandCallback *) output_set,
								NULL);
	/* basic playlist commands */
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("play"), ".*", _("Play <query>"),
								(GmpcEasyCommandCallback *) play_command, NULL, "gtk-media-play");
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("add"), ".*", _("Add <query>"),
								(GmpcEasyCommandCallback *) add_command, NULL, GTK_STOCK_ADD);
	gmpc_easy_command_add_entry_stock_id(gmpc_easy_command, _("replace"), ".*", _("Replace <query>"),
								(GmpcEasyCommandCallback *) replace_command, NULL, GTK_STOCK_REDO);

	/* Basic seek commands */
	gmpc_easy_command_add_entry(gmpc_easy_command, _("seek"), "[+-]?[0-9:]+", _("Seek within the current song"),
								(GmpcEasyCommandCallback *) seek_command, NULL);
	/* Advanced commands */
	gmpc_easy_command_add_entry(gmpc_easy_command,
								_("stop after current song"),
								"",
								_("Stop playback after the current song"),
								(GmpcEasyCommandCallback *) stop_after_current_song_command, NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command,
								_("repeat current song"),
								"",
								_("Repeat the current song"),
								(GmpcEasyCommandCallback *) repeat_current_song_command, NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command,
								_("update database"),
								"",
								_("Update the database"), (GmpcEasyCommandCallback *) update_database_command, NULL);


	gmpc_easy_command_add_entry(gmpc_easy_command,
								_("disconnect"),
								"",
								_("disconnect from MPD"), (GmpcEasyCommandCallback *) disconnect, NULL);

	gmpc_easy_command_add_entry(gmpc_easy_command,
								_("connect"),
                                "",
								_("connect to MPD"), (GmpcEasyCommandCallback *) connect, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command,
								_("connect"),
								".*",
								_("connect to MPD using profile"), (GmpcEasyCommandCallback *) connect, NULL);
}
