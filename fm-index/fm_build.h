int fm_build(fm_index *index, uchar *text, ulong length);

int fm_read_file(char *filename, uchar **textt, ulong *length);

int fm_build_config (fm_index *index, suint tc, double freq, 
							ulong bsl1, ulong bsl2, suint owner);
