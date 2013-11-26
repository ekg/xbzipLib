/***************************************************************************
 *   Copyright (C) 2005 by Paolo Ferragina, Università di Pisa             *
 *   Contact address: ferragina@di.unipi.it								   *
 *																		   *
 *   API interface and data types of the library xbzip.h            	   *
 *																		   *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


// Basic data types
#include "xbzip_types.h" 

// ------------- To use BigBzip ---------- 
#include "bigbzip.h"

// ------------- To use Zlib ---------- 
#include "zlib.h"

// ------------- To use FM-index vers 2---------- 
#include "interface.h"


// ------------- To manage XML data ----------
#include <expat.h> 

// -------------- SOME MACROS ----------------

#ifndef min
#define min(a, b) ((a)<=(b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a)>=(b) ? (a) : (b))
#endif

/* macro to detect and notify errors  */
#define IFERROR(error) { \
	{if (error) { fprintf(stderr, "%s\n", error_index(error)); exit(1); }} \
	}

#define __START_TIMER__ {					\
	start_partial_timer=getTime();			\
	}

#define __END_TIMER__ {									\
	end_partial_timer = getTime();						\
	tot_partial_timer = (end_partial_timer - start_partial_timer);\
	}

// ------------------------------------------------------------------------
// Main interface, available in xbzip_fnct.c
//
// The two procedures below take care of allocating the memory for
// the output array and set its length properly
// -------------------------------------------------------------------------

void xbzip_compress(UChar text[], int text_len, UChar *ctext[], int *ctext_len, UChar flag);
void xbzip_decompress(UChar ctext[], int ctext_len, UChar *text[], int *text_len, UChar flag);

void xbzip_index(UChar text[], int text_len, UChar *disk[], int *disk_len);
void xbzip_deindex(UChar disk[], int disk_len, UChar *text[], int *text_len);

void xbzip_search(xbwt_index_type *index, UChar **path, int pathlen, 
				 int *first, int *last, int *pathocc, int *occ, int flag);



// -----------------------------------------------------------
// You find the functions below in xbzip_fnct_compr.c 
// -----------------------------------------------------------
void xbzip_compress(UChar text[], int text_len, UChar *ctext[], int *ctext_len, UChar flag);
void xbzip_decompress(UChar ctext[], int ctext_len, UChar *text[], int *text_len, UChar flag);

void xbwt_builder(UChar *text, int text_len, xbwt_type *xbwt);
void xbwt_unbuilder(xbwt_type *xbwt, UChar **text, int *text_len);
void xbwt2xbwtstr(xbwt_type *xbwt, xbwt_string_type *xbwtstr);
void xbwtstr2xbwt(xbwt_string_type *xbwtstr, xbwt_type *xbwt);
void xbwtstr2compr(xbwt_string_type *xbwtstr, UChar *ctext[], int *ctext_len, UChar flag);
void compr2xbwtstr(UChar ctext[], int ctext_len, xbwt_string_type *xbwtstr, UChar flag);
void unfuse_alpha_last(UChar *fused, int fused_len, UChar *alpha[], UChar *last[], 
					   int *alphalen, int *lastlen);
void fuse_alpha_last(xbwt_string_type *xbwtstr, UChar *fused[], int *fused_len);


// ----------------------------------------------------------
// You find the functions below in xbzip_fnct_index.c
// ----------------------------------------------------------

// Main indexing, searching and printing functions
void xbzip_index(UChar text[], int text_len, UChar *disk[], int *disk_len);
void xbzip_deindex(UChar disk[], int disk_len, UChar *text[], int *text_len);
void xbzip_search(xbwt_index_type *index, UChar **path, int pathlen, 
				 int *first, int *last, int *pathocc, int *occ, int flag);
void Subtree2Text(xbwt_index_type *index, int row, int *printed_row, 
				  UChar **snippet, int *snippetLength);

// Tree navigation functions
UChar get_node_type(xbwt_index_type *index, int row);
void get_node_labelNcode(xbwt_index_type *index, int row, UChar **symb, int *symbcode);
int get_children(xbwt_index_type *index, int row, int *first, int *last);
int get_parent(xbwt_index_type *index, int row);
int get_ith_symb_child(xbwt_index_type *index, int row, int rank, UChar *c);
int get_text_content(xbwt_index_type *index, int row, int *cLen, UChar **c);

// Basic functions for indexing the compressed data
int rank1_last(xbwt_index_type *index, int pos);
int select1_last(xbwt_index_type *index, int pos);
int selectSymb_alpha(xbwt_index_type *index, UChar *q, int rank);
int rankSymb_alpha(xbwt_index_type *index, UChar *q, int pos);
int get_symbol_code(xbwt_index_type *index, UChar *q);
void compress_block(uchar *source, int sourceLen, uchar **dest, int *destLen);
void decompress_block(uchar *source, int sourceLen, uchar **dest, int *destLen);


// Basic functions for data-structure (de)coding
void index2xbwtstr(xbwt_index_type *index, xbwt_string_type *xbwtstr);
void xbwtstr2index(xbwt_string_type *xbwtstr, xbwt_index_type *index);
void index2disk(xbwt_index_type *index, UChar **ctext, int *ctext_len);
void disk2index(UChar *ctext, int ctext_len, xbwt_index_type *index);


// TO BE DELETED
void dummy_block_search(UChar *block, int blocklen, UChar *pattern, int **occArray, int *occNum);
void xbwt_partition(UChar *text, int text_len);


// ------------------------------------------------------
// You find the functions below in data_compressor.c 
// ------------------------------------------------------
void data_compress(unsigned char *s, int slen, unsigned char **t, int *tlen);
void data_decompress(unsigned char *s, int slen, unsigned char **t, int *tlen);



// ------------------------------------------------------
// You find the functions below in xbzip_aux.c 
// ------------------------------------------------------
int PI_cmp(const void *a, const void *b);
int S_cmp(const void *a, const void *b);
void print_pretty(char c);
void print_pretty_len(char *s, int len);
void print_nodes_array(Tree_node **x, int size);
void print_xbwt(xbwt_type *x);
void print_index(xbwt_index_type *x);
void free_tree(Tree_node *u);
int log2int(int u);
double getTime ( void );


// ------------------------------------------------------
// You find the functions below in xbzip_parser.c 
// ------------------------------------------------------
void default_hndl(void *data, const char *s, int len); 
void start_hndl(void *data, const char *el, const char **attr);
void end_hndl(void *data, const char *el); 
void char_hndl(void *data, const char *s, int len);
Tree_node *xml2tree(UChar *text, int text_len, int *treesize);
void tree2nodearray(Tree_node *u, Tree_node *array[], int *cursor);

// ------------------------------------------------------
// You find the functions below in xbzip_hash.c 
// ------------------------------------------------------

void HHashtable_init(HHash_table *ht, int n);
void HHashtable_print(HHash_table *ht);
int HHashtable_func(char *s, int len, HHash_table *ht);
Hash_node *HHashtable_search(char *s, int slen, HHash_table *ht);
int HHashtable_insert(char *s, int slen, int code, HHash_table *ht); 
void HHashtable_clear(HHash_table *ht);

