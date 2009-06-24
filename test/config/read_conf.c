#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "config1.h"
void print_all_entries(config_obj *cfg, config_obj *cfg2)
{
    conf_mult_obj *iter, *list;
    iter = list = cfg_get_class_list(cfg);
    while(iter){
        printf("[%s]\n", iter->key);
        conf_mult_obj *kiter, *klist;
        kiter = klist = cfg_get_key_list(cfg, iter->key);
        while(kiter)
        {
            cfg_set_single_value_as_string(cfg2, iter->key, kiter->key, kiter->value);
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
    config_obj *cfg, *cfg2;
    if(argc == 2){
        path = argv[1];
    }
    /* Open the test file */
    cfg = cfg_open(path);
    cfg2 = cfg_open("test-db2");
    print_all_entries(cfg,cfg2);
    /* Close the test file */
    cfg_close(cfg);
    cfg_close(cfg2);
    /* Remove test file */
    g_unlink("test-db2");
    return EXIT_SUCCESS;
}
