
#ifndef __GMPC_TEST_H__
#define __GMPC_TEST_H__

#include <glib.h>
#include <glib-object.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>

G_BEGIN_DECLS



#define GMPC_MISC_plugin (memset (&_tmp0, 0, sizeof (gmpcPlugin)), _tmp0.type = 1, _tmp0.name = "Test Plugin", _tmp0)
void gmpc_misc_test (const MpdOb* server, const mpd_Song* song);


G_END_DECLS

#endif
