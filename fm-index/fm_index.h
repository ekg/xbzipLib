/* 	
	Index struct and function
	Rossano Venturini
*/

#include <stdlib.h>
#include "fm_common.h"  	/* Common useful functions */
#include "fm_errors.h"   	/* Implement void error_index(int e) */

#ifndef DATATYPE 
typedef unsigned char uchar;
#ifndef __USE_MISC
typedef unsigned long ulong;
#endif
#define DATATYPE 
#endif

#ifndef INTERNALDATATYPE 
#define ALPHASIZE (256)
#define SMALLFILESIZE (51201)
// 50 Kbytes
#define SMALLSMALLFILESIZE (1025)
		
#define EXT ".fmi"

/* Tipo di compressione */
#define MULTIH 	 (4) 	/* MTF + Wheeler 1/2 + Multi Huffman 	*/
#define MWG 	 (5) 	/* MTF + Wheeler 1/2 + Gamma 			*/
#define M2WG 	 (6) 	/* MTF2 + Wheeler 1/2 + Gamma 			*/
#define HUFFMAN  (7)    /* MTF + HUFFMAN  Total */
#define HUFFMAN2 (8)    /* MTF2 + HUFFMAN Total */
#define TEST 	 (9) 	/* Huffman whitout mtf, useful for DNA */

/* constants used for searching in fmi files (see search_main()) */
#define NULL_CHAR      (0)	/* used to skip the count of char-occs */
#define WHAT_CHAR_IS   (1)	/* used to retrieve the char in a given pos */
#define COUNT_CHAR_OCC (2)	/* used to count char-occs before a given pos */

int __Fm_Verbose;    	 /* if is != 0 print verbose output on stdout  */

typedef unsigned short int suint;

typedef struct {
  ulong *occ;             /* occ chars of compact alph in prev. superbuc */
  suint alpha_size;
  uchar *bool_char_map;   /* boolean map of chars occurring in this superbucket */
} bucket_lev1;
 
/* unbuild and count/locate */
typedef struct {
  uchar *text;					/* input text */
  uchar *compress;				/* compress text */
  uchar *bwt;					/* BWT of input text */
  ulong *lf;					/* lf-mapping or suffix-array*/
  ulong compress_size;			/* size of compressed file */
  ulong text_size;				/* size of text */
  bucket_lev1 *buclist_lev1; 	/* array of num_bucs buckets */
	
  /* Info readed/writed on index prologue */
  ulong bucket_size_lev1;		/* size of level 1 buckets */	
  ulong bucket_size_lev2;		/* size of level 2 buckets */
  ulong num_bucs_lev1;			/* number buckets lev 1 */
  ulong num_bucs_lev2;			/* number buckets lev 2 */
  ulong bwt_eof_pos;			/* position of EOF within BWT */
  suint alpha_size;				/* actual size of alphabet in input text */
  suint type_compression;		/* buckets lev 1 type of compression */
  double freq;					/* frequency of marked chars */
  suint owner;					/* == 0 frees the text and alloc a new with overshoot */	
  suint compress_owner;         /* == 1 frees the compress */
  suint smalltext;				/* If == 1 stores plain text without compression */

  /* Starting position (in byte) of each group of info */
  ulong start_prologue_info_sb;	/* byte inizio info sui superbuckets */
  ulong start_prologue_info_b;	/* byte inizio posizioni buckets */
  uchar *start_prologue_occ;	/* byte inizio posizioni marcate */
  ulong start_positions;        /* byte inizio posizioni marcate per build */
  ulong *start_lev2;			/* starting position of each buckets in compr file */

  /* Chars remap info */
  suint bool_char_map[ALPHASIZE];/* is 1 if char i appears on text */
  uchar char_map[ALPHASIZE];	 /* cm[i]=j say that chars i is remapped on j */
  uchar inv_char_map[ALPHASIZE]; /* icm[i] = j say that j was remapped on i */

  /* Multiple locate. Chars substitution */
  uchar specialchar;			/* carattere speciale che indica marcamento */
  uchar subchar;				/* carattere sostituito dal carattere speciale */

  // Da spostare in una struct working_space
  /* Running temp info of actual superbucket and bucket */
  ulong pfx_char_occ[ALPHASIZE];	/* i stores # of occ of chars 0.. i-1 in the text */
  suint	bool_map_sb[ALPHASIZE]; 	/* info alphabet for superbucket to be read */
  uchar inv_map_sb[ALPHASIZE]; 	/* inverse map for the current superbucket */
  suint alpha_size_sb;	  					/* current superbucket alphasize */
  
  uchar	bool_map_b[ALPHASIZE];		/* info alphabet for the bucket to be read */
  uchar inv_map_b[ALPHASIZE];		/* inverse map for the current superbucket */
  suint alpha_size_b;     					/* actual size of alphabet in bucket */
  
  uchar mtf[ALPHASIZE];  		/* stores MTF-picture of bucket to be decompressed */
  uchar *mtf_seq;				/* store bucket decompressed */
  ulong occ_bucket[ALPHASIZE];  /* number chars occurences in the actual bucket needed by Mtf2 */
  suint int_dec_bits; 			/* log2(log2(text_size)) */
  suint log2textsize; 			/* int_log2(s.text_size-1) */
  suint var_byte_rappr;   		/* variable byte-length repr. (log2textsize+7)/8;*/
  ulong num_marked_rows;		/* number of marked rows */
  ulong bwt_occ[ALPHASIZE];     /* entry i stores # of occ of chars 0..i-1 */
  ulong char_occ[ALPHASIZE];	/* (useful for mtf2) entry i stores # of occ of char i == bwt_occ[i]-bwt_occ[i-1] */
  suint skip;			/* 0 no marked pos, 1 all pos are marked, 2 only a char is marked*/
  suint sb_bitmap_size;			/* size in bytes of the bitmap of superbuckets */			
  
  /* Needed by fm_build */
  ulong *loc_occ;				/* Positions of marcked rows */
  
} fm_index;

/* Report rows from mulri_count */
typedef struct {
	ulong first_row; 			/* riga inizio occorrenza */
	ulong elements;   			/* numero occorrenze */	
	} multi_count;

#define INTERNALDATATYPE
#endif
