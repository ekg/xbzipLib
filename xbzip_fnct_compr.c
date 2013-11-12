/***************************************************************************
 *   Copyright (C) 2005 by Paolo Ferragina, Università di Pisa             *
 *   Contact address: ferragina@di.unipi.it								   *
 *                                                                         *
 *   Description. Main set of functions offered by the library xbzip.h     *
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



/* ***** LIBRARY FEATURES **********************************************
The library is designed to manage empty tags and external entities. 
The empty tags <t/> must be changed into <t></t> by xml_normalize.pl
The library manages also the external entities avoiding their resolving.

The data structure we create consists of three main components:
- Slast binary array of SItemsNum positions
- Salpha array of strings, being <TAGname or @ATTRname or = for Pcdata
- Pcdata string, where each Pcdata entry is prefixed by \0 

[PLAIN] We devise a serialization that consist of a Prologue and a blob string
containing the three arrays. The prologue contains some (6) integers referring
to the characteristics of the data, whereas the blob is obtained by catenating
the arrays Salpha and Pcdata. The Slast array is fused into Salpha by
coding the bit 1 as a dummy last child </ for each set of children. The plain
version is thus suitable to be compressed through any text compressor.

[BIGBZIP] we compress separately the Salpha+Slast, and Pcdata. 

[LAST] is a serialization in which we keep the three arrays distinct, and encode
the gaps between 1s in Slast via a Delta code (Elias). Salpha and Pcdata are
compressed with Bigbzip

[MTFMHUFF] compresses last and pcdata with bigbzip (as two pieces) and then uses
the series of compressors Mft+MultiHuff (which includes RLE) over Salpha.

******************************************************************** */


/* ------------- To manage xbzip and bigbzip ---------- */
#include "xbzip.h"
#include "bigbzip.h"

/* ------------- To print further infos for debugging ---------- */
extern int Verbose;


/* ----------------------------------------------------------------------------
	Procedure xbzip_compress()

	text: XML text to be compressed
	text_len: length of XML text
	ctext: (Reference to the) XBWT compressed data
	ctext_len: (Reference to the) length of XBWT compressed data
	flag: type of compression to be adopted (0=bigbzip,1=plain,2=last distinct)

	The space for the compressed text and its length is allocated here.
	---------------------------------------------------------------------------- */
void xbzip_compress(UChar text[], int text_len, 
					UChar *ctext[], int *ctext_len, UChar flag)
{
	double end_partial_timer, start_partial_timer, tot_partial_timer; // time usage
	xbwt_type xbwt;
	xbwt_string_type xbwtstr;

	printf("\n\n------- TIMINGS ----------\n");

	// Compute the XBWT
	printf("xbwt building\n");
	xbwt_builder(text, text_len, &xbwt);

	// Serialize the XBWT data into three strings and some infos
	// The strings are: Slast, Salpha, and the Pcdata
	__START_TIMER__;
	xbwt2xbwtstr(&xbwt, &xbwtstr);
	__END_TIMER__;
	printf("xbwt serialization %.4f seconds\n\n", tot_partial_timer);

	// Compress the serialized XBWT 
	__START_TIMER__;
	xbwtstr2compr(&xbwtstr,ctext,ctext_len, flag);
	__END_TIMER__;
	printf("xbwt compression %.4f seconds\n", tot_partial_timer);
	
	printf("\n\n--------------- XBWT INFOS ---------------\n\n");
	printf("Text of total length %d bytes\n\n",xbwt.TextLength);
	printf("XML tree consists of %d nodes and leaves\n\n", xbwt.SItemsNum);
	printf("TAG and ATTR names:\n");
	printf("\tnumber %15d\n",xbwt.TagAttrItemsTot);
	printf("\tdistinct %13d\n",xbwt.TagAttrItemsCard);
	printf("\tlength %15d bytes\n\n",xbwt.SalphaTotLen);
	printf("PCDATA entries:\n");
	printf("\tnumber %15d\n",xbwt.PcdataItems);
	printf("\tlength %15d bytes\n\n",xbwt.PcdataTotLen);
	printf("STRING lengths (uncompressed):\n");
	printf("\tPrologue: %12d bytes\n", 6*4);
	printf("\tSlast: %15d bytes\n", xbwtstr.lastLen);
	printf("\tSalpha: %14d bytes\n", xbwtstr.alphaLen);
	printf("\tPcdata: %14d bytes\n\n", xbwtstr.pcdataLen);
	printf("The total compressed size is of %d bytes\n\n", *ctext_len);

}


/* ----------------------------------------------------------------------
	Procedure xbzip_decompress()

	ctext: XBWT data to be decompressed
	ctext_len: length of XBWT data
	text: (Reference to the) uncompressed text
	text_len: (Reference to the) length of uncompressed text
	flag: type of compression adopted (0=bigbzip,1=plain,2=last distinct)

	The space for the output text and its length is allocated here.
	---------------------------------------------------------------------- */
void xbzip_decompress(UChar ctext[], int ctext_len, 
					  UChar *text[], int *text_len, UChar flag)
{
	double end_partial_timer, start_partial_timer, tot_partial_timer; // time usage
	xbwt_type xbwt;
	xbwt_string_type xbwtstr;

	printf("\n\n------- TIMINGS ----------\n");
	// Decompressing the serialized XBWT, it consists of three strings and some infos
	__START_TIMER__;
	compr2xbwtstr(ctext, ctext_len, &xbwtstr, flag);
	__END_TIMER__;
	printf("\nxbwt serialized decompress %.4f seconds\n", tot_partial_timer);

	// Deserialize the XBWT data		
	__START_TIMER__;
	xbwtstr2xbwt(&xbwtstr, &xbwt);
	__END_TIMER__;
	printf("\nxbwt deserialize %.4f seconds\n", tot_partial_timer);

	// Reconstruct the XML text
	printf("\nxbwt unbuilding\n");
	__START_TIMER__;
	xbwt_unbuilder(&xbwt, text, text_len);
	__END_TIMER__;

	printf("\n\n--------------- XBWT INFOS ---------------\n\n");
	printf("Text of total length %d bytes\n\n",xbwt.TextLength);
	printf("XML tree consists of %d nodes and leaves\n\n", xbwt.SItemsNum);
	printf("TAG and ATTR names:\n");
	printf("\tnumber %15d\n",xbwt.TagAttrItemsTot);
	printf("\tdistinct %13d\n",xbwt.TagAttrItemsCard);
	printf("\tlength %15d bytes\n\n",xbwt.SalphaTotLen);
	printf("PCDATA entries:\n");
	printf("\tnumber %15d\n",xbwt.PcdataItems);
	printf("\tlength %15d bytes\n\n",xbwt.PcdataTotLen);
	printf("STRING lengths (uncompressed):\n");
	printf("\tPrologue: %12d bytes\n", 6*4);
	printf("\tSlast: %15d bytes\n", xbwtstr.lastLen);
	printf("\tSalpha: %14d bytes\n", xbwtstr.alphaLen);
	printf("\tPcdata: %14d bytes\n\n", xbwtstr.pcdataLen);
	printf("The total compressed size is of %d bytes\n\n", ctext_len);
}


/* ----------------------------------------------------------------------------
	Building the XBW transform given the DOM tree of the XML document
	This procedure allocates the space for the XBWT datatype
	--------------------------------------------------------------------------- */
void xbwt_builder(UChar *text, int text_len, xbwt_type *xbwt)
{
	Tree_node *root;
	Tree_node **nodes_array;
	double end_partial_timer, start_partial_timer, tot_partial_timer; // time usage
	int i, cursor, TreeSize;
	HHash_table ht;


	__START_TIMER__;
	// Build the DOM tree for the XML document
	root = xml2tree(text, text_len, &TreeSize);
	xbwt->SItemsNum = TreeSize;
	xbwt->TextLength = text_len;

	//------------ stop measuring time
	__END_TIMER__;
	printf("  xml2tree %.4f seconds\n", tot_partial_timer);


	// Serializes the DOM tree into a DOM array
	__START_TIMER__;
	nodes_array = (Tree_node **) malloc(sizeof(Tree_node *) * xbwt->SItemsNum);
	if (!nodes_array) fatal_error("Error in allocating nodes_array! (xbwt builder)");
	cursor=0;
	tree2nodearray(root, nodes_array, &cursor);

	if(cursor < xbwt->SItemsNum)	fatal_error("Failing in serializing the XML tree!\n");
	__END_TIMER__;
	printf("  tree2array %.4f seconds\n", tot_partial_timer);
	
	// Sorts the DOM array according to the PI-component
	__START_TIMER__;
	qsort(nodes_array, xbwt->SItemsNum, sizeof(Tree_node *), PI_cmp);
	__END_TIMER__;
	printf("  qsort %.4f seconds\n", tot_partial_timer);

	if(Verbose)	
		print_nodes_array(nodes_array,cursor);

	// Memory Allocation 
	xbwt->Slast = (UChar *) malloc( sizeof(UChar) * xbwt->SItemsNum );
	if ( !xbwt->Slast ) fatal_error("Error in allocating space for Slast! (XBWT builder)\n");

	xbwt->Stype = (UChar *) malloc( sizeof(UChar) * xbwt->SItemsNum );
	if ( !xbwt->Stype ) fatal_error("Error in allocating space for Stype! (XBWT builder)\n");

	xbwt->Salpha = (UChar **) malloc( sizeof(UChar *) * xbwt->SItemsNum );
	xbwt->LenSalpha = (int *) malloc( sizeof(int) * xbwt->SItemsNum );
	if ( !xbwt->Salpha || !xbwt->LenSalpha) 
		fatal_error("Error in allocating space for Salpha or LenSalpha! (XBWT builder)\n");

	// Statistics and storage
	// 	xbwt->SItemsNum contains size of S array
	//  xbwt->TextLength is the text length

	__START_TIMER__;
	xbwt->SalphaTotLen=0;  // NOT counting the Pcdata 
	xbwt->PcdataTotLen=0;
	xbwt->PcdataItems=0;
	xbwt->TagAttrItemsTot=0;
	xbwt->TagAttrItemsCard=0;
	HHashtable_init(&ht, 2 * xbwt->SItemsNum);

	for(i=0; i < xbwt->SItemsNum; i++){ 
		
		xbwt->Slast[i] = (nodes_array[i]->next_sibling == NULL);
		xbwt->Stype[i] = nodes_array[i]->type;
		xbwt->Salpha[i] = nodes_array[i]->str;
		xbwt->LenSalpha[i] = nodes_array[i]->len_str;

		switch (nodes_array[i]->type) {

			// This is Pcdata or Attribute value
			case TEXT: 
				xbwt->PcdataItems++;
				xbwt->PcdataTotLen += xbwt->LenSalpha[i];
				break;
						
			// This is a tag or attribute name
			case TAGATTR: // Prefixed by @ or < 
				xbwt->TagAttrItemsTot++;
				xbwt->TagAttrItemsCard +=
					HHashtable_insert(nodes_array[i]->str, nodes_array[i]->len_str, 0, &ht);
				xbwt->SalphaTotLen += xbwt->LenSalpha[i];
				break;

			fatal_error("\nUnrecognized node type (XBWT formation)!\n");
		}
	}

	__END_TIMER__;
	printf("  xbwt data-type build %.4f seconds\n", tot_partial_timer);

}


/* ----------------------------------------------------------------------------
	Recovers the original XML document from the XBWT
	We assume that all fields of xbwt_type are initialized.
	This procedure allocates the space for the text and returns its length
	--------------------------------------------------------------------------- */
void xbwt_unbuilder(xbwt_type *xbwt, UChar *text[], int *text_len)
{
	char *strndup(const char *s, size_t n);
	double end_partial_timer, start_partial_timer, tot_partial_timer; // time usage
	int i, j, k, skip;
	int InAngleBrackets, cursor, top_stack, PosSalpha, starting_tag, *Stack;
	int *C, *F, *J; 
	UChar **S;
	HHash_table ht;
	Hash_node *hn;

	assert(xbwt->TagAttrItemsCard <= xbwt->TagAttrItemsTot);

	__START_TIMER__;

	// Lexicographic encode the TAG and ATTR names
	// TagAttrCard = # distinct TAG-ATTRS names (plain letters terminated by \0)
	S = (UChar **) malloc(sizeof(UChar *) * xbwt->TagAttrItemsCard);
	if (!S)	fatal_error("\nError in allocating the S array! (xbwt unbuilder)\n");

	HHashtable_init(&ht, 2 * xbwt->TagAttrItemsCard);
	for(i=0,k=0; i<xbwt->SItemsNum; i++){
		if(xbwt->Stype[i] != TEXT){
			if (HHashtable_insert(xbwt->Salpha[i], xbwt->LenSalpha[i], 0, &ht))
				S[k++] = strndup(xbwt->Salpha[i], xbwt->LenSalpha[i]);	
		}
	}

	assert(xbwt->TagAttrItemsCard == k);

	// Sort lexicoraphically the TAG and ATTR names
	qsort(S, xbwt->TagAttrItemsCard, sizeof(UChar *), S_cmp);

	// Compute the lexicographic code
	for(i=0; i<xbwt->TagAttrItemsCard; i++){
		hn = HHashtable_search(S[i], strlen(S[i]), &ht);
		hn -> code = i;
		}

	// BuildF 
	// F[i]=j iff the first PI-component prefixed by "i" is in row "j"
	C = (int *) malloc(sizeof(int) * xbwt->TagAttrItemsCard);
	F = (int *) malloc(sizeof(int) * xbwt->TagAttrItemsCard);
	
	for(i=0; i < xbwt->TagAttrItemsCard; i++) C[i]=0;
	for(i=0; i < xbwt->SItemsNum; i++)
		if(xbwt->Stype[i] != TEXT){
			hn=HHashtable_search(xbwt->Salpha[i], xbwt->LenSalpha[i], &ht);
			C[hn->code]++;  
			}

	// First PI-string is empty, first TAG-ATTR encoded with 0
	F[0]=1;						
	for(i=0; i < xbwt->TagAttrItemsCard - 1; i++){
		j=F[i];
		skip=0;
		while(skip != C[i]) // skip C[i] 1s in Slast
			if (xbwt->Slast[j++] == 1) skip++;
		F[i+1]=j; 
	}

	// BuildJ
	// J[i]=j iff Salpha[j] is the first child of Salpha[i]
	// J[i]=-1 iff Salpha[i] is a leaf
	J = (int *) malloc(sizeof(int) * xbwt->SItemsNum);
	for(i=0; i < xbwt->SItemsNum; i++) { 
		if (xbwt->Stype[i] == TEXT) 
			{ J[i] = -1; }
		else {
			hn=HHashtable_search(xbwt->Salpha[i], xbwt->LenSalpha[i], &ht); 
			J[i]=F[hn->code];
			j=J[i];
			while (xbwt->Slast[j] != 1) j++;
			F[hn->code] = j+1; // we jump to the next child and skip the 1
			}
		}

	__END_TIMER__;
	printf("  build arrays F and J %.4f seconds\n", tot_partial_timer);

	__START_TIMER__;

	// Rebuild the source document
	Stack = (int *) malloc(sizeof(int) * xbwt->SItemsNum);
	if (!Stack) fatal_error("Error in allocating Stack! (xbwt unbuilder)");

	*text_len = xbwt->TextLength; 
	*text = (UChar *) malloc(sizeof(UChar) * ((*text_len) + 2) );
	if( !(*text) ) fatal_error("\nError in allocating the text space! (xbwt_unbuilder)\n");
	(*text)[*text_len] = '\0'; // handling string end

	InAngleBrackets=0;	// flags if we are within <....>	
	cursor=0;			// moves over the text under construction
	top_stack=-1;		// points to the top of Stack
	starting_tag = 1;	// flag for managing <xml_xbwtroot>

	Stack[++top_stack] = 0; // Push the starting row

	for( ; top_stack > -1; ) {

		PosSalpha = Stack[top_stack--]; // Pop the top item

		// Managing the closing tag 
		// We set its PosAlpha to a negative value... a trick 
		if(PosSalpha < 0){
			if (InAngleBrackets != 0)
				fatal_error("InAngleBrackets is not 0 and PosAlpha is negative ! (xbwt_unbuilder)\n");
			(*text)[cursor++] = '<';
			(*text)[cursor++] = '/';
			// Cancel the <
			memcpy(*text+cursor,xbwt->Salpha[-PosSalpha]+1,xbwt->LenSalpha[-PosSalpha]-1);
			cursor += xbwt->LenSalpha[-PosSalpha]-1;			
			// Append the >
			(*text)[cursor++] = '>';
			continue; // back to Pop from Stack
		} 
		
		// We are within <.....> 
		if ( InAngleBrackets ) { 
			
			// We extracted something outside <....>
			// Hence, we need to manage the closing of >
			if ((xbwt->Stype[PosSalpha] == TEXT) || (xbwt->Salpha[PosSalpha][0] == '<')) {
				InAngleBrackets=0;
				Stack[++top_stack]=PosSalpha; // re-insert (push) into the stack
				if(!starting_tag) // not closing the dummy tag <xml_xbwtroot>
					(*text)[cursor++] = '>';
				starting_tag=0; // No longer meet the dummy tag
				continue; // back to Pop from Stack
			} 
			
			// We extracted some attribute inside <....>
			assert( (xbwt->Stype[PosSalpha] == TAGATTR) && (xbwt->Salpha[PosSalpha][0] == '@') ); 

			(*text)[cursor++] = ' ';
			memcpy(*text+cursor,xbwt->Salpha[PosSalpha]+1,xbwt->LenSalpha[PosSalpha]-1); // avoid @
			cursor += xbwt->LenSalpha[PosSalpha]-1;

			// Create the attribute string
			(*text)[cursor++] = '='; (*text)[cursor++] = '\"';
			assert(xbwt->Stype[J[PosSalpha]] == TEXT);
			memcpy(*text+cursor,xbwt->Salpha[J[PosSalpha]],xbwt->LenSalpha[J[PosSalpha]]);
			cursor += xbwt->LenSalpha[J[PosSalpha]];
			(*text)[cursor++] = '"';

			// No pushing in the stack since we completed the attribute
			continue; // back to pop from Stack
		}


		// We are outside <....>
		assert((xbwt->Stype[PosSalpha] == TEXT) || (xbwt->Salpha[PosSalpha][0] == '<'));

		// Manage the texts
		if (xbwt->Stype[PosSalpha] == TEXT) { 
			if (xbwt->Salpha[PosSalpha][0] != (UChar) 255) // not dummy filler for empty tag
				{
				memcpy(*text+cursor,xbwt->Salpha[PosSalpha],xbwt->LenSalpha[PosSalpha]);
				cursor += xbwt->LenSalpha[PosSalpha];
				}
			continue; // back to pop from Stack
		}


		// Manage the tags, mark that we are in a tag
		if (xbwt->Stype[PosSalpha] == TAGATTR) { 
			InAngleBrackets=1; 
			if(!starting_tag) // write if not the dummy root
			{
				memcpy(*text+cursor,xbwt->Salpha[PosSalpha],xbwt->LenSalpha[PosSalpha]);
				cursor += xbwt->LenSalpha[PosSalpha];
			} 

		// Insert in the stack the children of the current node
		// J[]>0 ensures the node is not a leaf
		if (J[PosSalpha] > 0) { // PosSalpha > 0, we managed </...> above 
			k=J[PosSalpha];
			while(xbwt->Slast[k]==0) k++;
			if (PosSalpha != 0)		// not re-inserting the dummy root 
				Stack[++top_stack] = -PosSalpha; // Push negative Pos as closing tag: trick
			while(k>=J[PosSalpha])  // Insert in reverse order, since we use a stack
				Stack[++top_stack] = k--;
			}
		}
	}
	__END_TIMER__;
	printf("  reconstruct the text %.4f seconds\n", tot_partial_timer);

	// To avoid some spurious chars after the last tag
	*text_len = cursor;
	 
	free(F); free(J); free(S); free(C);
}


/* ----------------------------------------------------------------------------
	Serializes the XBWT data type into xbwt_string_type (look at xbzip_type.h)
	Salpha consists of <tag, @attr, = (for Pcdata)
	Pcdata (text and attr value) is ordered according to Salpha, prefixed by \0
	--------------------------------------------------------------------------- */

void xbwt2xbwtstr(xbwt_type *xbwt, xbwt_string_type *xbwtstr)
{
	int i, alphaOff, PcdataOff;


	xbwtstr->TextLength=xbwt->TextLength;		// Reserved to store the TextLength
	xbwtstr->SItemsNum=xbwt->SItemsNum;			// Reserved to store the SItemsNum
	xbwtstr->TagAttrItemsCard=xbwt->TagAttrItemsCard;	// Reserved to store TagAttrItemsCard (costly to be derived)
	xbwtstr->PcdataItems=xbwt->PcdataItems;		// Reserved to store # of Pcdata items (costly to be derived)


	// Serializing (copying) Slast
	xbwtstr->lastLen=xbwt->SItemsNum;
	xbwtstr->lastStr=xbwt->Slast;

	// Serializing Salpha: 1 byte is reserved for TEXT entries, marked with = 
	xbwtstr->alphaLen = xbwt->SalphaTotLen + xbwt->PcdataItems;
	xbwtstr->alphaStr = (UChar *) malloc((xbwtstr->alphaLen) * sizeof(UChar));
	if (! (xbwtstr->alphaStr) ) fatal_error("Failed allocating the SALPHA array! (XBWT2STR)\n");

	// Serializing Pcdata: 1 byte is reserved for the prefix \0 
	xbwtstr->pcdataLen = xbwt->PcdataTotLen + xbwt->PcdataItems;
	xbwtstr->pcdataStr = (UChar *) malloc((xbwtstr->pcdataLen) * sizeof(UChar));
	if (! (xbwtstr->pcdataStr) ) fatal_error("Failed allocating the SALPHA array! (XBWT2STR)\n");

	alphaOff=0; PcdataOff=0;
	for (i=0; i<xbwt->SItemsNum; i++) {

		// We need \0 to prefix each PcdataItem
		// We put = to mark the position of an attrval or text in Salpha
		// We have @ in front of an attribute name
		// We have < in front of an attribute name
		switch (xbwt->Stype[i]) {

			// This is Pcdata or Attribute value
			case TEXT: 
				xbwtstr->alphaStr[alphaOff++] = '='; // Salpha marked with =
				xbwtstr->pcdataStr[PcdataOff++]= (UChar) TEXT; // Pcdata prefixed by \0
				memcpy(xbwtstr->pcdataStr+PcdataOff, xbwt->Salpha[i], xbwt->LenSalpha[i]);
				PcdataOff += xbwt->LenSalpha[i];
				break;
						
			// This is a tag or attribute name
			case TAGATTR: // Prefixed by @ or < 
				memcpy(xbwtstr->alphaStr+alphaOff, xbwt->Salpha[i], xbwt->LenSalpha[i]);
				alphaOff += xbwt->LenSalpha[i];
				break;

			fatal_error("\nUnrecognized node type (XBWT2STR)!\n");
		}
	}
}


/* ----------------------------------------------------------------------------
	Deserializes the XBWT_STRING data type into the XBWT data type 
	Salpha consists of <tag, @attr, = (for Pcdata)
	Pcdata (text and attr value) is ordered according to Salpha, prefixed by \0
	This procedure allocates the space for the XBWT data type
	--------------------------------------------------------------------------- */
void xbwtstr2xbwt(xbwt_string_type *xbwtstr, xbwt_type *xbwt)
{
	int i,alphaOff,pcdataOff;

	// Set the common fields
	xbwt->TextLength = xbwtstr->TextLength;
	xbwt->SItemsNum = xbwtstr->SItemsNum;
	xbwt->TagAttrItemsCard = xbwtstr->TagAttrItemsCard;
	xbwt->PcdataItems = 0;
	xbwt->PcdataTotLen = 0;
	xbwt->SalphaTotLen = 0; 
	xbwt->TagAttrItemsTot = 0;

	// Load Slast
	xbwt->Slast = xbwtstr->lastStr;

	// Load Stype, Salpha, and LenSalpha
	xbwt->Stype = (UChar *) malloc((xbwt->SItemsNum) * sizeof(UChar));
	if (! (xbwt->Stype) ) fatal_error("Failed allocating the STYPE array! (STR2XBWT)\n");

	xbwt->Salpha = (UChar **) malloc((xbwt->SItemsNum) * sizeof(UChar *));
	if (! (xbwt->Salpha) ) fatal_error("Failed allocating the SALPHA array! (STR2XBWT)\n");

	xbwt->LenSalpha = (int *) malloc((xbwt->SItemsNum) * sizeof(int));
	if (! (xbwt->LenSalpha) ) fatal_error("Failed allocating the LENSALPHA array! (STR2XBWT)\n");

	alphaOff=0; pcdataOff=0;
	for (i=0; i < xbwt->SItemsNum; i++) {

		if ( xbwtstr->alphaStr[alphaOff] != '=' ) // This is a TAG-ATTR name
			{ 
			  xbwt->TagAttrItemsTot++;
			  xbwt->Stype[i] = TAGATTR;
			  xbwt->Salpha[i] = xbwtstr->alphaStr + alphaOff;
			  // skip the starting char < or @
			  xbwt->LenSalpha[i]=1; alphaOff++; 
			  // Search for the end of the tag-attr name
			  while ( (xbwtstr->alphaStr[alphaOff] != '=') &&
					  (xbwtstr->alphaStr[alphaOff] != '<')  &&
					  (xbwtstr->alphaStr[alphaOff] != '@')  &&
					  (alphaOff < xbwtstr->alphaLen)
					  ) { 
						  alphaOff++; 
						  xbwt->LenSalpha[i]++; 
						}
		  	  xbwt->SalphaTotLen += xbwt->LenSalpha[i]; 
			} 
		else // This is a TEXT field (Pcdata is prefixed by \0)
			{ 
   			  alphaOff++; // skip the mark = on Salpha
			  xbwt->PcdataItems++;
			  xbwt->Stype[i] = TEXT;
			  pcdataOff++; // skip the prefix \0
			  xbwt->Salpha[i] = xbwtstr->pcdataStr+pcdataOff; 
			  xbwt->LenSalpha[i]=0;
  			  // Search for the end of the pcdata
			  while ( (xbwtstr->pcdataStr[pcdataOff] != '\0') && 
				       (pcdataOff < xbwtstr->pcdataLen) 
					) { 
						  pcdataOff++; 
						  xbwt->LenSalpha[i]++; 
  					  }
		  	  xbwt->PcdataTotLen += xbwt->LenSalpha[i]; 
			}

	}
}

/* ----------------------------------------------------------------------------
	Fuse Salpha with Slast: </ ends any group of children, = stands for Pcdata 
	--------------------------------------------------------------------------- */
void fuse_alpha_last(xbwt_string_type *xbwtstr, UChar *fused[], int *fused_len)
{
	UChar *stemp;
	int stemp_len, i, j, k;

	// Oversize in case of short texts which do expand !
	stemp_len = max(xbwtstr->TextLength + 4*xbwtstr->lastLen, 100000); 
	stemp = (UChar *) malloc(sizeof(UChar) * (stemp_len) );
	if( !(stemp) ) fatal_error("\nError in allocating the STEMP string! (FUSE)\n");

	for(i=0,j=0,k=0; (j < xbwtstr->lastLen) && (k < xbwtstr->alphaLen); k++ ){

		// For each node verify if the previous is the last children
		// In case, it appends </ to the previous one 
		if ((xbwtstr->alphaStr[k] == '@') ||
			(xbwtstr->alphaStr[k] == '<') ||
			(xbwtstr->alphaStr[k] == '=')) {

				// Mark the last children, prefixing a </
				if( (j>0) && (xbwtstr->lastStr[j-1] == 1)) { 
					stemp[i++] = '<'; 
					stemp[i++] = '/';
				}

				j++;
			}

		stemp[i++] = xbwtstr->alphaStr[k];
	}

	if( xbwtstr->lastStr[xbwtstr->lastLen-1] == 1) 
		{ stemp[i++] = '<'; stemp[i++] = '/';}

	if( i > stemp_len )
		fatal_error("Out of bounds for i! (FUSE)\n");
	
	realloc(stemp,i);
	*fused = stemp; *fused_len = i;
}

/* ----------------------------------------------------------------------------
	Unfuse Salpha from Slast: </ ends any group of children, = stands for Pcdata 
	--------------------------------------------------------------------------- */
void unfuse_alpha_last(UChar *fused, int fused_len, 
					   UChar *alpha[], UChar *last[], 
					   int *alphalen, int *lastlen)
{
	int i,j,k;	

	// Loading Slast and Salpha, oversized
	*alpha  = (UChar *) malloc(sizeof(UChar) * fused_len );
	if( !(*alpha) ) fatal_error("\nError in allocating the SLAST string! (UNFUSE)\n");

	*last  = (UChar *) malloc(sizeof(UChar) * fused_len );
	if( !(*last) ) fatal_error("\nError in allocating the SALPHA string! (UNFUSE)\n");

	// The Pcdata string starts with a \0, hence stop there in stemp
	// j moves over Slast, k moves over Salpha, i moves over stemp
	for(i=0, j=0, k=0; i<fused_len; ){

		// For each TAG-ATTR verify if it is the first one
		// In case, it marks the previous
		if ((fused[i] == '@') ||
			(fused[i] == '<') ||
			(fused[i] == '=')) {
				
				if (fused[i+1] == '/') { (*last)[j-1] = 1; i += 2;}
				else { (*last)[j++] = 0; (*alpha)[k++] = fused[i]; i += 1; }

			} else { (*alpha)[k++] = fused[i]; i += 1;}

		}

	if( (k >fused_len) || (j > fused_len) )
		fatal_error("Error in Defusing Salpha and Slast! (UNFUSE)\n");

	*alphalen=k; realloc(*alpha,*alphalen);
	*lastlen=j; realloc(*last,*lastlen);

}

/* ----------------------------------------------------------------------------
	Compresses the XBWT_STRING data type 
		This procedure allocates the space for the compressed text
		The compressed string is generated by catenating:
		- Prologue: 6 numbers of 4 bytes each
		- Salpha fused with Slast: </ ends any group of children, = stands for Pcdata 
		- Pcdata: each entry is prefixed by \0
	
	The semantic of the numbers depends on the type of chosen compression
	--------------------------------------------------------------------------- */
void xbwtstr2compr(xbwt_string_type *xbwtstr, UChar *ctext[], int *ctext_len, UChar flag)
{
	UChar *fused, *cfused, *cpc, *calpha, *cpcdata, *clast, *Ualpha, *mtfc;
	int i, j, fused_len, cfused_len, cpc_len, gap, gaplen, loggaplen;
	int nbits_min, clastlen, calphalen, cpcdatalen;
	int k,pos,code,AlfLen,rest,startb;
	HHash_table ht;
	Hash_node *hn;


	// Oversize in case of short texts which may expand !
	*ctext_len = max(xbwtstr->TextLength + 5*xbwtstr->lastLen, 100000); 
	*ctext = (UChar *) malloc(sizeof(UChar) * (*ctext_len) );
	if( !(*ctext) ) fatal_error("\nError in allocating the CTEXT string! (STR2COMPR)\n");

	switch (flag) {

		case PLAIN:

			// Fuse the two arrays into one: </ per group of children
			fuse_alpha_last(xbwtstr, &fused, &fused_len);
	
			// Prologue
			init_buffer(*ctext,50);
			bbz_bit_write(32,xbwtstr->TextLength);
			bbz_bit_write(32,xbwtstr->SItemsNum);
			bbz_bit_write(32,xbwtstr->TagAttrItemsCard);
			bbz_bit_write(32,xbwtstr->PcdataItems);
			bbz_bit_write(32,xbwtstr->lastLen);		// Store the (un)compressed length 
			bbz_bit_write(32,fused_len);			// Store the FUSED length 
			bbz_bit_write(32,xbwtstr->pcdataLen);	// Store the (un)compressed length
			i = 7 * 4; // 7 numbers on 4 bytes each

			// Write fused 
			memcpy((*ctext)+i, fused, fused_len);
			i += fused_len;

			// Write Pcdata (each Pcdata item starts with \0)
			memcpy((*ctext) + i, xbwtstr->pcdataStr, xbwtstr->pcdataLen);
			i += xbwtstr->pcdataLen;

			if(i > (*ctext_len))
				fatal_error("Overflow in writing the PLAIN file! (STR2COMPR)\n");

			*ctext_len = i;
			realloc(*ctext, *ctext_len);
			break;

		case BIGBZIP: // Fuse Last and Salpha

			// Fuse and compress 
			fuse_alpha_last(xbwtstr, &fused, &fused_len);
			data_compress(fused, fused_len, &cfused, &cfused_len);
			data_compress(xbwtstr->pcdataStr, xbwtstr->pcdataLen, &cpc, &cpc_len);

			// Prologue
			init_buffer(*ctext,50);
			bbz_bit_write(32,xbwtstr->TextLength);
			bbz_bit_write(32,xbwtstr->SItemsNum);
			bbz_bit_write(32,xbwtstr->TagAttrItemsCard);
			bbz_bit_write(32,xbwtstr->PcdataItems);
			bbz_bit_write(32,xbwtstr->lastLen);		// Store the uncompressed length (unused)
			bbz_bit_write(32,cfused_len);			// Store the compressed length of FUSED
			bbz_bit_write(32,cpc_len);				// Store the compressed length of PCDATA
			
			i = 7 * 4; // 7 numbers on 4 bytes each in the prologue
			memcpy((*ctext) + i, cfused, cfused_len); // Copy the compressed FUSED
			i += cfused_len;
			memcpy((*ctext) + i, cpc, cpc_len); // Copy the compressed PCDATA
			i += cpc_len;
			if(i > (*ctext_len)) 
				fatal_error("Overflow in writing the BIGBZIP file! (STR2COMPR)\n");

			*ctext_len = i;
			realloc(*ctext, *ctext_len);

			printf("\n\nCompression ratio over single pieces:\n");
			printf("  Salpha+Last bigbzip-compressed = %8d bytes\n",cfused_len);
			printf("  Pcdata bigbzip-compressed      = %8d bytes\n\n",cpc_len);
			break;

		case LAST: // Last is compressed alone

			// Prologue with dummy values for 6 integers on 4 bytes
			init_buffer(*ctext, 7 * 4 + 10 * xbwtstr->lastLen);
			for (i=0; i < 7; i++)
				bbz_bit_write(32,0);

			// Encoding Slast by DELTA-code
			nbits_min = 0;
			for(j=-1; j<xbwtstr->lastLen-1; ){
			for(gap=1; (j+gap < xbwtstr->lastLen) && (xbwtstr->lastStr[j+gap] == 0); gap++) ;
				gaplen = log2int(gap)+1;
				loggaplen = log2int(gaplen)+1;
				if(loggaplen > 1)
					bbz_bit_write(loggaplen-1,0);
				bbz_bit_write(loggaplen,gaplen);
				bbz_bit_write(gaplen,gap);
				j += gap;
				nbits_min += gaplen;
			}
			bbz_bit_flush();

			i = 7 * 4;
			clastlen = get_buffer_fill() - i;
			i += clastlen;

			// Encoding Salpha
			data_compress(xbwtstr->alphaStr, xbwtstr->alphaLen, &calpha, &calphalen);	
			memcpy(*ctext + i, calpha, calphalen);
			i += calphalen;

			// Encoding Pcdata
			data_compress(xbwtstr->pcdataStr, xbwtstr->pcdataLen, &cpcdata, &cpcdatalen);
			memcpy(*ctext + i, cpcdata, cpcdatalen);
			i += cpcdatalen;

			// Write Prologue with correct values
			init_buffer(*ctext,50);
			bbz_bit_write(32,xbwtstr->TextLength);
			bbz_bit_write(32,xbwtstr->SItemsNum);
			bbz_bit_write(32,xbwtstr->TagAttrItemsCard);
			bbz_bit_write(32,xbwtstr->PcdataItems);
			bbz_bit_write(32,clastlen);		
			bbz_bit_write(32,calphalen);	
			bbz_bit_write(32,cpcdatalen);	


			*ctext_len = i;
			realloc(*ctext, *ctext_len);

			printf("\n\nCompression ratio over single pieces:\n");
			printf("  Last   delta-compressed   = %8d bytes\n",clastlen);
			printf("  Salpha bigbzip-compressed = %8d bytes\n",calphalen);
			printf("  Pcdata bigbzip-compressed = %8d bytes\n\n",cpcdatalen);
			break;

		case MTFMHUFF:  // We use MTF+MULTIHUFF (has RLE inside) over Salpha

			if (xbwtstr->TagAttrItemsCard > 256)
				fatal_error("Current version does not support #tag-attrs > 256. Sorry!");

			// Prologue with dummy values for 6 integers on 4 bytes
			init_buffer(*ctext, 7 * 4 + 10 * xbwtstr->lastLen);
			for (i=0; i < 7; i++)
				bbz_bit_write(32,0);
			i = 7 * 4;

			data_compress(xbwtstr->lastStr, xbwtstr->lastLen, &clast, &clastlen);
			memcpy(*ctext + i, clast, clastlen);
			i += clastlen;

			// Encoding Salpha
			Ualpha  = (UChar *) malloc(sizeof(UChar) * xbwtstr->SItemsNum );
			if( !Ualpha ) fatal_error("\nError in creating Ualpha! (xbwt2compr)\n");
			mtfc  = (UChar *) malloc(sizeof(UChar) * xbwtstr->SItemsNum );
			if( !mtfc ) fatal_error("\nError in creating mtfc! (xbwt2compr)\n");
			calpha  = (UChar *) malloc(sizeof(UChar) * xbwtstr->alphaLen * 2);
			if( !calpha ) fatal_error("\nError in creating Ualpha! (xbwt2compr)\n");

			// Encode Salpha's symbols with Uchars
			// k moves over Salpha, pos moves over Ualpha
			// AlfLen moves over the calpha and limits the alphabet symbols
			HHashtable_init(&ht, 2 * (xbwtstr->TagAttrItemsCard + 1) );
			for(k=0, code=0, pos=0, AlfLen=0; k < xbwtstr->alphaLen; ){
				startb=k;
				k++;
				while ( (k < xbwtstr->alphaLen) &&
						(xbwtstr->alphaStr[k] != '@') &&
						(xbwtstr->alphaStr[k] != '<') &&
						(xbwtstr->alphaStr[k] != '=')) {
							k++;
						}
				hn = HHashtable_search(xbwtstr->alphaStr + startb, k - startb, &ht);
				if(!hn){
					HHashtable_insert(xbwtstr->alphaStr + startb, k - startb, code++, &ht);
					hn = HHashtable_search(xbwtstr->alphaStr + startb, k - startb, &ht);
					memcpy(calpha+AlfLen,xbwtstr->alphaStr + startb, k - startb);
					AlfLen += k - startb;
					} 

				Ualpha[pos++]= (UChar) hn->code; //Salpha mapped to integer codes			
				}
			calpha[AlfLen++] = '>';
			if(pos != xbwtstr->SItemsNum)
				fatal_error("Wrong count of Salpha size! (xbwt2compr)\n");

			mtf(Ualpha, mtfc, xbwtstr->SItemsNum);
			rest = xbwtstr->alphaLen * 2 - AlfLen;
			multihuf_compr(mtfc,xbwtstr->SItemsNum, calpha+AlfLen, &rest);

			calphalen = rest+AlfLen;
			memcpy(*ctext + i, calpha, calphalen);
			i += calphalen;

			// Encoding Pcdata
			data_compress(xbwtstr->pcdataStr, xbwtstr->pcdataLen, &cpcdata, &cpcdatalen);
			memcpy(*ctext + i, cpcdata, cpcdatalen);
			i += cpcdatalen;

			// Write Prologue with correct values
			init_buffer(*ctext,50);
			bbz_bit_write(32,xbwtstr->TextLength);
			bbz_bit_write(32,xbwtstr->SItemsNum);
			bbz_bit_write(32,xbwtstr->TagAttrItemsCard);
			bbz_bit_write(32,xbwtstr->PcdataItems);
			bbz_bit_write(32,clastlen);		
			bbz_bit_write(32,calphalen);	
			bbz_bit_write(32,cpcdatalen);	


			*ctext_len = i;
			realloc(*ctext, *ctext_len);

			printf("\n\nCompression ratio over single pieces:\n");
			printf("  Last bigbzip-compressed     = %8d bytes\n",clastlen);
			printf("  Salpha MultiHuff-compressed = %8d bytes\n",calphalen);
			printf("  Pcdata bigbzip-compressed   = %8d bytes\n\n",cpcdatalen);
			break;

		case DISTINCT: // Last is compressed alone

			// Prologue with dummy values for 7 integers on 4 bytes
			init_buffer(*ctext, 7 * 4 + 10 * xbwtstr->lastLen);
			for (i=0; i < 7; i++)
				bbz_bit_write(32,0);

			i = 7*4;

			data_compress(xbwtstr->lastStr, xbwtstr->lastLen, &clast, &clastlen);	
			memcpy(*ctext + i, clast, clastlen);
			i += clastlen;

			// Encoding Salpha
			data_compress(xbwtstr->alphaStr, xbwtstr->alphaLen, &calpha, &calphalen);	
			memcpy(*ctext + i, calpha, calphalen);
			i += calphalen;

			// Encoding Pcdata
			data_compress(xbwtstr->pcdataStr, xbwtstr->pcdataLen, &cpcdata, &cpcdatalen);
			memcpy(*ctext + i, cpcdata, cpcdatalen);
			i += cpcdatalen;

			// Write Prologue with correct values
			init_buffer(*ctext,50);
			bbz_bit_write(32,xbwtstr->TextLength);
			bbz_bit_write(32,xbwtstr->SItemsNum);
			bbz_bit_write(32,xbwtstr->TagAttrItemsCard);
			bbz_bit_write(32,xbwtstr->PcdataItems);
			bbz_bit_write(32,clastlen);		
			bbz_bit_write(32,calphalen);	
			bbz_bit_write(32,cpcdatalen);	


			*ctext_len = i;
			realloc(*ctext, *ctext_len);

			printf("\n\nCompression ratio over single pieces:\n");
			printf("  Last bigbzip-compressed   = %8d bytes\n",clastlen);
			printf("  Salpha bigbzip-compressed = %8d bytes\n",calphalen);
			printf("  Pcdata bigbzip-compressed = %8d bytes\n\n",cpcdatalen);
			break;
	}

}

/* ----------------------------------------------------------------------------
	Decompresses the XBWT_STRING data type 
		This procedure allocates the space for the XBWT_STRING data type
	The semantic of the numbers depends on the type of chosen compression
	--------------------------------------------------------------------------- */
void compr2xbwtstr(UChar ctext[], int ctext_len, xbwt_string_type *xbwtstr, UChar flag)
{
	char *strndup(const char *s, size_t n);
	UChar *fused, *calpha, *Ualpha, *mtfc, **S;
	int i, fused_len, cfused_len, cpc_len, mtfc_len;
	int clastlen, calphalen, cpcdatalen, loggaplen, gaplen, gap;
	int k, startb, code, rest;


	switch (flag) {

		case PLAIN:

			// Prologue
			init_buffer(ctext,50);
			xbwtstr->TextLength			= bbz_bit_read(32);
			xbwtstr->SItemsNum			= bbz_bit_read(32);
			xbwtstr->TagAttrItemsCard	= bbz_bit_read(32);
			xbwtstr->PcdataItems		= bbz_bit_read(32);
			xbwtstr->lastLen			= bbz_bit_read(32);		 
			fused_len					= bbz_bit_read(32); // length of fused Salpha and Slast
			xbwtstr->pcdataLen			= bbz_bit_read(32);	// correct uncompressed length	 	
			i = 7 * 4; // 7 numbers on 4 bytes each
			
			// Allocate the space for the output arrays
			unfuse_alpha_last(ctext+i, fused_len, 
							&(xbwtstr->alphaStr), &(xbwtstr->lastStr), 
							&(xbwtstr->alphaLen), &(xbwtstr->lastLen));

			// Pcdata is just mapped
			i += fused_len; 
			xbwtstr->pcdataStr  = ctext + i;

			// Just for correctness check
			i += xbwtstr->pcdataLen;
			if (i != ctext_len)
				fatal_error("Error in decompression - not whole scan! (COMPR2STR)\n");			
			break;


		case BIGBZIP:

			init_buffer(ctext,50);
			xbwtstr->TextLength			= bbz_bit_read(32);
			xbwtstr->SItemsNum			= bbz_bit_read(32);
			xbwtstr->TagAttrItemsCard	= bbz_bit_read(32);
			xbwtstr->PcdataItems		= bbz_bit_read(32);
			xbwtstr->lastLen			= bbz_bit_read(32); // UNcompressed length		 
			cfused_len					= bbz_bit_read(32); // compressed length of fused	 
			cpc_len						= bbz_bit_read(32);	// compressed length of Pcdata	 	

			i = 7 * 4;							// 7 numbers on 4 bytes each

			// Decompress the fused arrays
			data_decompress(ctext+i, cfused_len, &fused, &fused_len);

			// Unfuse the two arrays Salpha and Slast
			unfuse_alpha_last(fused, fused_len, 
							&(xbwtstr->alphaStr), &(xbwtstr->lastStr), 
							&(xbwtstr->alphaLen), &(xbwtstr->lastLen));

			// Decompress Pcdata
			i += cfused_len; 
			data_decompress(ctext+i, cpc_len, &(xbwtstr->pcdataStr), &(xbwtstr->pcdataLen));
			break;

		case LAST: // Last is compressed alone

			// Read the Prologue 
			init_buffer(ctext,50);
			xbwtstr->TextLength			= bbz_bit_read(32);
			xbwtstr->SItemsNum			= bbz_bit_read(32);
			xbwtstr->TagAttrItemsCard	= bbz_bit_read(32);
			xbwtstr->PcdataItems		= bbz_bit_read(32);
			clastlen					= bbz_bit_read(32);		
			calphalen					= bbz_bit_read(32);	
			cpcdatalen					= bbz_bit_read(32);	

			// Decoding Slast by DELTA-code
			xbwtstr->lastLen = xbwtstr->SItemsNum;
			xbwtstr->lastStr = (UChar *) malloc(sizeof(UChar) * xbwtstr->lastLen);
			if( !xbwtstr->lastStr ) 
				fatal_error("\nError in allocating the Slast array! (COMPR2STR)\n");

			for(i=0; i<xbwtstr->lastLen; ){
				for(loggaplen=1; bbz_bit_read(1) == 0; loggaplen++) ;
				gaplen = (1<< (loggaplen - 1));
				if (loggaplen>1)
					gaplen += bbz_bit_read(loggaplen-1); 
				gap = bbz_bit_read(gaplen);
				for(; gap>1; gap--) xbwtstr->lastStr[i++]=0;
				xbwtstr->lastStr[i++]=1;
			} 			
			bbz_byte_align();

			i = get_buffer_fill();

			if (7 * 4 + clastlen != i)
				fatal_error("Error in reading the compressed last! (COMPR2STR)\n");

			data_decompress(ctext + i, calphalen, &(xbwtstr->alphaStr), &(xbwtstr->alphaLen));
			i += calphalen;
			
			data_decompress(ctext + i, cpcdatalen, &(xbwtstr->pcdataStr), &(xbwtstr->pcdataLen));
			i += cpcdatalen;

			if(i != ctext_len)
				fatal_error("Error in decompressing! (COMPR2STR)\n");
			break;


		case MTFMHUFF:

			// Read the Prologue 
			init_buffer(ctext,50);
			xbwtstr->TextLength			= bbz_bit_read(32);
			xbwtstr->SItemsNum			= bbz_bit_read(32);
			xbwtstr->TagAttrItemsCard	= bbz_bit_read(32);
			xbwtstr->PcdataItems		= bbz_bit_read(32);
			clastlen					= bbz_bit_read(32);		
			calphalen					= bbz_bit_read(32);	
			cpcdatalen					= bbz_bit_read(32);	

			// Decompressing Slast
			xbwtstr->lastLen = xbwtstr->SItemsNum;
			i = 7 * 4;
			data_decompress(ctext + i, clastlen, &(xbwtstr->lastStr), &(xbwtstr->lastLen));
			i += clastlen;

			// Decompressing Salpha
			Ualpha  = (UChar *) malloc(sizeof(UChar) * xbwtstr->SItemsNum );
			if( !Ualpha ) fatal_error("\nError in creating Ualpha! (compr2xbwt)\n");
			mtfc  = (UChar *) malloc(sizeof(UChar) * xbwtstr->SItemsNum );
			if( !mtfc ) fatal_error("\nError in creating mtfc! (compr2xbwt)\n");
			S  = (UChar **) malloc(sizeof(UChar *) * (xbwtstr->TagAttrItemsCard + 1));
			if( !S ) fatal_error("\nError in creating S! (compr2xbwt)\n");

			// Create table for Salpha's symbol <--> codes
			calpha = ctext + i;
			for(k=0, startb=0, code=0; (k < calphalen) && (calpha[k] != '>'); ){
				startb = k;
				k++;
				while ( (k < calphalen) &&
						(calpha[k] != '@') &&
						(calpha[k] != '<') &&
						(calpha[k] != '>') &&
						(calpha[k] != '=')) {
							k++;
						}
				S[code++] = strndup(calpha+startb, k-startb);
				}
			
			// Error check
			if(calpha[k] != '>') // Alphabet not ended by the special symbol >
				fatal_error("Compressed Salpha format wrong! (compr2xbwt)\n");
			if(calphalen < k)    
				fatal_error("Scanned for alphabet out-of-bound! (compr2xbwt)\n");
			if(code > xbwtstr->TagAttrItemsCard+1)
				fatal_error("Error in counting alphabet size! (compr2xbwt)\n");

			// Decompressing the rest of Salpha
			k++; // skip the >
			rest = calphalen - k;
			mtfc_len = xbwtstr->SItemsNum * 2;
			multihuf_decompr(calpha+k, rest, mtfc, &mtfc_len);
			if (mtfc_len != xbwtstr->SItemsNum)
				fatal_error("Error in reconstructing Ualpha! (compr2xbwt)\n");

			unmtf(mtfc, Ualpha, xbwtstr->SItemsNum);

			// oversized
			xbwtstr->alphaStr  = (UChar *) malloc(sizeof(UChar) * xbwtstr->TextLength );
			if( !xbwtstr->alphaStr ) fatal_error("\nError in creating Salpha! (compr2xbwt)\n");

			// Create Salpha from Ualpha
			for(k=0, startb=0; k < xbwtstr->SItemsNum; k++){
				memcpy(xbwtstr->alphaStr + startb, S[(int)Ualpha[k]], strlen(S[(int)Ualpha[k]]));
				startb += strlen(S[Ualpha[k]]);
				}
			xbwtstr->alphaLen = startb;
			realloc(xbwtstr->alphaStr,xbwtstr->alphaLen);
			i += calphalen;

			// decompressing the Pcdata
			data_decompress(ctext + i, cpcdatalen, &(xbwtstr->pcdataStr), &(xbwtstr->pcdataLen));
			i += cpcdatalen;

			if(i != ctext_len)
				fatal_error("Error in decompressing! (COMPR2STR)\n");
			break;

		case DISTINCT:

			// Read the Prologue 
			init_buffer(ctext,50);
			xbwtstr->TextLength			= bbz_bit_read(32);
			xbwtstr->SItemsNum			= bbz_bit_read(32);
			xbwtstr->TagAttrItemsCard	= bbz_bit_read(32);
			xbwtstr->PcdataItems		= bbz_bit_read(32);
			clastlen					= bbz_bit_read(32);		
			calphalen					= bbz_bit_read(32);	
			cpcdatalen					= bbz_bit_read(32);	

			// Decompressing Slast
			xbwtstr->lastLen = xbwtstr->SItemsNum;
			i = 7 * 4;
			data_decompress(ctext + i, clastlen, &(xbwtstr->lastStr), &(xbwtstr->lastLen));
			i += clastlen;

			// Decompressing Salpha
			data_decompress(ctext + i, calphalen, &(xbwtstr->alphaStr), &(xbwtstr->alphaLen));
			i += calphalen;

			// decompressing the Pcdata
			data_decompress(ctext + i, cpcdatalen, &(xbwtstr->pcdataStr), &(xbwtstr->pcdataLen));
			i += cpcdatalen;

			if(i != ctext_len)
				fatal_error("Error in decompressing! (COMPR2STR)\n");
			break;
		}

}
