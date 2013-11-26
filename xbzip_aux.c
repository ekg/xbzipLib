/***************************************************************************
 *   Copyright (C) 2005 by Paolo Ferragina, Università di Pisa             *
 *   Contact address: ferragina@di.unipi.it								   *
 *                                                                         *
 *   Description. Auxiliary functions for the library xbzip.h              *
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

#include "xbzip.h" 

extern int Verbose;

/* ****************************************************************** 
   comparison function used to sort the PI-components
   ****************************************************************** */
int PI_cmp(const void *a, const void *b)
{
	register int aa;
	register Tree_node *x = *( (Tree_node **) a);
	register Tree_node *y = *( (Tree_node **) b);
	
	// Comparison is done on the PI-component
	if (! (x->upward_path)) return -1; 
	if (! (y->upward_path)) return +1; 
	aa = strcmp(x->upward_path,y->upward_path);
	if (!aa) return (( x->position <= y->position ) ? -1 : +1); 
	return aa; 
}

/* ************************************************************************* 
   comparison function used to sort the TagAttr strings for their encoding
   ************************************************************************* */
int S_cmp(const void *a, const void *b)
{
	return strcmp( *((UChar **) a), *((UChar **) b) );
}


/* ****************************************************************** 
   A user-friendly printing function
   ****************************************************************** */
void print_pretty(char c)
{
  
  if(c>=32 && c<127)      // printable char
    printf("%c", c);
  else if(c=='\n')
    printf("\\n");        // \n
  else if(c=='\t')
    printf("\\t");        // \t
  else if(c=='\0')
    printf("\\0");        // \t
  else     
    printf("%02x", c);      // print hex code
}

/* ****************************************************************** 
   Another user-friendly printing function
   ****************************************************************** */
void print_pretty_len(char *s, int len)
{
  
for(;len>0; s++,len--) 
  print_pretty(*s);
}


/* ****************************************************************** 
   Printing the nodes array
   ****************************************************************** */
void print_nodes_array(Tree_node **x, int size)
{
	int i;
	Tree_node *u;
	
	printf("\n---------- NODES ARRAY ---------------\n");
	for(i=0; i<size; i++){
		printf("%d --LAST: %d  ---ALPHA: ", i, (x[i]->next_sibling==NULL) );
		print_pretty_len( x[i]->str, x[i]->len_str );
		printf("    ");
		u = x[i]->parent;
		printf("---PI: ");
		while (u) {
			print_pretty_len( u->str, u->len_str );
			printf(", ");
			u = u->parent;
			}
		printf("\n");
		}
	printf("\n\n");

}

/* ****************************************************************** 
   Printing the XBWT datatype
   ****************************************************************** */
void print_xbwt(xbwt_type *x)
{
	int i;

	printf("\n\n=========================================================================");
	printf("  \n================= XBW-Transform =========================================");
	printf("  \n=========================================================================\n");
	printf("\n\n----------------------- Slast -------------------\n");
	for(i=0;i < x->SItemsNum;i++){
		if (i%10 == 0) printf("\n[index  %3d: ] ",i); 
		printf("%d",x->Slast[i]);
	}
	printf("\n\n----------------------- Salpha (type 0=text/attrval, 1=tag/attr)-------------------\n");
	for(i=0;i < x->SItemsNum;i++){
		printf("\n[%d][type: %d] ",i,x->Stype[i]);
		print_pretty_len(x->Salpha[i],x->LenSalpha[i]);
		}

printf("\n\n=========================================================================");
	printf("  \n=========================================================================\n\n");

}

/* ****************************************************************** 
   Printing the INDEX datatype
   ****************************************************************** */
void print_index(xbwt_index_type *x)
{
	int i,j,pcdatalen;
	UChar *pcdata;

	printf("\n\n=========================================================================");
	printf("  \n===================== Index =============================================");
	printf("  \n=========================================================================\n");

	printf("Text Length = %d bytes\n",x->TextLength); 
	printf("Sarray length = %d\n",x->SItemsNum); 
	printf("Alphabet Length = %d bytes\n",x->AlphabetLen); 
	printf("Alphabet size = %d symbols\n",x->AlphabetCard); 

	printf("\n----------- Alphabet (sorted)\n"); 
	print_pretty_len(x->Alphabet,x->AlphabetLen);
	printf("\n");

	printf("\n----------- Last index information\n"); 
	printf("Last index length = %d bytes\n",x->LastIndexLen); 
	printf("Last Num blocks = %d (the last one is dummy)\n",x->LastNumBlocks); 

	for(i=0; i < x->LastNumBlocks; i++)
		printf("block #%d: offset %d pos in Last %d\n",i,x->LastOffsetBlocks[i],x->LastPosBlocks[i]);	

	printf("\n----------- Alpha index information\n"); 
	printf("Alpha index length = %d bytes\n",x->AlphaIndexLen); 
	printf("Alpha Num blocks = %d (the last one is dummy)\n",x->AlphaNumBlocks); 

	if (Verbose) {
		for(i=0; i < x->AlphaNumBlocks - 1; i++) // The ending block is dummy
			printf("block #%d: offset %d\n",i,x->AlphaOffsetBlocks[i]);	

		printf("\nPrefix counts....\n"); 
		for(i=0; i < x->AlphaNumBlocks - 1; i++){ // The ending block is dummy
			printf("\nblock #%d: ",i); 
			for(j=0; j < x->AlphabetCard; j++)
				printf(" %d",x->AlphaPrefixCounts[i*x->AlphabetCard+j]);  
			}
		}

	printf("\n\n----------- Pcdata index information\n"); 
	printf("Pcdata index length = %d bytes\n",x->PcdataIndexLen); 
	printf("Pcdata Num blocks = %d\n",x->PcNumBlocks); 

	if (Verbose) {
		for(i=0; i < x->PcNumBlocks; i++){
			printf("block #%d: offset %d\n",i,x->PcOffsetBlocks[i]);
			if (Verbose > 1) {
				bigbzip_decompress(x->PcdataIndex + x->PcOffsetBlocks[i], 
					x->PcOffsetBlocks[i+1] - x->PcOffsetBlocks[i], &pcdata, &pcdatalen);	
				print_pretty_len(pcdata, pcdatalen);
				printf("\n\n");
				}
			}
		}

	printf("\n----------- F information\n"); 
	for(i=0; i < x->AlphabetCard; i++)
		printf(" %d",x->F[i]);

	printf("\n\n");

}

//**************************************************************************
// Free the memory occupied by the DOM tree
//**************************************************************************
void free_tree(Tree_node *u)
{
	if(!u) {return;}
	free_tree(u->leftmost_child);
	free_tree(u->next_sibling);
	free(u);
}

//**************************************************************************
// Free the memory occupied by the DOM tree
//**************************************************************************
void print_tree(Tree_node *u)
{
	if(!u) {return;}
	print_pretty_len(u->str,u->len_str); printf("\n");
	print_tree(u->leftmost_child);
	print_tree(u->next_sibling);
}

//**************************************************************************
// integer logarithm
//**************************************************************************
int log2int(int u)    
{
  int i = 1;
  
  assert(u>0);
  while((i<32) && ( (u>>i) > 0) ) {
    i++;
	}

  assert(i<=32);
  return (i-1);
}

//**************************************************************************
// For time performance measurements
//**************************************************************************
double getTime ( void )
{
   double usertime,systime;
   struct rusage usage;

   getrusage ( RUSAGE_SELF, &usage );

   usertime = (double)usage.ru_utime.tv_sec +
     (double)usage.ru_utime.tv_usec / 1000000.0;

   systime = (double)usage.ru_stime.tv_sec +
     (double)usage.ru_stime.tv_usec / 1000000.0;

   return(usertime+systime);
}
