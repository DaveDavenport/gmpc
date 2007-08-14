#ifndef __PLAYLIST_MESSAGES_H__
#define __PLAYLIST_MESSAGES_H__

gboolean playlist3_close_error(void);

typedef enum {
	ERROR_INFO,
	ERROR_WARNING,
	ERROR_CRITICAL
} ErrorLevel;

void playlist3_show_error_message(const gchar *message, ErrorLevel el);

#endif
