#ifndef __CONFIG_1_H__
#define __CONFIG_1_H__
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef struct config_obj
{
	gchar *url;
	xmlDocPtr xmldoc;
	xmlNodePtr root;
} config_obj;


config_obj *cfg_open(gchar *url);
void cfg_save(config_obj *cfgo);



char * cfg_get_single_value_as_string(config_obj *cfg, char *class, char *key);
int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key);
void cfg_del_single_value(config_obj *cfg, char *class, char *key);
void cfg_set_single_value_as_string(config_obj *cfg, char *class, char *key, char *value);

#endif
