
int count_row_mu(fm_index *index, uchar *pattern, ulong len, ulong sp, ulong ep);

int fm_multi_count(fm_index *index, uchar *pattern, ulong len, multi_count **list);

int fm_boyermoore(fm_index *s, uchar * pattern, ulong length, ulong ** occ, ulong * numocc);
