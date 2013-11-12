/***************************************************************************
 *   Copyright (C) 2005 by Paolo Ferragina		                           *
 *   Contact address: ferragina@di.unipi.it		                           *
 *                                                                         *
 *   Description. This file contains the data types of xbzip.h and		   *
 *   all the includes.                                                     *
 *                                                                         *
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
	
/* ------------- Standard Include ---------- */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <expat.h>


/* ---------- types ----------- */
typedef int					Int32;
typedef unsigned int		UInt32;
typedef unsigned short		UInt16;
typedef unsigned char		UChar;
typedef unsigned char		Bool;
typedef unsigned long long	UInt64;
#define True   ((Bool)1)
#define False  ((Bool)0)


/* ---------- costants ----------- */

#define TEXT				0
#define TAGATTR				1

#define BIGBZIP				0
#define PLAIN				1
#define LAST				2
#define MTFMHUFF			3
#define DISTINCT			4

#define MAX_NESTING			100000

#define BUFFER_TA_SIZE		1024 * 1024 //blocks of 1Mb
#define BUFFER_NODES_SIZE	50000       // about 1Mb space (sizeof = 28 bytes)

/* ------------- global variables ---------- */
extern int Verbose;
extern int NUM1_IN_BLOCK;
extern int BLOCK_ALPHA_LEN;
extern int Last_Block_Counter;
extern int Last_Byte_Counter;
extern int Alpha_Block_Counter;
extern int Alpha_Byte_Counter;
extern int Pcdata_Block_Counter;
extern int Pcdata_Byte_Counter;


// ------------------------------------------------------------
// Data type for storing the XML tree node
// ------------------------------------------------------------
typedef struct Tree_node {
  char	*str;          // token
  int len_str;         // length of the token (to manage also NULL)
  int position;       // left-to-right numbering for "stable" sort
  int type;			  // TAGATTR or TEXT (for textual content and attribute value)
  char  *upward_path;  // sequence of tag-attr labels according to our scheme with <,@
  struct Tree_node *parent; // parent for subsequent sorting phase
  struct Tree_node *leftmost_child;    // pointer to the leftmost child
  struct Tree_node *last_child;    // pointer to the last child (for fast insertion)
  struct Tree_node *next_sibling;    // pointer to the next sibling (NULL if rightmost)
} Tree_node;


// ------------------------------------------------------------
// Data structure passed through all XML parsing functions
// ------------------------------------------------------------
typedef struct user_data {
	XML_Parser parser;
	Tree_node *stack_nodes[MAX_NESTING];
	Tree_node *buffer_nodes;
	int buffer_nodes_fill;  //size is BUFFER_NODES_SIZE
	int top_stack;
	UChar *text_buffer;
	int text_buffer_len;
	UChar *ta_buffer;
	int ta_buffer_fill;
	int ta_buffer_size;
	int counter;
	UChar *main_text;
	int empty_tag_flag;  //flag 
	UChar empty_tag_string[2]; // Special string indicating an empty tag
} user_data;

// ------------------------------------------------------------
// Data type containing all info about XBWT
// ------------------------------------------------------------
typedef struct xbwt_type {
	UChar *Slast;		
	UChar *Stype;		
	UChar **Salpha;			
	int   *LenSalpha;
	int TextLength;
	int SItemsNum;
	int PcdataItems;
	int PcdataTotLen;
	int SalphaTotLen;  // NOT counting the Pcdata 
	int TagAttrItemsTot;
	int TagAttrItemsCard;
} xbwt_type;


// ------------------------------------------------------------
// Data type containing all info about XBWT-serialized
// ------------------------------------------------------------
typedef struct xbwt_string_type {
	UChar *lastStr;		
	UChar *alphaStr;		
	UChar *pcdataStr;		
	int lastLen;
	int alphaLen;
	int pcdataLen;
	int TextLength;
	int SItemsNum;
	int TagAttrItemsCard;
	int PcdataItems;
} xbwt_string_type;


// ------------------------------------------------------------
// Data type containing all info about XBWT-index
// ------------------------------------------------------------
typedef struct xbwt_index_type {
	UChar *LastIndex;		
	int LastIndexLen;
	int *LastOffsetBlocks; // starting byte of the compressed block
	int *LastPosBlocks;    // starting position of the block (var length)
	int LastNumBlocks;

	UChar *AlphaIndex;		
	int AlphaIndexLen;
	int *AlphaOffsetBlocks;  // starting byte of the compressed block
	int *AlphaPrefixCounts;  // occurrences counted from the end of the block
	int AlphaNumBlocks;

	UChar *Alphabet;		// each item is ended by a null (includes =)
	int AlphabetLen;
	int AlphabetCard;

	UChar *PcdataIndex;
	int PcdataIndexLen;
	int *PcOffsetBlocks;	// starting byte of the compressed block
	int *PcBlockItems;		// number of Pc-items per compressed block
	int PcNumBlocks;

	int *F;					// first row PI-prefixed by an item, undefined on =
	int TextLength;
	int SItemsNum;
	int PcdataNum;
	} xbwt_index_type;


// ------------------------------------------------------------
// Element of the catenating lists in the hash table. the field
// "code" contains the position of the token within the alphabet. 
// ------------------------------------------------------------
typedef struct Hash_node {
  char	*str;          
  int len_str;         // length of the token (to manage also NULL)
  int code;				
  int count_occ;		
  struct Hash_node *next;    
} Hash_node;

typedef Hash_node **Hash_nodeptr_array; //!< array of pointers to Hash_nodes

// ------------------------------------------------------------
// Hash table managed by catenating lists and MTF. 
// The field "size" indicates the table size, whereas the field
// "card" indicates the number of objects stored into the catenating lists. 
// ------------------------------------------------------------
typedef struct {
  int size;             
  int card;             // number of stored items
  Hash_nodeptr_array table;   
} HHash_table;

