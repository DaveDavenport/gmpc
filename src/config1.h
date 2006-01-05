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

config_obj *cfg_open(gchar *url);
void cfg_save(config_obj *cfgo);
void config_close(config_obj *cfgo);
/* string */

char * cfg_get_single_value_as_string(config_obj *cfg, char *class, char *key);
void cfg_set_single_value_as_string(config_obj *cfg, char *class, char *key, char *value);
char * cfg_get_single_value_as_string_with_default(config_obj *cfg, char *class, char *key , char *def);

/* int */
int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key);
void cfg_set_single_value_as_int(config_obj *cfg, char *class, char *key, int value);
int cfg_get_single_value_as_int_with_default(config_obj *cfg, char *class, char *key, int def);
/* float */
float cfg_get_single_value_as_float(config_obj *cfg, char *class, char *key);
void cfg_set_single_value_as_float(config_obj *cfg, char *class, char *key, float value);
float cfg_get_single_value_as_float_with_default(config_obj *cfg, char *class, char *key, float def);

/* del */
void cfg_del_single_value(config_obj *cfg, char *class, char *key);

/* multiple */
void cfg_free_multiple(conf_mult_obj *data);
conf_mult_obj * cfg_get_multiple_as_string(config_obj *cfg, char *class, char *key);
void cfg_set_multiple_value_as_string(config_obj *cfg, char *class, char *key, char *id, char *value);
void cfg_del_multiple_value(config_obj *cfg, char *class, char *key,char *id);
void cfg_free_string(char *string);
#endif
