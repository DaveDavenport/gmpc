#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>

#include "libmpdclient.h"
#include "main.h"
#define CONFIG ".gmpc.cfg"

void load_config ()
{
	gchar *filename = g_strdup_printf ("%s/%s", g_getenv ("HOME"), CONFIG);
	FILE *fp;
	gchar buffer[1024];
	fp = fopen (filename, "r");
	if (fp == NULL)
	{
		g_free (filename);
		return;
	}
	memset (buffer, '\0', 1024);
	while (fgets (buffer, 1024, fp))
	{

		if (!strncmp (buffer, "host:", 5))
		{
			gchar *buf = g_strstrip (&buffer[5]);
			if (buf != NULL)
			{
				memset (preferences.host, '\0', 256);
				strncpy (preferences.host, buf, MIN (strlen (buf), 256));
			}
		}

		else if (!strncmp (buffer, "port:", 5))
		{
			preferences.port = atoi (&buffer[5]);
		}
		else if (!strncmp (buffer, "timeout:", 8))
		{
			preferences.timeout = (float) g_strtod (&buffer[8], NULL);
		}
		else if (!strncmp (buffer, "auto-connect:", 13))
		{
			preferences.autoconnect = atoi (&buffer[13]);
		}
		else if (!strncmp (buffer, "do tray:", 8))
		{
			gchar *buf = g_strstrip (&buffer[8]);
			if (buf != NULL)
			{
				info.do_tray = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "do tray popup:", 14))
		{
			gchar *buf = g_strstrip (&buffer[14]);
			if (buf != NULL)
			{
				info.do_tray_popup = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "time format:", 12))
		{
			gchar *buf = g_strstrip (&buffer[12]);
			if (buf != NULL)
			{
				info.time_format = atoi (buf);
			}
		}


		else if (!strncmp (buffer, "do popup:", 9))
		{
			gchar *buf = g_strstrip (&buffer[9]);
			if (buf != NULL)
			{
				info.popup.do_popup = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup pos:", 10))
		{
			gchar *buf = g_strstrip (&buffer[10]);
			if (buf != NULL)
			{
				info.popup.position = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup stay:", 11))
		{
			gchar *buf = g_strstrip (&buffer[11]);
			if (buf != NULL)
			{
				info.popup.popup_stay = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup state:", 12))
		{
			gchar *buf = g_strstrip (&buffer[12]);
			if (buf != NULL)
			{
				info.popup.show_state = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup timeout:", 14))
		{
			gchar *buf = g_strstrip (&buffer[14]);
			if (buf != NULL)
			{
				info.popup.timeout = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "use auth:", 9))
		{
			gchar *buf = g_strstrip (&buffer[9]);
			if (buf != NULL)
			{
				preferences.user_auth = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "auth pass:", 10))
		{
			gchar *buf = g_strstrip (&buffer[10]);
			if (buf != NULL)
			{
				memset (preferences.password, '\0', 256);
				strncpy (preferences.password, buf, MIN (strlen (buf), 256));
			}
		}

		else if (!strncmp (buffer, "markup main display:", 20))
		{
			gchar *buf = g_strstrip (&buffer[20]);
			if (buf != NULL)
			{
				if (preferences.markup_main_display != NULL)
				{
					g_free (preferences.markup_main_display);
				}
				preferences.markup_main_display = g_strdup (buf);
			}
		}
		else if (!strncmp (buffer, "markup playlist:", 16))
		{
			gchar *buf = g_strstrip (&buffer[16]);
			if (buf != NULL)
			{
				if (preferences.markup_playlist != NULL)
				{
					g_free (preferences.markup_playlist);
				}
				preferences.markup_playlist = g_strdup (buf);
			}
		}
		else if (!strncmp (buffer, "markup song browser:", 20))
		{
			gchar *buf = g_strstrip (&buffer[20]);
			if (buf != NULL)
			{
				if (preferences.markup_song_browser != NULL)
				{
					g_free (preferences.markup_song_browser);
				}
				preferences.markup_song_browser = g_strdup (buf);
			}
		}
		else if (!strncmp (buffer, "pl2 tooltip timeout:", 20))
		{
			gchar *buf = g_strstrip (&buffer[20]);
			if (buf != NULL)
			{
				info.pl2_tooltip = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "pl2 do tooltip:", 15))
		{
			gchar *buf = g_strstrip (&buffer[15]);
			if (buf != NULL)
			{
				info.pl2_do_tooltip = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "rounded corners:", 16))
		{
			gchar *buf = g_strstrip (&buffer[16]);
			if (buf != NULL)
			{
				info.rounded_corners = atoi (buf);
			}
		}





		memset (buffer, '\0', 1024);

	}
	fclose (fp);
	g_free (filename);
}

void save_config ()
{
	gchar *filename = g_strdup_printf ("%s/%s", g_getenv ("HOME"), CONFIG);
	FILE *fp;

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		g_free (filename);
		return;
	}
	fprintf (fp, "host: %s\n", preferences.host);
	fprintf (fp, "port: %i\n", preferences.port);
	fprintf (fp, "timeout: %.2f\n", preferences.timeout);
	fprintf (fp, "auto-connect: %i\n", preferences.autoconnect);
	fprintf (fp, "do tray: %i\n", info.do_tray);
	fprintf (fp, "do tray popup: %i\n", info.do_tray_popup);
	fprintf (fp, "time format: %i\n", info.time_format);
	fprintf (fp, "do popup: %i\n", info.popup.do_popup);
	fprintf (fp, "popup pos: %i\n", info.popup.position);
	fprintf (fp, "popup stay: %i\n", info.popup.popup_stay);
	fprintf (fp, "popup state: %i\n", info.popup.show_state);
	fprintf (fp, "popup timeout: %i\n", info.popup.timeout);
	fprintf (fp, "use auth: %i\n", preferences.user_auth);
	fprintf (fp, "auth pass: %s\n", preferences.password);
	fprintf (fp, "markup main display: %s\n", preferences.markup_main_display);
	fprintf (fp, "markup playlist: %s\n", preferences.markup_playlist);
	fprintf (fp, "markup song browser: %s\n", preferences.markup_song_browser);
	fprintf (fp, "pl2 do tooltip: %i\n", info.pl2_do_tooltip);
	fprintf (fp, "pl2 tooltip timeout: %i\n", info.pl2_tooltip);	
	fprintf (fp, "rounded corners: %i\n", info.rounded_corners);
	fclose (fp);
	g_free (filename);
}
