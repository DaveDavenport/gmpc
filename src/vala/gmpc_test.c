
#include "gmpc_test.h"
#include <plugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>







void gmpc_misc_test (MpdObj* server, const mpd_Song* song) {
	gmpcPlugin _tmp0 = {0};
	mpd_Song* b;
	mpd_Song* _tmp1;
	const mpd_Song* _tmp0;
	char* _tmp2;
	g_return_if_fail (server != NULL);
	g_return_if_fail (song != NULL);
	b = NULL;
	_tmp1 = NULL;
	_tmp0 = NULL;
	b = (_tmp1 = (_tmp0 = song, (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0)), (b == NULL) ? NULL : (b = (mpd_freeSong (b), NULL)), _tmp1);
	_tmp2 = NULL;
	song->artist = (_tmp2 = g_strdup ("test"), song->artist = (g_free (song->artist), NULL), _tmp2);
	fprintf (stdout, "test: %s\n", GMPC_MISC_plugin.name);
	(b == NULL) ? NULL : (b = (mpd_freeSong (b), NULL));
}




