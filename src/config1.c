/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
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
#include "config1.h"
void cfg_remove_node(config_obj *cfg, config_node *node);
config_node *cfg_add_class(config_obj *cfg, char *class);
config_node *cfg_new_node();
void cfg_add_child(config_node *parent, config_node *child);
	
void cfg_open_parse_file(config_obj *cfgo, FILE *fp)
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
			}
			if(len==0)
			{
				multiple = NULL;
			}
			/* seek end of line */
			while(c != EOF && c != '\n') c = fgetc(fp);
		}
		
		/* next, ignore commants  and there must be a category*/
		else if(cur && (c  == '#' || c == '/' || c == '\n')){
			while(c != EOF && c != '\n') c = fgetc(fp);
		}
		else if(cur){
			config_node *new = NULL;
			gchar *key = NULL;
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
				}while((c != '\n' || quote) && c != EOF && quote >= 0);
				new->value = g_strndup(buffer, len);
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

	cfgo->url = g_strdup(url);
	cfgo->root = NULL;
	if(g_file_test(cfgo->url, G_FILE_TEST_EXISTS))
	{
		FILE *fp = fopen(cfgo->url, "r");
		if(fp)
		{
			cfg_open_parse_file(cfgo, fp);
			fclose(fp);
		}
	}

	return cfgo;
}


void config_close(config_obj *cfgo)
{
	if(cfgo == NULL)
	{
		return;
	}
	if(cfgo->url != NULL)
	{
		g_free(cfgo->url);
	}
	while(cfgo->root)cfg_remove_node(cfgo,cfgo->root);
	g_free(cfgo);
}
config_node *cfg_new_node()
{
	config_node *newnode = g_malloc0(sizeof(config_node));
	newnode->type = TYPE_CATEGORY;
	newnode->name = NULL; 
	newnode->next = NULL;
	newnode->prev = NULL;                                 
	newnode->parent = NULL;
	newnode->value = NULL;
	return newnode;
}
config_node *cfg_add_class(config_obj *cfg, char *class)
{
	config_node *newnode = cfg_new_node();
	newnode->type = TYPE_CATEGORY;
	newnode->name = g_strdup(class);
	newnode->value = NULL;
	newnode->children  = NULL;
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

void cfg_save_category(config_obj *cfg, config_node *node, FILE *fp)
{
	config_node *temp = NULL;
	if(node == NULL)return;
	/* find the first */
	while(node->prev != NULL) node = node->prev;
	/* save some stuff */
	for(temp = node;temp != NULL; temp = temp->next){
		if(temp->type == TYPE_CATEGORY)
		{
			fprintf(fp, "[%s]\n",temp->name);
			cfg_save_category(cfg,temp->children,fp);
		}
		if(temp->type == TYPE_ITEM_MULTIPLE)
		{
			fprintf(fp, "{%s}\n",temp->name);
			cfg_save_category(cfg,temp->children,fp);
			fprintf(fp, "{}\n");
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

void cfg_save(config_obj *cfgo)
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

	}
	//	xmlSaveFormatFile(cfgo->url, cfgo->xmldoc,1);
	return;
}

config_node *cfg_get_class(config_obj *cfg, char *class)
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

config_node *cfg_get_single_value(config_obj *cfg, char *class, char *key)
{
	/* take children */
	config_node *cur = cfg_get_class(cfg, class);

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

void cfg_free_string(char *string)
{
	if(string != NULL)
	{
	}
}

char * cfg_get_single_value_as_string(config_obj *cfg, char *class, char *key)
{
	config_node *cur = cfg_get_single_value(cfg, class,key);
	if(cur != NULL)
	{
		if(cur->type == TYPE_ITEM)
		{
			return (char *)cur->value;
		}
	}
	return NULL;
}

char * cfg_get_single_value_as_string_with_default(config_obj *cfg, char *class, char *key , char *def)
{
	char *retv = cfg_get_single_value_as_string(cfg,class,key);
	if(retv == NULL)
	{
		cfg_set_single_value_as_string(cfg,class,key,def);
		retv = cfg_get_single_value_as_string(cfg,class,key);
	}
	return retv;
}

int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key)
{
	config_node *cur = cfg_get_single_value(cfg, class,key);
	if(cur != NULL)
	{
		if(cur->type == TYPE_ITEM)
		{
			return (int)g_ascii_strtoull(cur->value,NULL,0);	

		}
	}
	return CFG_INT_NOT_DEFINED;
}

void cfg_set_single_value_as_int(config_obj *cfg, char *class, char *key, int value)
{
	gchar *	temp = g_strdup_printf("%i",value);
	cfg_set_single_value_as_string(cfg,class,key,temp);
	g_free(temp);
}
int cfg_get_single_value_as_int_with_default(config_obj *cfg, char *class, char *key, int def)
{
	int retv = cfg_get_single_value_as_int(cfg,class,key);
	if(retv == CFG_INT_NOT_DEFINED)
	{
		cfg_set_single_value_as_int(cfg,class,key,def);
		retv = cfg_get_single_value_as_int(cfg,class,key);		
	}
	return retv;
}
/* float */
float cfg_get_single_value_as_float(config_obj *cfg, char *class, char *key)
{
	char * temp = cfg_get_single_value_as_string(cfg,class,key);
	float result = 0;
	if(temp == NULL)
	{
		return CFG_INT_NOT_DEFINED;
	}
	/* make it return an error */
	result = g_ascii_strtod(temp,NULL);
	return result;
}

void cfg_set_single_value_as_float(config_obj *cfg, char *class, char *key, float value)
{
	char *value1 = g_strdup_printf("%f", value);
	cfg_set_single_value_as_string(cfg,class,key,value1);
	g_free(value1);
}
float cfg_get_single_value_as_float_with_default(config_obj *cfg, char *class, char *key, float def)
{
	float retv = cfg_get_single_value_as_float(cfg,class,key);
	if(retv == CFG_INT_NOT_DEFINED)
	{
		cfg_set_single_value_as_float(cfg,class,key,def);
		retv = cfg_get_single_value_as_float(cfg,class,key);		
	}
	/* make it return an error */
	return retv;
}
void cfg_remove_node(config_obj *cfg, config_node *node)
{
	if(node->type != TYPE_ITEM)
	{
		printf("Still children in the node, going to delete it recursive\n");
		while(node->children)
		{
			cfg_remove_node(cfg,node->children);
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
	if(node->name) g_free(node->name);
	if(node->value) g_free(node->value);
	g_free(node);
}
void cfg_del_single_value(config_obj *cfg, char *class, char *key)
{
	config_node *node = cfg_get_single_value(cfg,class,key);
	if(node == NULL) return;
	cfg_remove_node(cfg,node);
	cfg_save(cfg);
}

void cfg_set_single_value_as_string(config_obj *cfg, char *class, char *key, char *value)
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
		cfg_add_child(node,newnode);

	}	
	newnode->type = TYPE_ITEM;
	if(newnode->value) g_free(newnode->value);
	newnode->value = g_strdup(value);	
	cfg_save(cfg);
}

/* multiple values */


config_node *cfg_get_multiple_value(config_obj *cfg, char *class, char *key, char* id)
{
	config_node *node = cfg_get_single_value(cfg,class,key);
	if(node == NULL){
	 	printf("node %s not found\n", key);      
		return NULL;
	}
	if(node->type != TYPE_ITEM_MULTIPLE){
		printf("no multiple\n");
	       	return NULL;
	}
	if(node->children == NULL){
	 	printf("empty multiple\n");      
		return NULL;
	}
	/* first*/
	node = node->children;
	while(node->prev != NULL) node = node->prev;
	/* get item */
	for(; node != NULL;node = node->next) {
		printf("%s\n", node->name);
		if(!strcmp(node->name, id)) 
		{
			return node;
		}
	}
	return NULL;
}

void cfg_del_multiple_value(config_obj *cfg, char *class, char *key,char *id)
{
	config_node *cur = cfg_get_multiple_value(cfg, class,key,id);
	if(cur != NULL)
	{
		cfg_remove_node(cfg,cur);
		cfg_save(cfg);
	}
}

void cfg_set_multiple_value_as_string(config_obj *cfg, char *class, char *key, char *id, char *value)
{
	config_node *cur = cfg_get_multiple_value(cfg, class,key,id);
	printf("found node: %p\n",cur);
	if(cur != NULL)
	{
		if(cur->value) g_free(cur->value);
		cur->value = g_strdup(value);
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
			node = cur;
		}
		if(node->type != TYPE_ITEM_MULTIPLE) return;
		cur = cfg_new_node();
		cur->type = TYPE_ITEM;
		cur->name = g_strdup(id);
		cur->value = g_strdup(value);
		cfg_add_child(node,cur);
		cfg_save(cfg);
	}
}

conf_mult_obj * cfg_get_multiple_as_string(config_obj *cfg, char *class, char *key)
{
	config_node *cur = cfg_get_single_value(cfg, class,key);
	if(cur && cur->type == TYPE_ITEM_MULTIPLE && cur->children != NULL)
	{
		conf_mult_obj *list = NULL;
		cur = cur->children;	
		/* get first */
		while(cur->prev != NULL) cur = cur->prev;
		do {
			conf_mult_obj *temp= g_malloc0(sizeof(conf_mult_obj));
			temp->value = cur->value;
			temp->key = cur->name;
			temp->next = list;
			if(temp->next) temp->next->prev = temp;
			list = temp;
			cur = cur->next;
		}while(cur != NULL);		
		return list;
	}
	return NULL;
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
		if(list->next != NULL)
		{
			if(list->prev)g_free(list->prev);
			list = list->next;
		}
		else{
			g_free(list->prev);
			g_free(list);
			list = NULL;
		}
	}
}
