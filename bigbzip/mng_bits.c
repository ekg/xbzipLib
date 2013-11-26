/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	mng_bits.c
    Ver 1.0    20-may-2005

	Set of functions to manage a BitBuffer which has been
	initialized outside, with a proper byte size.

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

UChar*  __BufferAddr;     // memory address where to write/read 
int		__ProcdBytes;    // #bytes read or written 
int		__BufferSize;    // number of bytes in the buffer
UInt32  __32BitBuffer;   // 32bit Word to manage bits going in/from buffer
int		__32BitBufferFill; // Current #bits present in the 32bit Word


/* To initialize the memory buffer for reading/writing */   
void init_buffer(UChar *mem, int size) {
	void fatal_error(char *s);

	__BufferAddr = mem;
	__ProcdBytes = 0;
	__BufferSize = size;
	__32BitBuffer = 0;
	__32BitBufferFill = 0;
	if (!__BufferAddr) 
		fatal_error("Internal buffer for bit read/write is NULL\n");
	}

int get_buffer_fill (void)
	{
	return(__ProcdBytes);
	}


/* (#bits <=24, integer to encode) */
#define bit_write24(__n, __vv)  {									\
	{UInt32 __v = (UInt32) (__vv); 									\
  	assert(__32BitBufferFill <8); 									\
	assert(__n>0 && __n<=24);	 									\
	assert( __v < 1u << __n );										\
	if(__ProcdBytes + ((__32BitBufferFill + __n)/8) > __BufferSize) \
	{ fatal_error("Buffer too small!\n");}							\
	__32BitBufferFill += (__n); 									\
	__32BitBuffer |= (__v << (32 - __32BitBufferFill));				\
	while (__32BitBufferFill >= 8) {            					\
	*__BufferAddr = (__32BitBuffer>>24); 							\
	__BufferAddr++; 												\
    	__ProcdBytes++;												\
		__32BitBuffer <<= 8; 										\
		__32BitBufferFill -= 8;										\
		}															\
	}}
								   
/* (#bits <=24, result of reading) */
#define bit_read24(__n, __r)	{									\
	{ UInt32 __t,__u;												\
	assert(__32BitBufferFill<8);									\
	assert(__n>0 && __n<=24);										\
	/* --- read 8 bits until size>= n --- */						\
	while(__32BitBufferFill < __n) {								\
		__t = (UInt32) *__BufferAddr;								\
		__BufferAddr++; __ProcdBytes++;								\
			__32BitBuffer |= (__t << (24-__32BitBufferFill));		\
			__32BitBufferFill += 8;									\
		}															\
			/* ---- write n top bits in u ---- */					\
		__u = __32BitBuffer >> (32-__n);							\
		/* ---- update buffer ---- */								\
		__32BitBuffer <<= __n;										\
		__32BitBufferFill -= __n;									\
		__r = ((int)__u);											\
	} }


/* -----------------------------------------------------------------------------
   Functions to read/write more than 24 bits 
   ----------------------------------------------------------------------------- */

// ****** Write in 32BitBuffer n bits taken from vv (possibly n > 24) 
void bbz_bit_write(int n, int vv)
{  
  void fatal_error(char *s);
  
  UInt32 v = (UInt32) vv;
  assert(n <= 32);
  if (n > 24){
	  bit_write24((n-24), (v>>24 & 0xffL));
	  bit_write24(24, (v & 0xffffffL));
	} else {
		bit_write24(n,v);
	}
}

// ****** Read n bits from 32BitBuffer 
int bbz_bit_read(int n)
{  
  void fatal_error(char *s);
  
  UInt32 u = 0;
  int i;
  assert(n <= 32);
  if (n > 24){
	bit_read24((n-24),i);
    u =  i<<24;
	bit_read24(24,i);
    u |= i;
    return((int)u);
  } else {
    bit_read24(n,i);
	return i;
  }
}


// *****  Complete with zeroes the first byte of 32BitBuffer
// *****  This way, the content of 32BitBuffer is entirely flushed out

void bbz_bit_flush( void )
{
  void fatal_error(char *s);

  if(__32BitBufferFill != 0)
    bit_write24((8 - (__32BitBufferFill % 8)) , 0);  // pad with zero !
}

// **** Symmetric to the bit_flush() but used in bitread()
void bbz_byte_align( void )
{
	__32BitBufferFill = 0;
}

// Compute the logarithm in base 2 of integer u
int bbz_int_log2(int u)    
{
  int i = 0;
  int r = 1;
  
  assert(u>0);
  while((i<=32) && (r<u)){
    r=2*r+1;
    i = i+1;
  }
    
  assert(i<=32);
  return i;
}

void fatal_error(char *s)
{
  fprintf(stderr,"%s",s);
  exit(1);
}

