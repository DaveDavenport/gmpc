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


#endif
