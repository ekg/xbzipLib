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

The XBWT data structure we create consists of three main components:
- Slast binary array of SItemsNum positions
- Salpha array of strings, being <TAGname or @ATTRname or = for Pcdata
- Pcdata string, where each Pcdata entry is prefixed by \0, the order is the one in Salpha 

The index is subdivided into three main components:
- The Last array, compressed in blocks of variable length, each
	containing a fixed number of 1 (= NUM1_BLOCKS). This eases the Select1
- The Alpha array, compressed in blocks of fixed number of symbols (= BLOCK_ALPHA_LEN items).
	This eases the RankSymbol
- The Pcdata array, compressed in blocks, each containing a variable number of
	items, and thus a fortiori of variable length. Currently each block is kept
	compressed, without indexing. Each block is formed by items having the same
	leading path in the DOM tree.
	In the future we will apply the FM-index.
******************************************************************** */


/* ------------- To manage includes and data-type definitions ---------- */
#include "xbzip.h"

// Brute-force solution to compute the partition of the PCitem array
int *PartitionArray, PartitionCount; 


/* ----------------------------------------------------------------------------
	Procedure xbzip_index()

	text: XML text to be compressed
	text_len: length of XML text
	disk: (Reference to the) index data stored as a sequence of bytes
	disk_len: (Reference to the) length of index's serialization

	The space for the index and its length is allocated here.
	---------------------------------------------------------------------------- */
void xbzip_index(UChar text[], int text_len, UChar *disk[], int *disk_len)
{
	double end_partial_timer, start_partial_timer, tot_partial_timer; // time usage
	xbwt_type xbwt;
	xbwt_string_type xbwtstr;
	xbwt_index_type index;
	int t;

	printf("\n\n------- TIMINGS ----------\n");

	// Compute the XBWT
	printf("xbwt building\n");
	__START_TIMER__;
	xbwt_builder(text, text_len, &xbwt);
	__END_TIMER__;
	printf("...overall building took %.4f seconds\n\n", tot_partial_timer);

	// Compute the Partition of the PI-array
	printf("xbwt partitioning (starting from scratch...bad!!)\n");
	__START_TIMER__;
	xbwt_partition(text, text_len);
	__END_TIMER__;
	printf("...overall partitioning took %.4f seconds\n\n", tot_partial_timer);

	// Serialize the XBWT data into three strings and some infos
	// The strings are: Slast, Salpha, and the Pcdata
	printf("xbwt serialization\n");
	__START_TIMER__;
	xbwt2xbwtstr(&xbwt, &xbwtstr);
	__END_TIMER__;
	printf("...overall serialization took %.4f seconds\n\n", tot_partial_timer);

	// Index the serialized XBWT 
	printf("xbwt indexing\n");
	xbwtstr2index(&xbwtstr, &index);
	printf("...overall indexing took %.4f seconds\n\n", tot_partial_timer);

	// Create the serialization of the index
	printf("index writing to disk\n");
	__START_TIMER__;
	index2disk(&index, disk, disk_len);
	__END_TIMER__;
	printf("...overall writing took %.4f seconds\n\n", tot_partial_timer);

	// Extended profiling of indexing information
	if (Verbose) print_index(&index);

	printf("\n\n--------------- XBWT INFOS ---------------\n\n");
	printf("Text of total length %d bytes\n\n",xbwt.TextLength);
	printf("XML tree consists of %d nodes and leaves\n\n", xbwt.SItemsNum);
	printf("TAG and ATTR names:\n");
	printf("\tnumber %15d\n",xbwt.TagAttrItemsTot);
	printf("\tdistinct %13d\n",xbwt.TagAttrItemsCard);
	printf("\tlength %15d bytes\n\n",xbwt.SalphaTotLen);
	printf("PCDATA entries:\n");
	printf("\tnumber %15d\n",xbwt.PcdataItems);
	printf("\tblocks %15d\n",PartitionCount);
	printf("\tlength %15d bytes\n\n",xbwt.PcdataTotLen);
	printf("INDEX information:\n"); 
	printf("\tLast index   = %9d bytes, #blocks = %6d\n", index.LastIndexLen, index.LastNumBlocks); 
	printf("\tAlpha index  = %9d bytes, #blocks = %6d\n", index.AlphaIndexLen, index.AlphaNumBlocks); 
	printf("\tPcdata index = %9d bytes, #blocks = %6d\n", index.PcdataIndexLen, index.PcNumBlocks); 
	printf("\tF index      = %9d bytes, #items  = %6d\n", sizeof(int) * index.AlphabetCard, index.AlphabetCard); 
	printf("\tAlphabet     = %9d bytes, #items  = %6d\n", index.AlphabetLen, index.AlphabetCard);

	// Taken from index2disk()
	t = 11 + 2*(index.LastNumBlocks+index.PcNumBlocks) + index.AlphaNumBlocks + index.AlphabetCard*(index.AlphaNumBlocks+1);
	printf("..plus a set of %d integers over 4 bytes (offsets and positions).\n\n",t); 
}


/* ----------------------------------------------------------------------
	Procedure xbzip_deindex()

	disk: index data (serialized) to be decompressed
	disk_len: length of index serialization
	text: (Reference to the) uncompressed text
	text_len: (Reference to the) length of uncompressed text

	The space for the output text and its length is allocated here.
	---------------------------------------------------------------------- */
void xbzip_deindex(UChar *disk, int disk_len, UChar *text[], int *text_len)
{
	double end_partial_timer, start_partial_timer, tot_partial_timer; // time usage
	xbwt_type xbwt;
	xbwt_string_type xbwtstr;
	xbwt_index_type index;

	printf("\n\n------- TIMINGS ----------\n");

	// Loading the serialized index into its proper data type
	printf("\nindex loading\n");
	__START_TIMER__;
	disk2index(disk, disk_len, &index);
	__END_TIMER__;
	printf("...overall loading took %.4f seconds\n\n", tot_partial_timer);

	// Decompressing the serialized XBWT, it consists of three strings and some infos
	printf("index deserialization\n");
	__START_TIMER__;
	index2xbwtstr(&index, &xbwtstr);
	__END_TIMER__;
	printf("...overall deserialization took %.4f seconds\n\n", tot_partial_timer);

	// Deserialize the XBWT data		
	printf("xbwt building\n");
	__START_TIMER__;
	xbwtstr2xbwt(&xbwtstr, &xbwt);
	__END_TIMER__;
	printf("...overall xbwt-building took %.4f seconds\n\n", tot_partial_timer);

	// Reconstruct the XML text
	printf("\ntext reconstruction\n");
	__START_TIMER__;
	xbwt_unbuilder(&xbwt, text, text_len);
	__END_TIMER__;
	printf("...overall reconstruction took %.4f seconds\n\n", tot_partial_timer);


	// Extended profiling of indexing information
	if (Verbose) print_index(&index);

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
	printf("INDEX information:\n"); 
	printf("\tLast index   = %9d bytes, #blocks = %6d\n", index.LastIndexLen, index.LastNumBlocks); 
	printf("\tAlpha index  = %9d bytes, #blocks = %6d\n", index.AlphaIndexLen, index.AlphaNumBlocks); 
	printf("\tPcdata index = %9d bytes, #blocks = %6d\n", index.PcdataIndexLen, index.PcNumBlocks); 
	printf("\tF index      = %9d bytes, #items  = %6d\n", sizeof(int) * index.AlphabetCard, index.AlphabetCard); 
}



/* ----------------------------------------------------------------------------
	Indexes the XBWT_STRING data type 
	--------------------------------------------------------------------------- */
void xbwtstr2index(xbwt_string_type *xbwtstr, xbwt_index_type *index)
{
	char *strndup(const char *s, size_t n);	
	double end_partial_timer, start_partial_timer, tot_partial_timer; // time usage
	int count_ones, i, j, clastlen, aa, error;
	int k, tot_symb_len, startb, start_alpha_byte, index_offset, calphalen;
	int start_last_index, start_last_byte;
	UChar **S, *calpha, *clast;
	HHash_table ht;
	Hash_node *hn;
	int *GlobalPrefixCounts, current_block, skip, textcode;	
	ulong fmindex_len; // for the FM-index
	void *fmindex;     // for the FM-index


	// Copy these two fields
	index->TextLength = xbwtstr->TextLength;
	index->SItemsNum = xbwtstr->SItemsNum;
	index->PcdataNum = xbwtstr->PcdataItems;

	// Oversize the LastIndex data structures, then we will resize it
	index->LastNumBlocks = floor(xbwtstr->SItemsNum / NUM1_IN_BLOCK) + 3;
	index->LastIndexLen = max(100000, 2 * xbwtstr->SItemsNum);
	index->LastIndex = (UChar *) malloc(sizeof(UChar) * index->LastIndexLen);
	index->LastOffsetBlocks = (int *) malloc(sizeof(int) * index->LastNumBlocks);
	index->LastPosBlocks = (int *) malloc(sizeof(int) * index->LastNumBlocks);

	// Prepare for writing
	__START_TIMER__;

	// Encoding Slast by DELTA-code in blocks containing NUM1_IN_BLOCK 1s
	// LastPosBlocks[] is the starting position of each block
	// The last block is empty and its first position delimits Last (useful for scanning)
	current_block=0;
	count_ones = 1;
	start_last_byte = 0;
	start_last_index = 0;

	// -1 to ensure a non-empty last block
	for(j=0; j < xbwtstr->lastLen - 1; ){

		// Set the block infos, j points to Last[pos]=1 finishing the block
		if ((count_ones % NUM1_IN_BLOCK == 0) && ( j!=0 )){

			compress_block(xbwtstr->lastStr + start_last_byte, j - start_last_byte + 1, &clast, &clastlen);
			memcpy(index->LastIndex + start_last_index, clast, clastlen);
			free(clast);

			index->LastPosBlocks[current_block] = start_last_byte; // first position in Last of the block 
			index->LastOffsetBlocks[current_block] = start_last_index; // first byte position of the compr-block

			current_block++;
			start_last_byte = j+1; // points to the first char of the next block
			start_last_index += clastlen;
			}

		// find a sequence (0^*)1, increment 'count_ones'
		j++; // move to the next run
		for(; (j < xbwtstr->lastLen) && (xbwtstr->lastStr[j] == 0); j++) ;
		count_ones++;
		}

	// We set the ending block, it is guaranteed to be not empty 
	compress_block(xbwtstr->lastStr + start_last_byte, j - start_last_byte + 1, &clast, &clastlen);
	memcpy(index->LastIndex + start_last_index, clast, clastlen);
	free(clast);

	index->LastPosBlocks[current_block] = start_last_byte; // first position in Last of this block 
	index->LastOffsetBlocks[current_block] = start_last_index; // first byte position of the compr-block
	start_last_index += clastlen;
	current_block++;

	// We set a dummy block 
	index->LastIndexLen = start_last_index;
	index->LastNumBlocks = current_block+1; // correct value (last is empty)
	index->LastPosBlocks[current_block] = index->SItemsNum; // out of Last
	index->LastOffsetBlocks[current_block] = index->LastIndexLen; // dummy first byte position 

	realloc(index->LastIndex,index->LastIndexLen);

	__END_TIMER__;
	printf("  compressed the Last index in %.4f seconds\n", tot_partial_timer);

	// Lexicographic encode the TAG and ATTR names, and the symbol =
	// TagAttrCard = # distinct TAG-ATTRS names (plain letters terminated by \0)

	__START_TIMER__;
	index->AlphabetCard = xbwtstr->TagAttrItemsCard + 1; 	// We sum 1 because of the =
	S = (UChar **) malloc(sizeof(UChar *) * index->AlphabetCard);
	HHashtable_init(&ht, 2 * (index->AlphabetCard) );
	for(k=0, j=0, tot_symb_len=0; k < xbwtstr->alphaLen; ){
		startb=k;
		k++;
		while ( (k < xbwtstr->alphaLen) &&
				(xbwtstr->alphaStr[k] != '@') &&
				(xbwtstr->alphaStr[k] != '<') &&
				(xbwtstr->alphaStr[k] != '=')) {
					k++;
				}
		if (HHashtable_insert(xbwtstr->alphaStr + startb, k - startb, 0, &ht)) { // if new
			S[j++] = strndup(xbwtstr->alphaStr + startb, k - startb);
			tot_symb_len += (k - startb);
		}
	}

	if (j != index->AlphabetCard)
		fatal_error("Error in computing the alphabet cardinality! (XBWTSTR2INDEX)\n");

	// Sort lexicoraphically the alphabet symbols
	qsort(S, index->AlphabetCard, sizeof(UChar *), S_cmp);

	// Compute their lexicographic code
	for(i=0; i<index->AlphabetCard; i++){
		hn = HHashtable_search(S[i], strlen(S[i]), &ht);
		hn -> code = i;
		}

	// Create the string of the alphabet symbols: <tag, @attr, =
	// Symbols are separated by \0
	index->Alphabet = (UChar *) malloc(sizeof(UChar) * (tot_symb_len + index->AlphabetCard));

	for(i=0, index->AlphabetLen=0; i < index->AlphabetCard; i++){
		memcpy(index->Alphabet + index->AlphabetLen, S[i], strlen(S[i]));
		index->Alphabet[index->AlphabetLen + strlen(S[i])] = '\0';
		index->AlphabetLen += strlen(S[i]) + 1;
		}

	if ( index->AlphabetLen != (tot_symb_len + index->AlphabetCard) )
		fatal_error("Error in counting the alphabet length! (XBWTSTR2INDEX)\n");
	__END_TIMER__;
	printf("  compressed the Alphabet in %.4f seconds\n", tot_partial_timer);

	// Compute the index infos for the Alpha array
	// everything is ovsersized, then we resize them correctly
	// NOTE: The last compressed block of Alpha is empty, only OffsetBlocks is initialized
	__START_TIMER__;
	index->AlphaNumBlocks = floor(xbwtstr->SItemsNum / BLOCK_ALPHA_LEN) + 3;
	index->AlphaPrefixCounts = (int *) malloc(sizeof(int) * (index->AlphaNumBlocks * index->AlphabetCard));
	index->AlphaOffsetBlocks = (int *) malloc(sizeof(int) * index->AlphaNumBlocks);
	index->AlphaIndexLen = max(xbwtstr->TextLength,10000); 
	index->AlphaIndex = (UChar *) malloc(sizeof(UChar) * (index->AlphaIndexLen) );


	// We estimate the PrefixCounts counting at the **END** of each block
	GlobalPrefixCounts = (int *) malloc(sizeof(int) * index->AlphabetCard);
	for(i=0; i < index->AlphabetCard; i++)
		GlobalPrefixCounts[i] = 0;

	current_block = 0;
	start_alpha_byte = 0;
	index_offset = 0;

	// REMIND: Our PrefixCounts count till the **END** of each block
	// k moves over the bytes in Salpha
	// j moves over the items in Salpha
	// startb is the starting byte of an item in Salpha
	for(startb=0,j=0; startb < xbwtstr->alphaLen; ){
		k=startb+1; // next byte of an item
		while ( (k < xbwtstr->alphaLen) && (xbwtstr->alphaStr[k] != '@') &&
				(xbwtstr->alphaStr[k] != '<') && (xbwtstr->alphaStr[k] != '=')) {
					k++;
				}
		hn=HHashtable_search(xbwtstr->alphaStr + startb, k - startb, &ht);
		GlobalPrefixCounts[hn->code]++; // increase the prefix counting of this item  
		j++;							// new item

		if (j % BLOCK_ALPHA_LEN == 0){
			for(i=0; i < index->AlphabetCard; i++)
				index->AlphaPrefixCounts[current_block * index->AlphabetCard + i] = GlobalPrefixCounts[i];

			// Compress the block via a stream compressor
			compress_block(xbwtstr->alphaStr + start_alpha_byte, k - start_alpha_byte, 
				&calpha, &calphalen);
			if (index_offset + calphalen > index->AlphaIndexLen)
				fatal_error("Index size for Salpha is too small! (XBWTSTR2INDEX)\n");

			memcpy(index->AlphaIndex + index_offset, calpha, calphalen);
			free(calpha);

			index->AlphaOffsetBlocks[current_block] = index_offset;
			index_offset += calphalen;
			start_alpha_byte = k;
			current_block++;
			}

		startb = k;							// stating position of the next item (if any)
	}

	// Constructing the last block of Salpha 
	if(start_alpha_byte != k) {	// if symbols are pending
		for(i=0; i < index->AlphabetCard; i++)
			index->AlphaPrefixCounts[current_block * index->AlphabetCard + i] = GlobalPrefixCounts[i];

		// Compress the block via a stream compressor
		compress_block(xbwtstr->alphaStr + start_alpha_byte, k - start_alpha_byte, 
			&calpha, &calphalen);	
		memcpy(index->AlphaIndex + index_offset, calpha, calphalen);
		free(calpha);

		index->AlphaOffsetBlocks[current_block++] = index_offset;
		index_offset += calphalen;
	}

	// Append a dummy (empty) block to facilitate the scanning ops
	index->AlphaOffsetBlocks[current_block++] = index_offset;


	if( (index->AlphaNumBlocks < current_block) || (index->AlphaIndexLen < index_offset))
		fatal_error("Out of bounds in the alpha index creation! (XBWTSTR2INDEX)\n");

	index->AlphaNumBlocks = current_block; // +1 of correct value
	index->AlphaIndexLen = index_offset;

	// Resize the data structure for the Alpha array
	realloc(index->AlphaIndex,index->AlphaIndexLen);
	realloc(index->AlphaOffsetBlocks, sizeof(int) * index->AlphaNumBlocks);
	realloc(index->AlphaPrefixCounts, sizeof(int) * index->AlphaNumBlocks * index->AlphabetCard);
	__END_TIMER__;
	printf("  compressed the Alpha index in %.4f seconds\n", tot_partial_timer);


	// Pcdata (one index per block of Pcdata items)
	__START_TIMER__;
	index->PcNumBlocks = PartitionCount; // from procedure xbwt_partition() 
	index->PcOffsetBlocks = (int *) malloc(sizeof(int) * (index->PcNumBlocks) );
	index->PcBlockItems = (int *) malloc(sizeof(int) * (index->PcNumBlocks) );


	// Oversized, then we resize it
	index->PcdataIndexLen = max(xbwtstr->TextLength, 10000); 
	index->PcdataIndex = (UChar *) malloc(sizeof(UChar) * (index->PcdataIndexLen) );

	// Recall that Pcdata is prefixed by \0
	// startb is the starting byte of an item in Pcdata
	// j moves over the block numbers
	// k moves over the single Pcdata item	

	// Check the partition	
	for(i=0,aa=0; i < PartitionCount; i++)
		aa += PartitionArray[i];
	if(aa != xbwtstr->PcdataItems){
		printf("#partitioned = %d, #items = %d\n",aa,xbwtstr->PcdataItems);
		fatal_error("Error in the partitioning! (xbwt_partition)\n");
		}
	for(startb=0, j=0, index_offset=0; startb < xbwtstr->pcdataLen; j++){
		index->PcOffsetBlocks[j] = index_offset; 
		index->PcBlockItems[j] = PartitionArray[j];  

		// Identify the set of Pcdata items to index-compress together
		for(i=0, k=startb; i < index->PcBlockItems[j]; i++) {
			k++; // first byte is \0
			while ( (k < xbwtstr->pcdataLen) && (xbwtstr->pcdataStr[k] != '\0') ) k++;
		}

		// (Index and) compress together, k lies over a \0
		// -a 2 -f 0 -b 1024
		error = build_index(xbwtstr->pcdataStr+startb, (ulong)(k - startb), "-a 2 -f 0.005 -b 2048 -B 32", &fmindex);
		IFERROR(error);
		error=index_size(fmindex, &fmindex_len);
		IFERROR(error);
		error = save_index_mem(fmindex, index->PcdataIndex + index_offset); 
		IFERROR(error);
		error = free_index(fmindex);
		IFERROR(error);

		index_offset += (int)fmindex_len;
		if (index_offset > index->PcdataIndexLen)
			fatal_error("Writing the Pcdata index out of bounds!\n");

		startb = k;
	}
	    printf("hereMM\n");
	// Set the correct byte length of the compressed blocks
	index->PcdataIndexLen = index_offset;

	// Set the correct number of Pcdata blocks
	if (j != PartitionCount) 
		fatal_error("Error in compressing the Pcdata blocks! (XBWTSTR2INDEX)\n");

	// Resize the overestimated memory
	realloc(index->PcdataIndex, index->PcdataIndexLen );

	__END_TIMER__;
	printf("  compressed the Pcdata index in %.4f seconds\n", tot_partial_timer);

	// Compute the F array
	__START_TIMER__;
	index->F = (int *) malloc(sizeof(int) * index->AlphabetCard);
	if( !index->F ) fatal_error("Error in allocating F! (XBWTSTR2INDEX)\n");
	
	// Compute the code of = which does not occur as first item in PI-string
	hn=HHashtable_search("=", 1, &ht);
	textcode = hn->code;

    printf("yo\n");

	// We exploit GlobalPrefixCounts[]
	// First PI-string is empty, first TAG-ATTR encoded with 0
	index->F[0]=1;						
	for(i=0; i < index->AlphabetCard - 1; i++){
		if ( i == textcode ){  // Skip the =
			index->F[i+1]=index->F[i];
		} else {
			j=index->F[i];
			skip=0;
			while(skip != GlobalPrefixCounts[i]) // skip this number of 1s in Slast
				if (xbwtstr->lastStr[j++] == 1) skip++;
			index->F[i+1]=j; 
			}
		}
	__END_TIMER__;
	printf("  created the F index %.4f seconds\n\n", tot_partial_timer);

}


/* ----------------------------------------------------------------------------
	Extracting from the INDEX data type the info for the XBWT_STRING data type
	--------------------------------------------------------------------------- */
void index2xbwtstr(xbwt_index_type *index, xbwt_string_type *xbwtstr)
{
	int i, j, cursor, error;
	UChar *alphablock, *pcdata, *lastblock;
	int alphablocklen, startb, lastblocklen;
	unsigned long pcdatalen;
	ulong block_len; 
	void *fmindex;     // for the FM-index


	xbwtstr->TextLength = index->TextLength;
	xbwtstr->SItemsNum = index->SItemsNum;
	xbwtstr->TagAttrItemsCard = index->AlphabetCard -1; // minus symbol =

	// Reconstruct Last array
	xbwtstr->lastLen = xbwtstr->SItemsNum;
	xbwtstr->lastStr = (UChar *) malloc(sizeof(UChar) * xbwtstr->lastLen);
	if (!xbwtstr->lastStr)
		fatal_error("Error in allocating Last array! (INDEX2STR)\n");

	// The last one is dummy
	for(i=0; i < index->LastNumBlocks - 1; i++) {
		decompress_block(index->LastIndex + index->LastOffsetBlocks[i],
			index->LastOffsetBlocks[i+1]-index->LastOffsetBlocks[i], 
			&lastblock, &lastblocklen);
		memcpy(xbwtstr->lastStr + index->LastPosBlocks[i], lastblock, lastblocklen);
		free(lastblock);

		if (index->LastPosBlocks[i+1] - index->LastPosBlocks[i] != lastblocklen)
			fatal_error("Error in decompressing a block of Last! (INDEX2XBWTSTR)\n");
		}

	// Alpha array: oversized, then we will resize it
	xbwtstr->alphaLen = xbwtstr->TextLength;
	xbwtstr->alphaStr = (UChar *) malloc(sizeof(UChar) * xbwtstr->alphaLen);
	if (!xbwtstr->alphaStr)
		fatal_error("Error in allocating Alpha array! (INDEX2STR)\n");

	// Construct the Alpha index, by compressing the array into blocks
	// i = moves over the block numbers
	// cursor = stores the starting byte of each Alpha block
	// -1 to take care of the last dummy (empty) block
	for(i=0, cursor=0; i < index->AlphaNumBlocks - 1; i++) {
	
		// starting byte of the block in the compressed string
		startb = index->AlphaOffsetBlocks[i]; 

		// We use a generic stream compressor for compression
		decompress_block(index->AlphaIndex+startb, 
			index->AlphaOffsetBlocks[i+1] - index->AlphaOffsetBlocks[i],
			&alphablock, &alphablocklen);
		memcpy(xbwtstr->alphaStr+cursor,alphablock,alphablocklen);
		free(alphablock);

		cursor += alphablocklen;
	}

	// Reset the infos and resize the data structure
	xbwtstr->alphaLen = cursor;
	realloc(xbwtstr->alphaStr,	xbwtstr->alphaLen);

	// Pcdata: indexing and compression per group of items having same S_pi
	xbwtstr->PcdataItems = index->PcdataNum; 
	xbwtstr->pcdataLen = max(index->TextLength,10000); 
	xbwtstr->pcdataStr = (UChar *) malloc(sizeof(UChar) * (xbwtstr->pcdataLen) );

	
	// Recall that Pcdata is prefixed by \0
	// i moves over the bytes of the uncompressed Pcdata
	// j moves over the block numbers
	for(i=0,j=0; j < index->PcNumBlocks - 1; ){
	
		// Load the FM-index and decompress the indexed text
		error = load_index_mem(&fmindex, 
			index->PcdataIndex + index->PcOffsetBlocks[j], 
			index->PcOffsetBlocks[j+1] - index->PcOffsetBlocks[j]); 
		IFERROR(error);
		error = get_length(fmindex, &block_len);
		IFERROR(error);
		error = extract(fmindex, 0, block_len-1, &pcdata, &pcdatalen);		
		IFERROR(error);
		if (block_len != pcdatalen)
			fatal_error("Error in decompressing the FM-indexed block!");
		error = free_index(fmindex);
		IFERROR(error);		

		memcpy(xbwtstr->pcdataStr + i, pcdata, (int)pcdatalen);

		i += (int)pcdatalen;
		j++;
	}

	// Load the FM-index and decompress the indexed text
	error = load_index_mem(&fmindex, 
		index->PcdataIndex + index->PcOffsetBlocks[j], 
		index->PcdataIndexLen - index->PcOffsetBlocks[j]); 
	IFERROR(error);
	error = get_length(fmindex, &block_len);
	IFERROR(error);
	error = extract(fmindex, 0, block_len-1, &pcdata, &pcdatalen);		
	IFERROR(error);
	if (block_len != pcdatalen)
		fatal_error("Error in decompressing the FM-indexed block!");
	error = free_index(fmindex);
	IFERROR(error);		

	memcpy(xbwtstr->pcdataStr + i, pcdata, (int)pcdatalen);

	i += pcdatalen;
	xbwtstr->pcdataLen = i;
}


/* ----------------------------------------------------------------------------
	Returns the number of 1 in the array prefix Last[0,pos]
		Remind that Last has been partitioned in variable length blocks
		that contain a fixed number of 1-bit equal to NUM1_IN_BLOCK
	--------------------------------------------------------------------------- */
int rank1_last(xbwt_index_type *index, int pos)
{
	int start, blockNum, blockLen, diff, rank;
	UChar *blockStr;

	if( (pos < 0) || (pos >= index->SItemsNum) )
		fatal_error("Out-of-bound rank required on Last array! (RANK1)\n");

	// Compute the block of the input position
	for(blockNum=0; index->LastPosBlocks[blockNum+1] <= pos; blockNum++) ;

	// Prepare for decoding the block
	start = index->LastOffsetBlocks[blockNum]; 
	decompress_block(index->LastIndex+start, 
		index->LastOffsetBlocks[blockNum+1] - index->LastOffsetBlocks[blockNum], 
		&blockStr, &blockLen);

	// Relative 'pos' within the current block	
	diff = pos - index->LastPosBlocks[blockNum];
	if(diff > blockLen)
		fatal_error("Error in accessing a Last block for Rank1!\n");

	// Decoding the Last block containing the 'pos'
	rank = blockNum * NUM1_IN_BLOCK;

	while(diff >= 0){
		if (blockStr[diff--] != 0) rank++;
		} 			
	
	// Statistics
	Last_Block_Counter++;
	Last_Byte_Counter += index->LastPosBlocks[blockNum+1] - index->LastPosBlocks[blockNum] + 1;

	free(blockStr);
	return rank;
}

/* ----------------------------------------------------------------------------
	Returns the position of the RANK-th 1 in the array Last
		Remind that Last has been partitioned in variable length blocks
		each containing a fixed number of 1-bit, namely NUM1_IN_BLOCK
	--------------------------------------------------------------------------- */
int select1_last(xbwt_index_type *index, int rank)
{
	int start, blockNum, diffrank, pos, blockLen;
	UChar *blockStr;

	// Compute the block of the input rank, and the relative rank
	blockNum = floor((rank - 1) / NUM1_IN_BLOCK);
	diffrank = ((rank-1) % NUM1_IN_BLOCK) + 1;

	if( (rank <= 0) || (blockNum >= index->LastNumBlocks-1) )
		fatal_error("Out-of-bound select required on Last array! (SELECT1)\n");

	// Prepare for decoding the block
	start = index->LastOffsetBlocks[blockNum]; 
	decompress_block(index->LastIndex+start, 
		index->LastOffsetBlocks[blockNum+1] - index->LastOffsetBlocks[blockNum], 
		&blockStr, &blockLen);

	// Initial position of the block
	pos = -1;
	while((diffrank > 0) && (pos < blockLen)){
		pos++;
		if(blockStr[pos] != 0) diffrank--;
		} 			

	if (pos >= blockLen)
		fatal_error("Our-of-bound in Last block! (select1)\n");

	// Statistics
	Last_Block_Counter++;
	Last_Byte_Counter += index->LastPosBlocks[blockNum+1] - index->LastPosBlocks[blockNum] + 1;

	free(blockStr);

	// pos is relative to the blockNum-th block of Last
	return (pos + index->LastPosBlocks[blockNum]); 
}

/* --------------------------------------------------------------------------------
	Returns the position of the RANK-th symbol q in the array Alpha
		Recall Alpha is partitioned in blocks of fixed #symbols (BLOCK_ALPHA_LEN)
		Symbol q is either <tag and @attr
	------------------------------------------------------------------------------- */
int selectSymb_alpha(xbwt_index_type *index, UChar *q, int rank)
{
	int start, block, diffrank, symb_code, pos, i;
	UChar *alphablock; 
	int alphablocklen;

	if (rank <= 0)
		fatal_error("Asked to select a symbol of non positive rank! (SELECT_ALPHA)\n");

	// Rank within the sorted alphabet string
	symb_code = get_symbol_code(index,q);

	// Compute the block containing the searched symbol
	// Remind that AlphaPrefixCounts[] counts till the block's end
	for(block=0; (block < index->AlphaNumBlocks) && 
				 (rank > index->AlphaPrefixCounts[block * index->AlphabetCard + symb_code]); 
		block++) ;

	if( (block < 0) || (block >= index->AlphaNumBlocks) ) 
		fatal_error("Out-of-bound select required on Alpha array! (SELECT_ALPHA)\n");

	// Decode the Alpha's block
	start = index->AlphaOffsetBlocks[block]; 
	decompress_block(index->AlphaIndex+start, 
			index->AlphaOffsetBlocks[block+1] - index->AlphaOffsetBlocks[block], &alphablock, &alphablocklen);

	// Statistics
	Alpha_Block_Counter++;
	Alpha_Byte_Counter += index->AlphaOffsetBlocks[block+1] - index->AlphaOffsetBlocks[block];

	// Relative rank within the block: PrefixCounts count till block's end
	if (block > 0) 
		{ 
		diffrank = rank - index->AlphaPrefixCounts[(block - 1) * index->AlphabetCard + symb_code]; 
		pos = block * BLOCK_ALPHA_LEN; 
		} else {
			diffrank = rank; 
			pos = 0;
			}

	// Scan the block symbol by symbol
	// searching for the DIFFRANK's item equal to 'q'
	i=0; 
	while( 1 ){

		// Delimit a symbol in Alpha
		start = i;
		i++; // skip first char, is = or < or @
		while ( (i < alphablocklen) &&
				(alphablock[i] != '@') &&
				(alphablock[i] != '<') &&
				(alphablock[i] != '=')) {
					i++;
				}

		// Check if q is equal to this symbol
		if( (!strncmp(alphablock+start, q, i-start)) && (strlen(q) == (unsigned int) i-start) ){
			diffrank--;
			if( diffrank == 0) return pos;
			}

		// Increment the current position in Alpha
		pos++;
		if( diffrank < 0 )
			fatal_error("Error in determining the pos! (SELECT_ALPHA)\n");
	}

	free(alphablock);
	return -1;
}


/* --------------------------------------------------------------------------------
	Returns the rank of the symbol q in the prefix Alpha[1,pos]
		Remind that Alpha has been partitioned in blocks of fixed length BLOCK_ALPHA_LEN
		Symbol q is either <tag or @attr
	------------------------------------------------------------------------------- */
int rankSymb_alpha(xbwt_index_type *index, UChar *q, int pos)
{
	int start, block, diffpos, symb_code, rank, i;
	UChar *alphablock; 
	int alphablocklen;

	if ( (pos < 0) || (pos >= index->SItemsNum) )
		fatal_error("Out of bounds in symbol ranking! (RANK_ALPHA)\n");

	// Rank within the sorted alphabet string
	symb_code = get_symbol_code(index,q);

	// Compute the block containing the searched position
	block = floor(pos / BLOCK_ALPHA_LEN);
	diffpos = pos % BLOCK_ALPHA_LEN;

	// Remind that PrefixCounts[] counts by the block's end
	rank = 0;
	if (block > 0)
		rank += index->AlphaPrefixCounts[(block-1) * index->AlphabetCard + symb_code]; 

	// Decoding the block
	start = index->AlphaOffsetBlocks[block]; 
	decompress_block(index->AlphaIndex+start, index->AlphaOffsetBlocks[block+1] - index->AlphaOffsetBlocks[block],
						&alphablock, &alphablocklen);

	// Statistics
	Alpha_Block_Counter++;
	Alpha_Byte_Counter += index->AlphaOffsetBlocks[block+1] - index->AlphaOffsetBlocks[block];

	//Scan the block symbol by symbol
	// counting the number of them equal to q in prefix block[0,diffpos]
	i=0;
	while( diffpos >= 0 ){

		// Extract the next symbol
		start = i;
		i++; // skip first char, is = or < or @
		while ( (i < alphablocklen) &&
				(alphablock[i] != '@') &&
				(alphablock[i] != '<') &&
				(alphablock[i] != '=')) {
					i++;
				}

		// Compare it against q, in case increment the RANK
		if( (!strncmp(alphablock+start, q, i-start)) && (strlen(q) == (unsigned int) i-start) )
			rank++;

		// Decrements the current position in the block
		diffpos--;
	}	

	free(alphablock);

	return rank;
}


/* --------------------------------------------------------------------------------
	Returns the code of the symbol q within the sorted Alphabet		
		Alphabet is lexicographically sorted with terminating \0 per each string
	------------------------------------------------------------------------------- */

int get_symbol_code(xbwt_index_type *index, UChar *q)
{
	int len,k,start;

	for(k=0,start=0; k < index->AlphabetCard; k++){

		// Computes the length of the next symbol
		len = strlen(index->Alphabet+start);

		// Compare it against the input 'q'
		if ( !(strcmp(q, index->Alphabet+start)) )
			return k;

		// Sets the starting byte of the next symbol
		start += len+1;
		}

	fatal_error("Symbol not found in alphabet! (SELECT_ALPHA)\n");
	return -1;
}

/* ----------------------------------------------------------------------------
	Serialization of the index data type 
	--------------------------------------------------------------------------- */
void index2disk(xbwt_index_type *index, UChar *disk[], int *disk_len)
{
	int cursor, i, j;

	*disk_len = (7 + 2 * index->LastNumBlocks + 2 + index->AlphaNumBlocks + (index->AlphabetCard) * (index->AlphaNumBlocks) +
		2 + 2 * index->PcNumBlocks + index->AlphabetCard ) * sizeof(int) + index->LastIndexLen + 
		index->AlphaIndexLen + index->AlphabetLen + index->PcdataIndexLen;
	*disk = (UChar *) malloc(sizeof(UChar) * (*disk_len) );
	if (! (*disk) )
		fatal_error("Error in serializing the index! (INDEX2DISK)\n");


	// Variable 'cursor' is used for checking and correct byte offsets
	init_buffer(*disk,*disk_len); 	cursor = 0;

	bbz_bit_write(32,index->TextLength); cursor += sizeof(int);
	bbz_bit_write(32,index->SItemsNum); cursor += sizeof(int);
	bbz_bit_write(32,index->PcdataNum); cursor += sizeof(int);
	bbz_bit_write(32,index->AlphabetLen); cursor += sizeof(int);
	bbz_bit_write(32,index->AlphabetCard); cursor += sizeof(int);

	// Write the infos about Last
	bbz_bit_write(32,index->LastIndexLen); cursor += sizeof(int);
	bbz_bit_write(32,index->LastNumBlocks); cursor += sizeof(int);

	for(i=0; i < index->LastNumBlocks; i++){
		bbz_bit_write(32,index->LastOffsetBlocks[i]);
		cursor += sizeof(int);
		}
	for(i=0; i < index->LastNumBlocks; i++){
		bbz_bit_write(32,index->LastPosBlocks[i]);	
		cursor += sizeof(int);
		}

	if( cursor > *disk_len)
		fatal_error("Error *LAST* in writing the index on disk! (INDEX2DISK)\n");

	// Write the infos about Alpha
	bbz_bit_write(32,index->AlphaNumBlocks); cursor += sizeof(int);
	bbz_bit_write(32,index->AlphaIndexLen); cursor += sizeof(int);

	for(i=0; i < index->AlphaNumBlocks; i++){
		bbz_bit_write(32,index->AlphaOffsetBlocks[i]);
		cursor += sizeof(int);
		}
	for(i=0; i < index->AlphaNumBlocks; i++){
		for(j=0; j < index->AlphabetCard; j++){
			bbz_bit_write(32,index->AlphaPrefixCounts[i*index->AlphabetCard+j]);  
			cursor += sizeof(int);
			}
		}

	if( cursor > *disk_len)
		fatal_error("Error *ALPHA* in writing the index on disk! (INDEX2DISK)\n");

	// Write the infos about Pcdata
	bbz_bit_write(32,index->PcdataIndexLen); cursor += sizeof(int);
	bbz_bit_write(32,index->PcNumBlocks); cursor += sizeof(int);
	for(i=0; i < index->PcNumBlocks; i++){
		bbz_bit_write(32,index->PcOffsetBlocks[i]);
		cursor += sizeof(int);
		}
	for(i=0; i < index->PcNumBlocks; i++){
		bbz_bit_write(32,index->PcBlockItems[i]);
		cursor += sizeof(int);
		}

	if( cursor > *disk_len)
		fatal_error("Error *PC* in writing the index on disk! (INDEX2DISK)\n");

	// Write the infos about F
	for(i=0; i < index->AlphabetCard; i++){
		bbz_bit_write(32,index->F[i]);
		cursor += sizeof(int);
		}

	if( cursor > *disk_len)
		fatal_error("Error *SIGMA* in writing the index on disk! (INDEX2DISK)\n");

	// Write the index by exploiting the variable 'cursor'
	memcpy(*disk + cursor, index->LastIndex, index->LastIndexLen);
	cursor += index->LastIndexLen;

	memcpy(*disk + cursor, index->AlphaIndex, index->AlphaIndexLen);
	cursor += index->AlphaIndexLen;

	memcpy(*disk + cursor, index->PcdataIndex, index->PcdataIndexLen);
	cursor += index->PcdataIndexLen;

	memcpy(*disk + cursor, index->Alphabet, index->AlphabetLen);
	cursor += index->AlphabetLen;

	if( cursor != *disk_len)
		fatal_error("Error in writing the index on disk! (INDEX2DISK)\n");

}


/* ----------------------------------------------------------------------------
	Deserialization of the index data type 
	--------------------------------------------------------------------------- */
void disk2index(UChar *disk, int disk_len, xbwt_index_type *index)
{
 	int cursor, i, j;


	// Variable 'cursor' is used for checking and correct offset setting
	init_buffer(disk,disk_len); cursor = 0;

	index->TextLength=bbz_bit_read(32); cursor += sizeof(int);
	index->SItemsNum=bbz_bit_read(32); cursor += sizeof(int);
	index->PcdataNum=bbz_bit_read(32); cursor += sizeof(int);
	index->AlphabetLen=bbz_bit_read(32); cursor += sizeof(int);
	index->AlphabetCard=bbz_bit_read(32); cursor += sizeof(int);

	// Read the infos about Last
	index->LastIndexLen=bbz_bit_read(32); cursor += sizeof(int);
	index->LastNumBlocks=bbz_bit_read(32); cursor += sizeof(int);
	index->LastOffsetBlocks = (int *) malloc(sizeof(int) * index->LastNumBlocks);
	index->LastPosBlocks = (int *) malloc(sizeof(int) * index->LastNumBlocks);

	for(i=0; i < index->LastNumBlocks; i++){
		index->LastOffsetBlocks[i]=bbz_bit_read(32);
		cursor += sizeof(int);
		}
	for(i=0; i < index->LastNumBlocks; i++){
		index->LastPosBlocks[i]=bbz_bit_read(32);	
		cursor += sizeof(int);
		}

	if( cursor > disk_len)
		fatal_error("Error *LAST* in reading the index from disk! (INDEX2DISK)\n");

	// Read the infos about Alpha
	index->AlphaNumBlocks=bbz_bit_read(32); cursor += sizeof(int);
	index->AlphaIndexLen=bbz_bit_read(32); cursor += sizeof(int);
	index->AlphaPrefixCounts = (int *) malloc(sizeof(int) * (index->AlphaNumBlocks * index->AlphabetCard));
	index->AlphaOffsetBlocks = (int *) malloc(sizeof(int) * index->AlphaNumBlocks);
	for(i=0; i < index->AlphaNumBlocks; i++){
		index->AlphaOffsetBlocks[i]=bbz_bit_read(32);
		cursor += sizeof(int);
		}
	for(i=0; i < index->AlphaNumBlocks; i++){
		for(j=0; j < index->AlphabetCard; j++){
			index->AlphaPrefixCounts[i*index->AlphabetCard+j]=bbz_bit_read(32);  
			cursor += sizeof(int);
			}
		}

	if( cursor > disk_len)
		fatal_error("Error *ALPHA* in reading the index from disk! (DISK2INDEX)\n");

	// Read the infos about Pcdata
	index->PcdataIndexLen=bbz_bit_read(32); cursor += sizeof(int);
	index->PcNumBlocks=bbz_bit_read(32); cursor += sizeof(int);
	index->PcOffsetBlocks = (int *) malloc(sizeof(int) * index->PcNumBlocks);
	index->PcBlockItems = (int *) malloc(sizeof(int) * index->PcNumBlocks);
	for(i=0; i < index->PcNumBlocks; i++){
		index->PcOffsetBlocks[i]=bbz_bit_read(32);
		cursor += sizeof(int);
		}
	for(i=0; i < index->PcNumBlocks; i++){
		index->PcBlockItems[i]=bbz_bit_read(32);
		cursor += sizeof(int);
		}

	if( cursor > disk_len)
		fatal_error("Error *PC* in reading the index from disk! (DISK2INDEX)\n");

	// Read the infos about F
	index->F = (int *) malloc(sizeof(int) * index->AlphabetCard);
	for(i=0; i < index->AlphabetCard; i++){
		index->F[i]=bbz_bit_read(32);
		cursor += sizeof(int);
		}

	if( cursor > disk_len)
		fatal_error("Error *SIGMA* in reading the index from disk! (DISK2INDEX)\n");

	// Loading the indexes
	index->LastIndex = disk + cursor;
	cursor += index->LastIndexLen;

	index->AlphaIndex = disk + cursor;
	cursor += index->AlphaIndexLen;

	index->PcdataIndex = disk + cursor;
	cursor += index->PcdataIndexLen;

	index->Alphabet = disk + cursor;
	cursor += index->AlphabetLen;

	if( cursor != disk_len)
		fatal_error("Error in reading the index from disk! (DISK2INDEX)\n");

}

/* ----------------------------------------------------------------------------
	Search for a path of 'pathlen' symbols stored in the array of pointers 'path'
		the procedure returns various infos as (reference) parameters.
	In the case of path search, then *pathocc = *occ
	In case of content search, then maybe *pathocc != *occ
	--------------------------------------------------------------------------- */
void xbzip_search(xbwt_index_type *index, UChar **path, int pathlen, 
				  int *firstRow, int *lastRow, int *pathocc, int *occ, int visualize)
{
	int z, k1, k2, j, i, sum, error;
	int symb_code, symb_forbidden, blockStart, blockStartNext;
	int pcfirst_item, pclast_item, pcfirst_block, pclast_block; 
	UChar *pattern, *snippet_text_array;
	unsigned long occNum, *snippet_length_array, uncomprBlockLen; 
	void *fmindex;

//	UChar *snippet_text; unsigned long snippet_len, *occArray; // for the Location

	i=0;

	symb_code = get_symbol_code(index,path[i]);
	symb_forbidden = get_symbol_code(index,"=");

	*firstRow = index->F[symb_code];
	*lastRow = index->F[symb_code+1] - 1;

	// Manage the case of the next symbol =
	if (symb_code == symb_forbidden-1)
        *lastRow = index->F[symb_code+2] - 1;

	// Manage the case of the last symbol
	if (symb_code == index->AlphabetCard - 1)
		*lastRow = index->SItemsNum - 1;

	// Main loop
	while ( (*firstRow <= *lastRow) && (i+1 < pathlen) ) {

		i++;

		if ( (i != pathlen - 1) && (path[i][0] == '=') )
			fatal_error("Error in composing the Path Query! (XBZIP_SEARCH)\n");

		// Actually it plays a role only for i = 0
		if(path[i][0] == '='){ // Content search
			printf("\nThis is a content query!\n\n");
			break;	
			}

		symb_code = get_symbol_code(index,path[i]);

		z = rank1_last(index, index->F[symb_code] - 1);

		k1= rankSymb_alpha(index,path[i], *firstRow - 1);
		j = z+k1;
		if (j <= 0) { *firstRow = 0; }
		else { *firstRow = select1_last(index,j)+1; }


		k2=rankSymb_alpha(index,path[i], *lastRow);
		*lastRow = select1_last(index,z+k2);
	}


	printf("\n\nRows in [%d,%d] are prefixed by the Tag-Attr part of the Query\n",
		*firstRow, *lastRow);

	// We found NO occurrences
	if(*firstRow > *lastRow) {
		*pathocc=0;
		*occ=0;
		printf("The Tag-Attr path occurs %d times.\n", *pathocc );
		printf("The overall Query-Path occurs %d times.\n",*occ);
		return;
	}

	// We take into account the group of children
	*pathocc = rank1_last(index, *lastRow) - rank1_last(index, *firstRow - 1);

	// Here, we manage the path queries
	if (!visualize && path[pathlen-1][0] != '=') {
		*occ = *pathocc;
		printf("The Tag-Attr path occurs %d times.\n", *pathocc );
		printf("The overall Query-Path occurs %d times.\n",*occ);
		return;
	}

	// Otheriwse, we manage the content queries
	pcfirst_item = rankSymb_alpha(index, "=", *firstRow - 1);
	pclast_item = rankSymb_alpha(index, "=", *lastRow) - 1;
	printf("Content search within the interval [%d,%d] of PcdataItems.\n",pcfirst_item,pclast_item);

	// We search for the first block containing an occurrence
	for(j=0, sum=0; sum + index->PcBlockItems[j] < pcfirst_item; j++)
		sum += index->PcBlockItems[j];
	pcfirst_block = j;

	// We search for the last block containing an occurrence
	for(; sum + index->PcBlockItems[j] < pclast_item; j++)
		sum += index->PcBlockItems[j];
	pclast_block = j; 

	printf("Content search within the interval [%d,%d] of PcdataBlocks.\n",pcfirst_block,pclast_block);

	// We search within each Pcdata block
	// FOR NOW just print the block
	*occ = 0;
	for(j=pcfirst_block; j <= pclast_block; j++){

		if (j < index->PcNumBlocks-1) {
			blockStart = index->PcOffsetBlocks[j];
			blockStartNext = index->PcOffsetBlocks[j+1];
			} else { // the last block
				blockStart = index->PcOffsetBlocks[j];
				blockStartNext = index->PcdataIndexLen;
				}

		// Statistics
		Pcdata_Block_Counter++;
		Pcdata_Byte_Counter += blockStartNext - blockStart;

		pattern = path[pathlen-1] + 1; // discard the leading '='


		error = load_index_mem(&fmindex, index->PcdataIndex + blockStart, blockStartNext - blockStart); 
		IFERROR(error);
		error = get_length(fmindex, &uncomprBlockLen);
		IFERROR(error);

		// Counting the occurrences in the present Pcdata block
		error = count(fmindex, pattern, strlen(pattern), &occNum);
		IFERROR(error);

		printf("\n\n----------------------------------\n");
		printf("Block #%d contains %d occurrences\n", j, (int)occNum);
		printf("----------------------------------\n");
		
		// Searching the FM-index of the loaded block
		if ((occNum > 0) && visualize) {

			// Locating the pattern position
			//error=locate(fmindex, pattern, strlen(pattern), &occArray, &occNum);
			error=display(fmindex, pattern, strlen(pattern), 20, &occNum, 
						&snippet_text_array, &snippet_length_array);
			IFERROR(error);

			// print the snippets
			for(i=0; i < (int)occNum; i++){
				printf("Occurrence #%d: ",i+1);
				print_pretty_len(snippet_text_array + i * (strlen(pattern)+2*20),
								snippet_length_array[i]);
				printf("\n\n");

				// Locating the pattern position
				//printf("Occurrence #%d in position %d\n",i,(int)occArray[i]);
				//error=extract(fmindex, max( ((int)occArray[i]) - 20,0), 
				//			min(((int) occArray[i]) + 20 + (int)strlen(pattern), (int) uncomprBlockLen), 
				//			&snippet_text, &snippet_len);
				//IFERROR(error);
				//print_pretty_len(snippet_text, snippet_len);
				//printf("\n\n");

				}
			printf("\n\n\n");
			}

	error = free_index(fmindex); IFERROR(error);					
	*occ += occNum;
	}
	printf("\n\nIn summary:\n");
	printf("    Query path restricted to Tag-Attrs occurs %d times.\n", *pathocc );
	printf("    Query path occurs %d times.\n\n\n",*occ);
}


/* ----------------------------------------------------------------------------
	Groups the Pcdata items according to their leading path. 
	It must be equal.
	--------------------------------------------------------------------------- */
void xbwt_partition(UChar *text, int text_len)
{
	Tree_node **nodes_array, *root, *x, *y;
	int TreeSize,cursor,i,aa;
	int prev,curr;

	root = xml2tree(text, text_len, &TreeSize);
	nodes_array = (Tree_node **) malloc(sizeof(Tree_node *) * TreeSize);
	if (!nodes_array) fatal_error("Error in allocating nodes_array! (XBWT_PART)");
	cursor=0;
	tree2nodearray(root, nodes_array, &cursor);
	qsort(nodes_array, TreeSize, sizeof(Tree_node *), PI_cmp);

	PartitionArray = (int *) malloc( sizeof(int) * TreeSize );
	PartitionCount = 0;
	for(i=0; i < TreeSize; i++) PartitionArray[i]=0;

	// Compare an item with the previous one
	prev = 0;
	for(curr=1; curr < TreeSize; ){ 
		
		// Search for the next Pcdata item
		for(; (curr < TreeSize) && (nodes_array[curr]->type != TEXT); curr++) ;

		if (curr >= TreeSize) return; 

		if ( nodes_array[curr]->type != TEXT )
			fatal_error("It should be a TEXT node ! (xbwt_partition)\n");

		x = nodes_array[curr]->parent;
		y = nodes_array[prev]->parent;

		// Compare the upward labeled path
		while(x && y && (x != y))
		{
			if (x->len_str != y->len_str) { break; }
			aa = strncmp(x->str, y->str, x->len_str);
			if (aa != 0) { break; }
			x = x->parent; y = y->parent;
		}
	
		if ( x != y ) // new path
			PartitionCount++;

		PartitionArray[PartitionCount-1]++;

		// Advance pointers
		prev = curr;
		curr++;
	}
}


/* --------------------------------------------------------------------------------
	Computes the label and its code for the input row (node)
	------------------------------------------------------------------------------- */
void get_node_labelNcode(xbwt_index_type *index, int row, UChar **symb, int *symbcode)
{
	char *strndup(const char *s, size_t n);
	int start, block, diffpos, i;
	UChar *alphablock; 
	int alphablocklen;


	if ( (row < 0) || (row >= index->SItemsNum) )
		fatal_error("Out of bounds in row! (GET_CHILDREN)\n");

	// Compute the block containing the searched position
	block = floor(row / BLOCK_ALPHA_LEN);
	diffpos = row % BLOCK_ALPHA_LEN;

	// Decoding the block
	start = index->AlphaOffsetBlocks[block]; 
	decompress_block(index->AlphaIndex+start, index->AlphaOffsetBlocks[block+1] - index->AlphaOffsetBlocks[block],
						&alphablock, &alphablocklen);

	//Scan the block symbol by symbol
	i=0;
	while( diffpos > 0 ){

		// Extract the next symbol
		start = i;
		i++; // skip first char, is = or < or @
		while ( (i < alphablocklen) &&
				(alphablock[i] != '@') &&
				(alphablock[i] != '<') &&
				(alphablock[i] != '=')) {
					i++;
				}

		// Decrements the current position in the block
		diffpos--;
	}	

	// Extract the next symbol
	start = i;
	i++; // skip first char, is = or < or @
	while ( (i < alphablocklen) &&
			(alphablock[i] != '@') &&
			(alphablock[i] != '<') &&
			(alphablock[i] != '=')) {
				i++;
			}
	*symb = strndup(alphablock + start, i-start);
	*symbcode = get_symbol_code(index, *symb);
	free(alphablock);
}

/* --------------------------------------------------------------------------------
	Returns the row of the ith child of the input row (node)
	In the case of an input text node, then it returns 0 (with *first = *last = -1)
	------------------------------------------------------------------------------- */
int get_children(xbwt_index_type *index, int row, int *first, int *last)
{
	int x, symbCode, rowSymb, rankSymb;
	UChar *symb;

	if ((row < 0) || (row >= index->SItemsNum))
		fatal_error("Out-of-bound row! (GET_Ith_CHILD)\n");

	// label of the input node
	get_node_labelNcode(index, row, &symb, &symbCode);
	if (symb[0] == '=') { *first = -1; *last = -1; return 0; }

	// PI-row where symb first occurs as prefix
	rowSymb = index->F[symbCode];
	x = rank1_last(index,rowSymb-1); // surely rowSymb>0

	// Rank of the current symbol
	rankSymb = rankSymb_alpha(index, symb, row);

	// children
	*first = select1_last(index, x+rankSymb-1) + 1;
	*last = select1_last(index, x+rankSymb);
	return ((*last) - (*first) + 1);
}

/* --------------------------------------------------------------------------------
	Returns the row of the parent of the input row (node)
	------------------------------------------------------------------------------- */
int get_parent(xbwt_index_type *index, int row)
{
	int rank_symb,label_code,len,p,xx,yy;
	UChar *node_label;

	if (row >= index->SItemsNum)
		fatal_error("Out-of-bound row! (GET_PARENT)\n");

	// Compute the label and its code of the parent
	for(label_code = 0, node_label = index->Alphabet; 
		(label_code < index->AlphabetCard - 1) && (index->F[label_code+1] <= row); 
		label_code++) 
		{
		len = strlen(node_label);
		node_label += len+1; // skip the ending \0
		}

	xx = rank1_last(index,row-1); 
	yy = rank1_last(index, index->F[label_code]-1); // surely F[] > 0
	rank_symb = xx - yy + 1;
	p = selectSymb_alpha(index, node_label, rank_symb);
	return p;
}

/* -----------------------------------------------------------------------------------------------
	Returns the type of the input row (node): < (tag name), @ (attr name), = (text or attr value)
	---------------------------------------------------------------------------------------------- */
UChar get_node_type(xbwt_index_type *index, int row)
{
	UChar *node_label;
	int label_code;

	get_node_labelNcode(index, row, &node_label, &label_code);
	return(node_label[0]);
}

/* --------------------------------------------------------------------------------
	Returns the row of the ith child labeled c of the input row (node)
	It is -1 in case the child does not exist.
	------------------------------------------------------------------------------- */
int get_ith_symb_child(xbwt_index_type *index, int row, int rank, UChar *c)
{
	int firstChild, lastChild, beforeFirst, beforeLast;

	if (get_children(index, row, &firstChild, &lastChild) == 0)
		return -1;
	
	// Count symbols equal to c before first and last
	beforeFirst = rankSymb_alpha(index, c, firstChild - 1);
	beforeLast = rankSymb_alpha(index, c, lastChild);
	if (rank > (beforeLast - beforeFirst))
		return -1;

	// jump to the correct child
	return( selectSymb_alpha(index, c, beforeFirst + rank) );
}

/* --------------------------------------------------------------------------------
	Returns the text content of the input row (node)
	It is -1 in case the node is not a text node.
	------------------------------------------------------------------------------- */
int get_text_content(xbwt_index_type *index, int row, int *cLen, UChar **c)
{
	char *strndup(const char *s, size_t n);
	int pcItem, pcBlock, j, diffRank, sum, error, blockStartNext;
	UChar *block;
	unsigned long blocklen, blocklen1;
	void *fmindex;

	if (get_node_type(index,row) != '=')
		{ *cLen = 0; *c = NULL; return -1; }

	pcItem = rankSymb_alpha(index, "=", row);

	// We search for the first block containing an occurrence
	for(j=0, sum=0; (sum + index->PcBlockItems[j]) < pcItem; j++)
		sum += index->PcBlockItems[j];
	pcBlock = j;

	// We search for the exact Pcdata 
	// --- THIS IS VERY SLOW: We should have a direct access to the diffRank-th Pcdata item ---

	// Load the FM-index and decompress the indexed text
	if(pcBlock == index->PcNumBlocks-1)
		{ blockStartNext = index->PcdataIndexLen; }
	else { 	blockStartNext = index->PcOffsetBlocks[pcBlock+1]; }


	error = load_index_mem(&fmindex, 
		index->PcdataIndex + index->PcOffsetBlocks[pcBlock], 
		blockStartNext - index->PcOffsetBlocks[pcBlock]); 
	IFERROR(error);
	error = get_length(fmindex, &blocklen);
	IFERROR(error);
	error = extract(fmindex, 0, blocklen-1, &block, &blocklen1);		
	IFERROR(error);
	if (blocklen != blocklen1)
		fatal_error("Error in decompressing the FM-indexed block!");
	error = free_index(fmindex);
	IFERROR(error);		

	// Access the correct Pcdata item
	for(diffRank = pcItem - sum; diffRank > 0; diffRank--){
		for(; (*block) != '\0'; block++, blocklen--) ;
		block++; blocklen--;
		}

	// block points to the starting byte of the searched PcItem
	// blocklen accounts for the #bytes remaining to be scanned
	for(*cLen = 0; (block[*cLen] != '\0') && (*cLen < (int)blocklen) ; (*cLen)++) ;
	*c = strndup(block, *cLen);

	// Statistics
	Pcdata_Block_Counter++;
	Pcdata_Byte_Counter += index->PcOffsetBlocks[pcBlock+1] - index->PcOffsetBlocks[pcBlock];
	return 1;
}


/* --------------------------------------------------------------------------------
	Returns in *snippet the text version of the subtree descending from row (node)
	If row = 0, then it returns the entire document.
	The parameter printed_row contains the actual root of the printed subtree.
	This allows to manage the case in which row points to text or attr-name rows.
	------------------------------------------------------------------------------- */
void Subtree2Text(xbwt_index_type *index, int row, int *printed_row, UChar **snippet, int *snippetLength)
{
	int parent, *Stack, snippetLengthMax, InAngleBrackets, cursor, top_stack;
	int tmp, first, last, num, tokenLen, PosSalpha, k;
	UChar *token, c, *stmp;


	if (row < 0 ) { 
		fatal_error("Input ROW is negative. (Subtree2Text)\n"); } 
	if (row >= index->SItemsNum ) { 
		fatal_error("Input ROW is larger than Salpha's rows. (Subtree2Text)\n"); } 

	// We wish a well-formed printing, hence we ensure that the row points to
	// a tag name. Otherwise we percolate upward the tree searching for a 
	// parent tag of the input row.
	parent=row;
	switch ( get_node_type(index, row) ){
		case '@':
			// we go back to its parent tag
			parent=get_parent(index,row);
			break;
		case '=':
			// we go back to its parent (possibly, grandparent) tag
			parent=get_parent(index,row);
			if (get_node_type(index, parent) == '@') {
				row = parent; parent=get_parent(index,row);
			}
			break;
		case '<':
			// We print the descending subtree of the input row
			break;
		case '?':
			fatal_error("Unknown node type for subtree printing. (Subtree2Text)\n");
		}

	// We assume a maximum depth of 10000
	Stack = (int *) malloc(sizeof(int) * 10000);
	if (!Stack) fatal_error("Error in allocating Stack! (Subtree2Text)");

	// We work with chuncks of 200Kb
	snippetLengthMax = 200000; 
	*snippet = (UChar *) malloc(sizeof(UChar) * 200000 );

	InAngleBrackets=0;	// flags if we are within <....>	
	cursor=0;			// moves over the text under construction
	top_stack=-1;		// points to the top of Stack

	Stack[++top_stack] = parent; // Push the starting row

	for( ; top_stack > -1; ) {

		if (cursor >= snippetLengthMax)
			fatal_error("Out-of-snippet in subtree retrieval! (Subtree2Text)\n");
		if (cursor > snippetLengthMax - 100000){
			snippetLengthMax += 100000;
			stmp = realloc(*snippet, snippetLengthMax);
			if( !stmp ) fatal_error("\nError in allocating the snippet space! (Subtree2Text)\n");
			*snippet = stmp;
			}

		PosSalpha = Stack[top_stack--]; // Pop the top item

		// Managing the closing tag 
		// We set its PosAlpha to a negative value... a trick 
		if(PosSalpha < 0){
			if (InAngleBrackets != 0)
				fatal_error("InAngleBrackets is not 0 and PosAlpha is negative ! (Subtree2Text)\n");
			(*snippet)[cursor++] = '<';
			(*snippet)[cursor++] = '/';
			// Cancel the <
			get_node_labelNcode(index, -PosSalpha, &token, &tmp);
			token++;
			memcpy(*snippet+cursor,token,strlen(token));
			cursor += strlen(token);			
			// Append the >
			(*snippet)[cursor++] = '>';
			continue; // back to Pop from Stack
		} 
		
		// We are within <.....> 
		if ( InAngleBrackets ) { 
			
			// We extracted something outside <....>
			// Hence, we need to manage the closing of >
			c = get_node_type(index,PosSalpha);
			if ((c == '=') || (c == '<')) {
				InAngleBrackets=0;
				Stack[++top_stack]=PosSalpha; // re-insert (push) into the stack
				(*snippet)[cursor++] = '>';
				continue; // back to Pop from Stack
			} 
			
			(*snippet)[cursor++] = ' ';
			get_node_labelNcode(index, PosSalpha, &token, &tmp);
			token++;
			memcpy(*snippet+cursor,token,strlen(token));
			cursor += strlen(token);

			// Create the attribute string
			(*snippet)[cursor++] = '='; (*snippet)[cursor++] = '\"';
			num=get_children(index,PosSalpha,&first,&last);
			if (num > 1) fatal_error("I expected one single child! (Subtree2Text)\n");
			get_text_content(index, first, &tokenLen, &token);
			memcpy(*snippet+cursor,token,tokenLen);
			cursor += tokenLen;
			(*snippet)[cursor++] = '"';

			// No pushing in the stack since we completed the attribute
			continue; // back to pop from Stack
		}


		// Manage the texts
		get_node_labelNcode(index, PosSalpha, &token, &tmp);
		tokenLen = strlen(token);
		c = token[0];

		if (c == '=') { 
			get_text_content(index, PosSalpha, &tokenLen, &token);
			if (token[0] != (UChar) 255) // not dummy filler for empty tag
				{
				memcpy(*snippet+cursor,token,tokenLen);
				cursor += tokenLen;
				}
			continue; // back to pop from Stack
		}


		// Manage the tags, mark that we are in a tag
		if (c == '<') { 
			if (PosSalpha != 0) {
				InAngleBrackets=1; 
				memcpy(*snippet + cursor,token,tokenLen);
				cursor += tokenLen;
				}
			// Insert in the stack the children of the current node
			num=get_children(index, PosSalpha, &first, &last);
			if (PosSalpha != 0)
				Stack[++top_stack] = -PosSalpha; // Push negative Pos as closing tag: trick
			for(k=last; k >= first; k--)
				Stack[++top_stack] = k;
			}
		}

	// To avoid some spurious chars after the last tag
	*snippetLength = cursor;
	*printed_row = parent;
}

void compress_block(uchar *source, int sourceLen, uchar **dest, int *destLen)
{
	int err;		

	bigbzip_compress(source, sourceLen, dest, destLen);

	// Using ZLib
    /*
	*destLen = (int) (1.01 * sourceLen + 100000) * sizeof(int);
	*dest = (uchar *)calloc(*destLen, 1);
	memcpy(*dest,&sourceLen,4);  // Source Length for uncomression

	err = compress2(*dest + 4, (uLong *)destLen, source, sourceLen, 9);
	*destLen += 4; // preamble containing the source length

    if (err == Z_MEM_ERROR)
		fatal_error("Memory error when compressing a data block! (COMPRESS_BLOCK)\n");

    if (err == Z_BUF_ERROR)
		fatal_error("Buffer error when compressing a data block! (COMPRESS_BLOCK)\n");

    if (err == Z_STREAM_ERROR)
		fatal_error("Stream error when compressing a data block! (COMPRESS_BLOCK)\n");

	if (err != Z_OK) 
		fatal_error("Error in compressing a data block! (COMPRESS_BLOCK)\n");
    */

}

void decompress_block(uchar *source, int sourceLen, uchar **dest, int *destLen)
{

	int err;		

	bigbzip_decompress(source, sourceLen, dest, destLen);

    /*
	// Using ZLib
	if(sourceLen < 4) 
		fatal_error("Passed tiny block to decompress ! (DECOMPRESS_BLOCK)\n");
	
	memcpy(destLen,source,4);
	*dest = (uchar *)calloc(*destLen + 10, 1);
	err = uncompress(*dest, (uLong *) destLen, source+4, sourceLen);

	if (err != Z_OK) 
		fatal_error("Error in uncompressing a data block! (DECOMPRESS_BLOCK)\n");
    */

}
