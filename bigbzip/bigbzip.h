/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	bigbzip.h
    Ver 1.0    20-may-2005

	Prototypes for the functions of the library bigbzip. 
    In order to use it you need the ds_ssort library
    to build the suffix array, by Giovanni Manzini, and
    the bzip2 and/or libbzip2 library to execute the 
	Multi-Table Huffman compressor, by Julian Seward.

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
	

// ------------------------------------------------------------------------
// Main interface
//
// The two procedures below take care of allocating the memory for
// the output array and set its length properly
// -------------------------------------------------------------------------

void bigbzip_compress(UChar text[], int text_len, UChar *ctext[], int *ctext_len);
void bigbzip_decompress(UChar ctext[], int ctext_len, UChar *text[], int *text_len);


// ------------------------------------------------------------------------
// Auxiliary functions
// -------------------------------------------------------------------------
void bwt(UChar text[], UChar  bwt[], int *text_row, int length);
void unbwt(UChar bwt[], UChar text[], int text_row, int length);

void mtf(UChar bwt[], UChar mtfc[], int length);
void unmtf(UChar mtfc[], UChar bwt[], int length);

void rle0_wheeler(UChar mtfc[], int mtf_length, UInt16 rle[], int *rle_length);
void unrle0_wheeler(UInt16 rle[], int rle_length, UChar mtfc[], int *mtf_length);

void multihuf_compr(UChar in[], int in_len, UChar out[], int *out_len);
void multihuf_decompr(UChar in[], int in_len, UChar out[], int *out_len);

// ------------------------------------------------------------------------
// Other functions
// -------------------------------------------------------------------------
void init_buffer(UChar *mem, int size);
int get_buffer_fill (void);
void bbz_bit_write(int n, int vv);
int bbz_bit_read(int n);
void bbz_bit_flush( void );
void bbz_byte_align( void );
int bbz_int_log2(int u);
void fatal_error(char *s);
