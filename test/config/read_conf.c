#include <stdio.h>
#include <glib.h>
#include "config1.h"
void print_all_entries(config_obj *cfg)
{
    conf_mult_obj *iter, *list;
    iter = list = cfg_get_class_list(cfg);
    while(iter){
        printf("[%s]\n", iter->key);
        conf_mult_obj *kiter, *klist;
        kiter = klist = cfg_get_key_list(cfg, iter->key);
        while(kiter)
        {
            printf("%s=%s\n", kiter->key, kiter->value);
            kiter = kiter->next;
        }
        cfg_free_multiple(klist);
        iter = iter->next;
    }
    cfg_free_multiple(list);
}
int main(int argc, char **argv)
{
    gchar *path= "read_conf_test.db2";
    config_obj *cfg;
    if(argc == 2){
        path = argv[1];
    }
    /* Open the test file */
    cfg = cfg_open(path);
    print_all_entries(cfg);
    /* Close the test file */
    cfg_close(cfg);
}
