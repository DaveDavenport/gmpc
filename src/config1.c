/*
 *Copyright (C) 2004-2007 Qball Cow <qball@sarine.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "config1.h"
#include <libmpd/debug_printf.h>
typedef enum _ConfigNodeType  {
	TYPE_CATEGORY,
	TYPE_ITEM,
	TYPE_ITEM_MULTIPLE
} ConfigNodeType;
/**
 * A config node
 * 24byte large
 */
typedef struct _config_node {
	struct _config_node *next;
	struct _config_node *prev;
	struct _config_node *parent;
	gchar *name;
	ConfigNodeType type;
	/* Save some extra memory by using a union
	 *  It is actually effective because we build a resonable large tree using this
	 */
	union{
		struct _config_node *children; 	/* TYPE_CATEGORY */
		gchar *value;			/* TYPE_ITEM* */
	};
} config_node;


typedef struct _config_obj
{
	gchar *url;
	GMutex *lock;
	config_node *root;
	int total_size;
} _config_obj;

static void __int_cfg_set_single_value_as_string(config_obj *, char *, char *, char *);
static void cfg_save(config_obj *);
static void __int_cfg_remove_node(config_obj *, config_node *);

static void __int_cfg_do_special_cleanup(config_obj *cfg, config_node *node);
static config_node *cfg_add_class(config_obj *, char *);
static config_node *cfg_new_node(void);
static void cfg_add_child(config_node *, config_node *);

static void cfg_open_parse_file(config_obj *cfgo, FILE *fp)
{
	char buffer[1024];
	int len = 0;
	int c;
	config_node *cur = NULL;
	config_node *multiple = NULL;
	while((c = fgetc(fp)) != EOF)
	{
		if(c == '[')
		{
			len =0;
			c = fgetc(fp);
			while(c != ']' && c != EOF && len < 1024){
				buffer[len] = c;
				len++;
				c = fgetc(fp);
			}
			buffer[len] = '\0';
			if(len > 0 && len < 256)
			{
				cur = cfg_add_class(cfgo, buffer);
			}
			/* seek end of line */
			while(c != EOF && c != '\n') c = fgetc(fp);
		}
		if(cur && c == '{')
		{
			len =0;                                    		
			c = fgetc(fp);
			while(c != '}' && c != EOF && len < 1024){
				buffer[len] = c;
				len++;
				c = fgetc(fp);
			}
			buffer[len] = '\0';
			if(len > 0 && len < 256)
			{
				config_node *child = cfg_new_node();
				child->type = TYPE_ITEM_MULTIPLE;
				child->name = g_strndup(buffer, len);
				child->children = NULL;
				cfg_add_child(cur, child);
				multiple = child;
				cfgo->total_size+= len+sizeof(config_node);

			}
			if(len==0)
			{
				multiple = NULL;
			}
			/* seek end of line */
			while(c != EOF && c != '\n') c = fgetc(fp);
		}

		/* next, ignore commants  and there must be a category*/
		else if(cur && (c  == '#' || c == '/' || c == '\n' || c == ';')){
			while(c != EOF && c != '\n') c = fgetc(fp);
		}
		else if(cur){
			config_node *new = NULL;
			len = 0;
			while(c != '=' && c != EOF){
				buffer[len] = c;
				len++;
				c = fgetc(fp);
			}
			if(len < 256 && len > 0)
			{
				int quote=0;

				/* write key name */
				new = cfg_new_node();
				new->type = TYPE_ITEM;
				new->name = g_strndup(buffer, len);	
				cfgo->total_size+= len+sizeof(config_node);
				/* Get value */
				len = 0;
				/* skip spaces */
				while((c = fgetc(fp)) == ' ');
				/* we got a quoted string */
				if(c == '"')
				{
					quote= 1;
					c = fgetc(fp);
				}
				do{
					/* add escaped char */
					if(c == '\\'){
						c = fgetc(fp);
						if(c == 'n')
						{
							buffer[len] = '\n';
							len++;
						}
						else
						{
							buffer[len] = c;
							len++;
						}
					}
					/* We have a quoted string, and the closing quote comes */
					else if(c == '"' && quote) quote = -1;
					else{
						buffer[len] = c;
						len++;
					}
					c = fgetc(fp);
				}while((c != '\n' || quote) && c != EOF && quote >= 0 && len < 1024);
				new->value = g_strndup(buffer, len);
				cfgo->total_size+= len;
				if(multiple){
					cfg_add_child(multiple,new);
				}else{
					cfg_add_child(cur, new);
				}
			}
			/* seek end of line */
			while(c != EOF && c != '\n') c = fgetc(fp);			
		}
		else while(c != EOF && c != '\n') c = fgetc(fp);			
	}
}

config_obj *cfg_open(gchar *url)
{
	config_obj *cfgo = NULL;
	/* check if there is an url passed */
	if(url == NULL)
	{
		return NULL;
	}

	cfgo = g_malloc(sizeof(config_obj));
	/* check if malloc went ok */
	if(cfgo == NULL)
	{
		return NULL;
	}
	cfgo->lock = g_mutex_new();
	g_mutex_lock(cfgo->lock);
	cfgo->url = g_strdup(url);
	cfgo->root = NULL;
	cfgo->total_size = sizeof(config_obj)+sizeof(&cfgo->lock)+strlen(cfgo->url);

	if(g_file_test(cfgo->url, G_FILE_TEST_EXISTS))
	{
		FILE *fp = fopen(cfgo->url, "r");
		if(fp)
		{
			cfg_open_parse_file(cfgo, fp);
			fclose(fp);
		}
	}
	debug_printf(DEBUG_INFO,"Config %s: allocated: %i\n", cfgo->url, cfgo->total_size);
	g_mutex_unlock(cfgo->lock);
	return cfgo;
}


void cfg_close(config_obj *cfgo)
{
	if(cfgo == NULL)
	{
		return;
	}
	debug_printf(DEBUG_INFO,"Closing config '%s' with %i bytes allocated\n", cfgo->url, cfgo->total_size);
	g_mutex_lock(cfgo->lock);
	if(cfgo->url != NULL)
	{
		cfgo->total_size-=strlen(cfgo->url);
		cfg_free_string(cfgo->url);
	}
	while(cfgo->root)__int_cfg_remove_node(cfgo,cfgo->root);
	g_mutex_unlock(cfgo->lock);
	cfgo->total_size-= sizeof(&cfgo->lock)+sizeof(config_obj);
	g_mutex_free(cfgo->lock);
	debug_printf(DEBUG_INFO,"Memory remaining: %i\n", cfgo->total_size);
	g_free(cfgo);
	cfgo = NULL;
}
static config_node *cfg_new_node()
{
	config_node *newnode = g_slice_new(config_node);
	newnode->type = TYPE_CATEGORY;
	newnode->name = NULL; 
	newnode->next = NULL;
	newnode->prev = NULL;                                 
	newnode->parent = NULL;
	newnode->value = NULL;
	return newnode;
}
static config_node *cfg_add_class(config_obj *cfg, char *class)
{
	config_node *newnode = cfg_new_node();
	newnode->type = TYPE_CATEGORY;
	newnode->name = g_strdup(class);
	newnode->value = NULL;
	newnode->children  = NULL;
	cfg->total_size += sizeof(config_node)+strlen(class);
	if(cfg->root == NULL)
	{
		cfg->root = newnode;		
	}
	else
	{
		config_node *temp = cfg->root;
		while(temp->next != NULL) temp = temp->next;
		temp->next = newnode;
		newnode->prev = temp;
	}
	return newnode;
}
void cfg_add_child(config_node *parent, config_node *child)
{
	if(parent == NULL || child == NULL) return;
	if(parent->type == TYPE_ITEM ) return;
	if(parent->children == NULL)
	{
		parent->children = child;
		child->parent = parent;
	}
	else
	{
		config_node *temp = parent->children;

		/* get last node */
		while(temp->next != NULL) temp = temp->next;
		temp->next = child;
		child->prev = temp;	
		child->parent = parent;
	}
}

static void cfg_save_category(config_obj *cfg, config_node *node, FILE *fp)
{
	config_node *temp = NULL;
	if(node == NULL)return;
	/* find the first */
	while(node->prev != NULL) node = node->prev;
	/* save some stuff */
	for(temp = node;temp != NULL; temp = temp->next){
		if(temp->type == TYPE_CATEGORY)
		{
			fprintf(fp, "\n[%s]\n\n",temp->name);
			cfg_save_category(cfg,temp->children,fp);
		}
		if(temp->type == TYPE_ITEM_MULTIPLE)
		{
			fprintf(fp, "\n{%s}\n",temp->name);
			cfg_save_category(cfg,temp->children,fp);
			fprintf(fp, "{}\n\n");
		}                                                		
		else if (temp->type == TYPE_ITEM)
		{
			int i= 0;
			fprintf(fp, "%s=\"", temp->name);
			for(i=0;i<strlen(temp->value);i++)
			{
				if(temp->value[i] == '"'){
					fputs("\\\"",fp);
				}
				else if(temp->value[i] == '\\'){
					fputs("\\\\",fp);
				}
				else if(temp->value[i] == '\n'){
					fputs("\\n",fp);
				}
				else{
					fputc(temp->value[i],fp);
				}
			}
			fputs("\"\n",fp);
		}
	}
}

static void cfg_save(config_obj *cfgo)
{
	if(cfgo == NULL)
	{
		return;
	}
	if(cfgo->root != NULL)
	{
		FILE *fp = fopen(cfgo->url, "w");
		if(!fp) return;
		cfg_save_category(cfgo,cfgo->root, fp);	
		fclose(fp);
#ifndef WIN32
		chmod(cfgo->url, 0600);
#endif

	}
	return;
}

static config_node *cfg_get_class(config_obj *cfg, char *class)
{
	config_node *node = cfg->root;
	if(node == NULL) return NULL;
	/* find the first */
	while(node->prev != NULL) node = node->prev;

	for(; node!= NULL; node = node->next)
	{
		if(node->type == TYPE_CATEGORY && !strcmp(node->name, class))
		{
			return node;
		}
	}

	return NULL;
}

static config_node *cfg_get_single_value(config_obj *cfg, char *class, char *key)
{
	/* take children */
	config_node *cur = NULL;
	cur = cfg_get_class(cfg, class);
	if(cur == NULL || cur->children == NULL)
	{
		return NULL;
	}
	cur = cur->children;
	for(;cur != NULL; cur = cur->next) {
		if(!strcmp(cur->name, key))
		{
			return cur;
		}
	}
	return NULL;                                     	
}

/*void cfg_free_string(char *string)
{
	if(string != NULL)
	{
		q_free(string);
	}
}
*/
static char * __int_cfg_get_single_value_as_string(config_obj *cfg, char *class, char *key)
{
	config_node *cur = cfg_get_single_value(cfg, class,key);
	if(cur != NULL)
	{
		if(cur->type == TYPE_ITEM)
		{
			return g_strdup((char *)cur->value);
		}
	}
	return NULL;
}

char * cfg_get_single_value_as_string(config_obj *cfg, char *class, char *key)
{
	char *retv = NULL;
	g_mutex_lock(cfg->lock);
	retv = __int_cfg_get_single_value_as_string(cfg,class,key);
	g_mutex_unlock(cfg->lock);
	return retv;
}


char * cfg_get_single_value_as_string_with_default(config_obj *cfg, char *class, char *key , char *def)
{
	char *retv = NULL;
   	g_mutex_lock(cfg->lock);
	retv = __int_cfg_get_single_value_as_string(cfg,class,key);
	if(retv == NULL)
	{
		__int_cfg_set_single_value_as_string(cfg,class,key,def);
		retv = __int_cfg_get_single_value_as_string(cfg,class,key);
	}
	g_mutex_unlock(cfg->lock);
	return retv;
}

static int __int_cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key)
{
	config_node *cur =NULL;
	int retv = CFG_INT_NOT_DEFINED;

	cur = cfg_get_single_value(cfg, class,key);
	if(cur != NULL)
	{
		if(cur->type == TYPE_ITEM)
		{
			retv = (int)g_ascii_strtoull(cur->value,NULL,0);	
		}
	}

	return retv; 
}
int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key)
{
	int retv = 0;
	g_mutex_lock(cfg->lock);
	retv = __int_cfg_get_single_value_as_int(cfg, class, key);
	g_mutex_unlock(cfg->lock);
	return retv;
}

void cfg_set_single_value_as_int(config_obj *cfg, char *class, char *key, int value)
{
	gchar *	temp = NULL;
	g_mutex_lock(cfg->lock);
	temp  = g_strdup_printf("%i",value);
	__int_cfg_set_single_value_as_string(cfg,class,key,temp);
	cfg_free_string(temp);
	g_mutex_unlock(cfg->lock);
}
static void __int_cfg_set_single_value_as_int(config_obj *cfg, char *class, char *key, int value)
{
	gchar *	temp = NULL;
	temp  = g_strdup_printf("%i",value);
	__int_cfg_set_single_value_as_string(cfg,class,key,temp);
	cfg_free_string(temp);
}
int cfg_get_single_value_as_int_with_default(config_obj *cfg, char *class, char *key, int def)
{
	int retv = CFG_INT_NOT_DEFINED;
	g_mutex_lock(cfg->lock);		
	retv = __int_cfg_get_single_value_as_int(cfg,class,key);
	if(retv == CFG_INT_NOT_DEFINED)
	{
		__int_cfg_set_single_value_as_int(cfg,class,key,def);
		retv = __int_cfg_get_single_value_as_int(cfg,class,key);		
	}
	g_mutex_unlock(cfg->lock);		
	return retv;
}
/* float */
static float __int_cfg_get_single_value_as_float(config_obj *cfg, char *class, char *key)
{
	config_node *cur =NULL;
	cur = cfg_get_single_value(cfg,class,key);
	float result = 0;
	if(cur == NULL)
	{
		return CFG_INT_NOT_DEFINED;
	}
	/* make it return an error */
	result = g_ascii_strtod(cur->value,NULL);
	return result;
}

float cfg_get_single_value_as_float(config_obj *cfg, char *class, char *key)
{
	float retv = 0;
	g_mutex_lock(cfg->lock);
	retv = __int_cfg_get_single_value_as_float(cfg,class,key);
	g_mutex_unlock(cfg->lock);
	return retv;
}

void cfg_set_single_value_as_float(config_obj *cfg, char *class, char *key, float value)
{
	char *value1 = NULL;
	g_mutex_lock(cfg->lock);
	value1 = g_strdup_printf("%f", value);
	__int_cfg_set_single_value_as_string(cfg,class,key,value1);
	cfg_free_string(value1);
	g_mutex_unlock(cfg->lock);
}
float cfg_get_single_value_as_float_with_default(config_obj *cfg, char *class, char *key, float def)
{
	float retv = 0;
	g_mutex_lock(cfg->lock);
	retv = __int_cfg_get_single_value_as_float(cfg,class,key);
	if(retv == CFG_INT_NOT_DEFINED)
	{
		char * value1 = g_strdup_printf("%f", def);
		__int_cfg_set_single_value_as_string(cfg,class,key,value1);
		cfg_free_string(value1);
		retv = __int_cfg_get_single_value_as_float(cfg,class,key);		
	}
	g_mutex_unlock(cfg->lock);
	/* make it return an error */
	return retv;
}

static void __int_cfg_remove_node(config_obj *cfg, config_node *node)
{
	if(node->type != TYPE_ITEM)
	{
		while(node->children)
		{
			__int_cfg_remove_node(cfg,node->children);
		}
	}
	/*  only child, and I have a parent */
	if(node->next == NULL && node->prev == NULL && node->parent)
	{
		/* remove from list */	
		if(node->parent->type != TYPE_ITEM)
		{
			node->parent->children = NULL;
		}
	}
	/* remove node from linked list */
	if(node->next != NULL)
	{
		if(node->parent && node->parent->children == node)
		{
			node->parent->children = node->next;
		}
		node->next->prev = node->prev;
	}
	if(node->prev != NULL)
	{
		if(node->parent && node->parent->children == node)
		{
			node->parent->children = node->prev;		
		}
		node->prev->next = node->next;
	}
	if(node == cfg->root)
	{
		if(node->next){
			cfg->root = node->next;
		}else if (node->prev){
			cfg->root = node->prev;
		}else{
			cfg->root = NULL;
		}
	}	
	cfg->total_size-= sizeof(config_node);
	if(node->name){
		cfg->total_size-=strlen(node->name);
		 cfg_free_string(node->name);
	}
	if(node->value) {
		cfg->total_size-=strlen(node->value);
		cfg_free_string(node->value);
	}
	g_slice_free(config_node,node);
}
void cfg_del_single_value(config_obj *cfg, char *class, char *key)
{
	config_node *node = NULL;
	g_mutex_lock(cfg->lock);
   	node = cfg_get_single_value(cfg,class,key);
	if(node != NULL)
	{
		__int_cfg_remove_node(cfg,node);
		cfg_save(cfg);
	}
	g_mutex_unlock(cfg->lock);
}
void cfg_remove_class(config_obj *cfg, char *class)
{
	config_node *node = NULL;
	if(cfg == NULL || class == NULL)
		return;

	g_mutex_lock(cfg->lock);
	node = cfg_get_class(cfg, class);
	if(node)
	{
		__int_cfg_remove_node(cfg, node);
	}
	cfg_save(cfg);
	g_mutex_unlock(cfg->lock);
}
static void __int_cfg_set_single_value_as_string(config_obj *cfg, char *class, char *key, char *value)
{
	config_node *newnode = cfg_get_single_value(cfg,class,key);
	if(newnode == NULL)
	{	
		config_node *node = cfg_get_class(cfg, class);
		if(node == NULL)
		{
			node = cfg_add_class(cfg, class);	
			if(node == NULL) return;
		}
		newnode = cfg_new_node();
		newnode->name = g_strdup(key);
		cfg->total_size+=sizeof(config_node)+strlen(key);
		cfg_add_child(node,newnode);

	}	
	else if(strlen(newnode->value) == strlen(value) && !memcmp(newnode->value, value,strlen(newnode->value)))
	{
		/* Check if the content is the same, if it is, do nothing */
		return;
	}
	newnode->type = TYPE_ITEM;
	if(newnode->value){
		cfg->total_size-= strlen(newnode->value);
		cfg_free_string(newnode->value);
	}
	newnode->value = g_strdup(value);	
	cfg->total_size += strlen(value);
	cfg_save(cfg);
}

void cfg_set_single_value_as_string(config_obj *cfg, char *class, char *key, char *value)
{
	g_mutex_lock(cfg->lock);
	__int_cfg_set_single_value_as_string(cfg, class,key,value);
	g_mutex_unlock(cfg->lock);
}

/* multiple values */


static config_node *cfg_get_multiple_value(config_obj *cfg, char *class, char *key, char* id)
{
	config_node *node = cfg_get_single_value(cfg,class,key);
	if(node == NULL){
		return NULL;
	}
	if(node->type != TYPE_ITEM_MULTIPLE){
		return NULL;
	}
	if(node->children == NULL){
		return NULL;
	}
	/* first*/
	node = node->children;
	while(node->prev != NULL) node = node->prev;
	/* get item */
	for(; node != NULL;node = node->next) {
		if(!strcmp(node->name, id)) 
		{
			return node;
		}
	}
	return NULL;
}

void cfg_del_multiple_value(config_obj *cfg, char *class, char *key,char *id)
{
	config_node *cur = NULL;
	g_mutex_lock(cfg->lock);
	cur = cfg_get_multiple_value(cfg, class,key,id);
	if(cur != NULL)
	{
		__int_cfg_remove_node(cfg,cur);
		cfg_save(cfg);
	}
	g_mutex_unlock(cfg->lock);
}

void cfg_set_multiple_value_as_string(config_obj *cfg, char *class, char *key, char *id, char *value)
{
	config_node *cur = NULL;
	g_mutex_lock(cfg->lock);
	cur = cfg_get_multiple_value(cfg, class,key,id);
	if(cur != NULL)
	{
		if(strlen(cur->value) == strlen(value) && !memcmp(cur->value, value, strlen(cur->value)))
		{
			return;
		}
		if(cur->value){
			cfg->total_size -= strlen(cur->value);
			cfg_free_string(cur->value);
		}

		cur->value = g_strdup(value);
		cfg->total_size += strlen(cur->value);
		cfg_save(cfg);
	}
	else {
		config_node *node = cfg_get_single_value(cfg,class,key);
		if(node == NULL) {
			node = cfg_get_class(cfg,class);
			if(node == NULL)
			{
				node = cfg_add_class(cfg,class);
			}
			cur = cfg_new_node();
			cur->name = g_strdup(key);
			cur->type = TYPE_ITEM_MULTIPLE;
			cfg_add_child(node,cur);
			cfg->total_size+=sizeof(config_node)+strlen(key);
			node = cur;
		}
		if(node->type != TYPE_ITEM_MULTIPLE) return;
		cur = cfg_new_node();
		cur->type = TYPE_ITEM;
		cur->name = g_strdup(id);
		cur->value = g_strdup(value);
		cfg->total_size+=sizeof(config_node)+strlen(id)+strlen(value);;
		cfg_add_child(node,cur);
		cfg_save(cfg);
	}
	g_mutex_unlock(cfg->lock);
}

conf_mult_obj * cfg_get_multiple_as_string(config_obj *cfg, char *class, char *key)
{
	conf_mult_obj *list = NULL;
	config_node *cur = NULL;
	g_mutex_lock(cfg->lock);
	cur = cfg_get_single_value(cfg, class,key);
	if(cur && cur->type == TYPE_ITEM_MULTIPLE && cur->children != NULL)
	{
		cur = cur->children;	
		/* get first */
		while(cur->prev != NULL) cur = cur->prev;
		do {
			conf_mult_obj *temp= g_slice_new0(conf_mult_obj);
			temp->value = g_strdup(cur->value);
			temp->key = g_strdup(cur->name);
			temp->next = list;
			if(temp->next) temp->next->prev = temp;
			list = temp;
			cur = cur->next;
		}while(cur != NULL);		
	}
	g_mutex_unlock(cfg->lock);
	return list;
}
void cfg_free_multiple(conf_mult_obj *data)
{
	conf_mult_obj *list = data;
	if(list == NULL) return;
	/* go to first */
	while(list->prev != NULL)
	{
		list = list->prev;
	}
	while(list != NULL)
	{
		if(list->value)	cfg_free_string(list->value);
		if(list->key)	cfg_free_string(list->key);  		
		if(list->next != NULL)
		{
			if(list->prev)g_slice_free(conf_mult_obj,list->prev);
			list = list->next;
		}
		else{
			g_slice_free(conf_mult_obj,list->prev);
			g_slice_free(conf_mult_obj,list);
			list = NULL;
		}
	}
}

conf_mult_obj *cfg_get_class_list(config_obj *data)
{
	conf_mult_obj *list = NULL;
	config_node *root = NULL;
	if(!data) return NULL;
	if(!data->root) return NULL;
	g_mutex_lock(data->lock);
	root = data->root;
	while(root->prev != NULL) root = root->prev;
	do {
		if(root->type == TYPE_CATEGORY)
		{
			conf_mult_obj *temp= g_slice_new0(conf_mult_obj);
			temp->value = NULL;
			temp->key = g_strdup(root->name);
			temp->next = list;
			if(temp->next) temp->next->prev = temp;
			list = temp;
		}
		root=root->next;
	}while(root!= NULL);		
	while(list->prev != NULL) list = list->prev;
	g_mutex_unlock(data->lock);
	return list;
}

conf_mult_obj *cfg_get_key_list(config_obj *data,char *class)
{
	conf_mult_obj *list = NULL;
	config_node *root = NULL;
	if(data == NULL) return NULL;
	if(data->root == NULL) return NULL;
	g_mutex_lock(data->lock);
	root = cfg_get_class(data,class);
	if(!root || !root->children)
	{
		g_mutex_unlock(data->lock);
		return NULL;
	}
	root = root->children;
	while(root->prev != NULL) root = root->prev;
	do {
		if(root->type == TYPE_ITEM)
		{
			conf_mult_obj *temp= g_slice_new0(conf_mult_obj);
			temp->value = g_strdup(root->value);
			temp->key = g_strdup(root->name);
			temp->next = list;
			if(temp->next) temp->next->prev = temp;
			list = temp;
		}
		root=root->next;
	}while(root!= NULL);
	while(list->prev != NULL) list = list->prev;
	g_mutex_unlock(data->lock);
	return list;
}

static void __int_cfg_do_special_cleanup(config_obj *cfg, config_node *node)
{
	config_node *item,*root = NULL;
	if(!cfg || !cfg->root)
		return;
	if(node == NULL)
		root = cfg->root;
	else
		root = node;
	if(!root)return;
	while(root)
	{
		if(root->type == TYPE_CATEGORY)
		{
			if(root->children)
				__int_cfg_do_special_cleanup(cfg, root->children);		
			if(root->children == NULL)
			{
				item = NULL;
				if(root->prev)
					item =  root->prev;
				else if(root->next)
					item = root->next;
				__int_cfg_remove_node(cfg, root);	
				root = item;
			}
		}	
		else if(root->type == TYPE_ITEM)
		{
			if(root->value == NULL || root->value[0] == '\0' || strlen(root->value) == 0 )
			{
				config_node *node = root;
				root = node->prev;
				if(!root)
					root = node->next;
				__int_cfg_remove_node(cfg, node);
				if(!root) return;
			}
		}
        if(root)
            root = root->next;
	}
}
void cfg_do_special_cleanup(config_obj *cfg)
{
	if(!cfg || !cfg->root)
		return;

	g_mutex_lock(cfg->lock);
	__int_cfg_do_special_cleanup(cfg, NULL);
	cfg_save(cfg);
	g_mutex_unlock(cfg->lock);
}
