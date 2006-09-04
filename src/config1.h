#ifndef __CONFIG_1_H__
#define __CONFIG_1_H__
#include <gtk/gtk.h>
#define CFG_INT_NOT_DEFINED -65536

typedef struct _config_obj config_obj;

typedef struct conf_mult_obj
{
	char *key;
	char *value;
	struct conf_mult_obj *next;
	struct conf_mult_obj *prev;
} conf_mult_obj;

/**
 * open the config file
 * *done*
 */
config_obj *cfg_open(gchar *url);


/**
 * Close the object
 * *done*
 */
void cfg_close(config_obj *cfgo);
/* string */

/**
 * Get a single config value as a string.
 * It returns NULL when the value is not availible
 * *done*
 */
char * cfg_get_single_value_as_string(config_obj *cfg, char *class, char *key);
/**
 * set single value 
 * *done*
 */
void cfg_set_single_value_as_string(config_obj *cfg, char *class, char *key, char *value);
/**
 * get with default 
 * *done*
 */
char * cfg_get_single_value_as_string_with_default(config_obj *cfg, char *class, char *key , char *def);

/* done */
int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key);
/* done */
void cfg_set_single_value_as_int(config_obj *cfg, char *class, char *key, int value);
/* done */
int cfg_get_single_value_as_int_with_default(config_obj *cfg, char *class, char *key, int def);

/* float */
/* done */
float cfg_get_single_value_as_float(config_obj *cfg, char *class, char *key);
/* done */
void cfg_set_single_value_as_float(config_obj *cfg, char *class, char *key, float value);
/* done */
float cfg_get_single_value_as_float_with_default(config_obj *cfg, char *class, char *key, float def);

/* del */
/* done */
void cfg_del_single_value(config_obj *cfg, char *class, char *key);


/* done */
conf_mult_obj *cfg_get_class_list(config_obj *data);
/* done */ 
conf_mult_obj *cfg_get_key_list(config_obj *data,char *class);

/* multiple */
/* done */
void cfg_free_multiple(conf_mult_obj *data);
/*done */
conf_mult_obj * cfg_get_multiple_as_string(config_obj *cfg, char *class, char *key);
/*  done */
void cfg_set_multiple_value_as_string(config_obj *cfg, char *class, char *key, char *id, char *value);
/* done */ 
void cfg_del_multiple_value(config_obj *cfg, char *class, char *key,char *id);
void cfg_free_string(char *string);


void cfg_remove_class(config_obj *cfg, char *class);
#endif
