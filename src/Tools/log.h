#ifndef __LOG_H__
#define __LOG_H__

/**
 * Initialize the log system.
 */
void log_init(void);

/**
 * Set the log level. (see libmpd.conf)
 */
void log_set_debug_level(int debug_level);

/**
 * Used by options to enable extra logging for sub-systems.
 */ 
gboolean log_add_filter(
        const gchar * option_name,
        const gchar * value,
        gpointer data,
        GError ** error);


/**
 * Redirect the log output to a file
 * 
 * \todo (This function is incomplete and should no longer be used.)
 */
void log_redirect_to_file(const char *url);

#endif
