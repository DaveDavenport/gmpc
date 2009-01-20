
#include "gmpc_test.h"







void gmpc_test_test (const mpd_Song* song) {
	mpd_Song* b;
	mpd_Song* _tmp1;
	const mpd_Song* _tmp0;
	char* _tmp2;
	g_return_if_fail (song != NULL);
	b = NULL;
	_tmp1 = NULL;
	_tmp0 = NULL;
	b = (_tmp1 = (_tmp0 = song, (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0)), (b == NULL) ? NULL : (b = (mpd_freeSong (b), NULL)), _tmp1);
	_tmp2 = NULL;
	song->artist = (_tmp2 = g_strdup ("test"), song->artist = (g_free (song->artist), NULL), _tmp2);
	(b == NULL) ? NULL : (b = (mpd_freeSong (b), NULL));
}




