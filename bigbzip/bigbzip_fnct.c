/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   bigbzip_fnct.c 
   Ver 1.0    20-may-2005

   Big_bzip is a variant of bzip in which the BWT transform
   is applied to the whole text, and not to blocks of size
   up to 1Mb. 
   In this file we offer a set of functions for implementing 
   the compression and decompression stages of big_bzip. 
   It includes MTF, RLE0 (Wheeler's code), and 
   BWT compressors and decompressors. We use Multi-Table
   Huffman as final 0th order compressor.

   This set of functions exploit the ds_ssort library
   by Giovanni Manzini, and the bzip2 and/or libbzip2 library
   by Julian Seward. Both these libraries are GNU-GPL.

   Copyright (C) 2005 Paolo Ferragina (ferragina@di.unipi.it)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

   See COPYRIGHT file for further copyright information	   
   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

#include "mytypes.h"
#include "bigbzip.h"
#include "ds_ssort.h"

/* ----------------------------------------------------------------
	Procedure bigbzip_compress()

	text: text to be compressed
	text_len: length of text
	ctext: (Reference to the) compressed data
	ctext_len: (Reference to the) length of compressed data
						
	The first 4 bytes of the compressed data indicate the size of 
	text, and the next 4 bytes indicate the row
	of the BWT matrix containing this text.
	ctext is allocated inside this procedure.
	----------------------------------------------------------------- */
void bigbzip_compress(UChar text[], int text_len, 
					  UChar *ctext[], int *ctext_len)
{
	void init_buffer(UChar mem[], int size);
	void bbz_bit_write(int n, int vv);
	void fatal_error(char *s);
	int text_row,mtfc_len;
	UChar *bwtc, *mtfc;

	// ---------------------------------------------------------------------
	// PHASE I: BWT calculation
	// Calculates also the row containing the text in the BWT matrix
	// ---------------------------------------------------------------------
	
	bwtc = (UChar *) malloc(text_len * sizeof(UChar));    
	if (!bwtc) fatal_error("error in allocating BWT array!\n");
	bwt(text, bwtc, &text_row, text_len);

	// ---------------------------------------------------------------------
	// PHASE II: MTF encoding
	// ---------------------------------------------------------------------
	
	mtfc= (UChar *) malloc(text_len * sizeof(UChar));   
	if (!mtfc) fatal_error("error in allocating MTF array!\n");
	mtf(bwtc, mtfc, text_len);        
	mtfc_len=text_len;
	free(bwtc);
	
	// ---------------------------------------------------------------
	// PHASE III: MultiTable Huffman encoding (bzlib)
	//	This includes an RLE stage
	// ----------------------------------------------------------------

	// Overestimate the compressed text size (whose length is unknown)
	*ctext_len = max(2000, floor(1.1 * text_len));
	*ctext = (UChar *) malloc((*ctext_len) * sizeof(UChar));
	if (! (*ctext) ) fatal_error("Failed allocating the compressed data!\n");

	// Write the prologue of the compressed data
	init_buffer(*ctext,10);
	bbz_bit_write(32, text_len);
	bbz_bit_write(32, text_row);
	*ctext_len -=8; // "discards" the initial 8 bytes
	
	// Multi-Table Huffamn compression
	multihuf_compr(mtfc, mtfc_len, (*ctext) + 8, ctext_len);
	
	(*ctext_len) += 8; // "considers" again the initial 8 bytes
	
	free(mtfc);
	realloc(*ctext,*ctext_len); // adjusts memory to fit compressed data
} 


/* ----------------------------------------------------------------
	Procedure decompress_bigbzip()

	ctext: The compressed data
	ctext_len: The length of the compressed data
	text: (Reference to the) decompressed text 
	text_len: (Pointer to the) length of decompressed text
	
	The first 4 bytes of the compressed data indicate the size of the
	text we have compressed, and the next 4 bytes indicate the row
	of the BWT matrix containing this text.
	text is allocated inside this procedure.
	----------------------------------------------------------------- */

void bigbzip_decompress(UChar ctext[], int ctext_len, 
						UChar *text[], int *text_len)
{
	void init_buffer(UChar mem[], int size);
	int bbz_bit_read(int n);
	void fatal_error(char *s);
	UChar *mtfc, *bwtc;	 
	int text_row, mtfc_len;
  	
	// temporary buffer to read the header
	init_buffer(ctext,ctext_len); 
	
	// read the length of the original text
	*text_len = bbz_bit_read(32);
	*text = (UChar *) malloc((*text_len) * sizeof(UChar));

	// Position of the text_row in the BWT
	text_row = bbz_bit_read(32);

	// ---------------------------------------------------------------
	// PHASE I: MultiTable Huffamn decoding
	// ----------------------------------------------------------------
	mtfc_len = *text_len;
	mtfc = (UChar *) malloc(mtfc_len * sizeof(UChar));   
	if (!mtfc) fatal_error("error in allocating MTF array!\n");
	
	// Sets mtfc_len to the length of the decompressed data
	multihuf_decompr(ctext+8, ctext_len-8, mtfc, &mtfc_len);
	*text_len=mtfc_len; // adjusts *text_len

	// ---------------------------------------------------------------------
	// PHASE II: MTF decoding
	// ---------------------------------------------------------------------
	bwtc = (UChar *) malloc((*text_len) * sizeof(UChar));   
	if (!bwtc) fatal_error("error in allocating BWT array!\n");
	unmtf(mtfc, bwtc, *text_len);
	free(mtfc);

	// ---------------------------------------------------------------------
	// PHASE III: BWT inversion
	// ---------------------------------------------------------------------
	*text = (UChar *) malloc((*text_len) * sizeof(UChar));   
	if (! (*text)) fatal_error("error in allocating MTF array!\n");
	unbwt(bwtc, *text, text_row, *text_len);

	free(bwtc);
	realloc(*text, *text_len);
} 



/* ------------------------------------------------
	Procedure to MTF encoding [UChar+ --> UChar+]

	in: array of symbols to mtf-code
	out: output array of the mtf-coding 
	length: number of symbols to MTF-encode

	out must have been preallocated to in's size
	---------------------------------------------- */
void mtf(UChar in[], UChar out[], int length)
{
  int i,j,k;
  UChar c, mtf_list[256];

  for (i=0; i<256; i++)  // Initialize the MTF-list
    mtf_list[i]= (UChar) i;
  for (i=0; i < length; i++) {
    c=in[i];   // symbol to process
    for (j=0; mtf_list[j] != c; j++) ; // mtf_list rank of c
    out[i]=j;
	for (k=j; k>0; k--) // move-to-front c
		mtf_list[k]=mtf_list[k-1];
	mtf_list[0] = c;
  }
}


/* -------------------------------------------------------------
   Procedure to undo the MTF-coding [UChar+ --> UChar+]

	in: array of symbols giving an mtf-code
	out: output array of the mtf decoding 
	length: number of symbols to MTF-decode

	out must have been preallocated to in's size
	-------------------------------------------------------------*/

void unmtf(UChar in[], UChar out[], int length)
{
  int i,k, pos;
  UChar mtf_list[256];

  for (i=0; i<256; i++)  // Initialize the MTF list
    mtf_list[i]= (UChar)i;
  for (i=0; i < length; i++) {
	pos= (int) in[i];
	out[i] = mtf_list[pos]; // MTF unencode the next symbol
	for (k=pos; k>0; k--) // move-to-front bwt[i]
		mtf_list[k]=mtf_list[k-1];
	mtf_list[0]=out[i];
  }
}

/* -------------------------------------------------------------------
   Procedure to RLE the 0-runs (Wheeler's code) [UChar+ --> UInt16+]

	in: array of symbols to Run-Length encode
	in_length: number of symbols to RLE
	out: output array of the rle0 encoding (UInt16)
	out_length: (reference to) length of the rle-sequence

	out must have been preallocated to a size at least in_length
    ------------------------------------------------------------------- */

void rle0_wheeler(UChar in[], int in_length, 
				  UInt16 out[], int *out_length)
{
	int bbz_int_log2(int u); 
	int i, j, num_zeros;
	int code_length;

	*out_length=0;
	for (i=0; i < in_length; ) {
		if (in[i] == (UChar) 0) {  // We met a ZERO

			for(j=i+1; (j < in_length) && (in[j] == 0); j++) ; // Count ZEROs
			num_zeros = j - i;
			num_zeros++;
			
			// Compute the Wheeler code from right to left
			// dropping the most significant digit
			code_length = bbz_int_log2(num_zeros);
			for( ; code_length > 0; code_length--) {
				out[*out_length] = (UInt16) (num_zeros % 2); //Least significant digit
				num_zeros >>= 1;
				(*out_length)++;
				}
			i=j;
			} else { // +1 to manage {0,1} symbols for Wheeler code
				out[*out_length] = (UInt16) (in[i] + 1); 
				(*out_length)++;
				i++;
				}
		}
}

/* ------------------------------------------------------------------------------
   Procedure to RLE-undo the 0-runs (Wheeler's code) [UInt16+ --> UChar+]

	in: encoded rle0 sequence
	in_length: length of the rle-sequence
	out: output array giving the UNrle 
	out_length: (Reference to) number of symbols RLE-decoded

	out must have been preallocated to a proper size to contain the UNrle(in)
  ------------------------------------------------------------------------------- */

void unrle0_wheeler(UInt16 in[], int in_length, 
					UChar out[], int *out_length)
{
	int i, j, num_zeros, code_length;

	*out_length=0;
	for (i=0; i < in_length; ) {
		if (in[i] <= 1) {  // We met a 0-run
			for(j=i+1; (j < in_length) && (in[j] <= 1); j++) ; // find RLE end
			code_length = j - i;

			// Insert the bits in a reverse manner
			// by appending a 1 to the head
			num_zeros = 1;
			for( ; code_length > 0; code_length--) {
				num_zeros <<= 1;
				num_zeros += in[i+code_length-1]; // +0 or +1
				}
			num_zeros--; // -1 since the Wheeler increment
			// Expand the 0-run in the MTF sequence
			for( ; num_zeros > 0; num_zeros--){ 
				out[*out_length] = (UInt16) 0;
				(*out_length)++;
				}
			// jump the 0-rle 
			i=j; 
			} else { // recall +1 for symbols >=1 in Wheeler code
				out[*out_length] = (UInt16) (in[i] - 1); 
				(*out_length)++;
				i++;
				}
		}
}



/* -----------------------------------------------------------------
   Procedure to compute the BWT [UChar+ --> UChar+]

	text: original text 
	bwt: BWT-transformed text 
	text_row: row containing the text in the BWT matrix
	length: length of BWT (and text)

  bwt must have been preallocated to length positions
  ----------------------------------------------------------------- */
void bwt(UChar text[], UChar bwt[], int *text_row, int length)
{
	void fatal_error(char *s);
	int overshoot, *sa, i, j;
	UChar *text_tmp;


	// ----- init ds suffix sort routine
	overshoot=init_ds_ssort(500,2000);
	if(overshoot==0) fatal_error("ds_ssort initialization failed! \n");
	
	// ----- allocate suffix array, text (because of overshoot) and bwt
	sa= (int *) malloc(length * sizeof(int));                 
	text_tmp= (UChar *) malloc((length+overshoot)* sizeof(UChar));   
	if (!sa || !text_tmp) fatal_error("SA-BWT initialization failed! \n");
  
	// ----- compute the BWT and the text_row ------
	memcpy(text_tmp,text,length);
	ds_ssort(text_tmp,sa,length); 
	free(text_tmp);  

	bwt[0] = text[length-1];
    for(i=0,j=1;i<length;i++)
		if(sa[i]>0)	bwt[j++]=text[sa[i]-1];
			else *text_row=i;

	free(sa); 
}


/* ------------------------------------------------------
   Procedure to recover the text from the BWT

	bwt: BWT-transfromed text
	text: original text
	text_row: row containing the text in the BWT matrix
	length: length of BWT and text
  
	text must have been preallocated to length positions
	------------------------------------------------------- */
void unbwt(UChar bwt[], UChar text[], int text_row, int length)
{
  void fatal_error(char *s);
  int i,j,k, *fl, occ[256];


  // Compute FL[i] as the position in the BWT 
  // of the character F[i] (the first column of the BWT matrix)
  // ignoring the eof symbol

  for(i=0;i<256;i++) occ[i]=0;
  for(i=0;i<length;i++) occ[bwt[i]]++;
  for(i=1;i<256;i++) occ[i] += occ[i-1];
  assert(occ[255]==length);
  for(i=255;i>0;i--) occ[i] = occ[i-1];
  occ[0]=0;
  
  // Mapping from column F to column L
  fl = (int *) malloc(length * sizeof(int));
  for(i=0;i<length;i++) fl[occ[bwt[i]]++] = i;

  // i is an index in column L (of the bwt)
  // j is an index in the column F
  // k is a cursor over the text to be decompressed
  k=0;
  j = text_row;   // this is the first text char
  while(1) {
    i = fl[j];     // get last column index corresponding to j
    text[k++] = bwt[i];  
    assert(i>=0 && i<length && k<=length);
    if(k==length) break;
    if(i<=text_row) j=i-1;
		else j=i;
	  }
  if(i!=0) 
    fatal_error("Error writing inverse BWT\n");  
}
