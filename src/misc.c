#include <gtk/gtk.h>
#include <string.h>
#include "misc.h"

/* this should be something configurable by the user */
/* or .f.e window size dependent */ 
gint max_string_length = 32;

char *remove_extention_and_basepath(const char *filename)
{
	int i = 0;
	char *buf = NULL;
	if(filename == NULL)
	{
		return NULL;
	}
	buf  = g_path_get_basename(filename);
	if(buf != NULL)
	{
		for(i=strlen(buf);buf[i] != '.';i--);
		/* cut of the extention */
		if(i > 0)
		{
			gchar *buf2 = g_strndup(buf, i);
			g_free(buf);
			return buf2;
		}
	}
	return buf;
}

char * shorter_string(const char *long_string)
{
	char *ret_val = NULL;
	int strl = 0;
	if(long_string == NULL)
	{
		return NULL;
	}
	strl = g_utf8_strlen(long_string, -1);
	/* this should be configurable? */
	if(strl > max_string_length)
	{
		ret_val = g_strndup(long_string, max_string_length);
		ret_val[max_string_length-3] = '.';
		ret_val[max_string_length-2] = '.';
		ret_val[max_string_length-1] = '.';

	/*	ret_val[max_string_length] = '\0';*/
	}
	else ret_val = g_strdup(long_string);
	return ret_val;

}

