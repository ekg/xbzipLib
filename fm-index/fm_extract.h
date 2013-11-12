
ulong go_back(fm_index *index, ulong row, ulong len, uchar *dest);

ulong go_forw(fm_index *s, ulong row, ulong len, uchar *dest);

int fm_unbuild(fm_index *index, uchar ** text, ulong *length); /* extract whole text */

int fm_snippet(fm_index *s, ulong row, ulong plen, ulong clen, 
               uchar *dest, ulong *snippet_length);

int fm_invert_bwt(fm_index *s);
int fm_compute_lf(fm_index *s);
