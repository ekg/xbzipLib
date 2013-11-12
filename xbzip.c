/***************************************************************************
 *   Copyright (C) 2005 by Paolo Ferragina, Università di Pisa             *
 *   Contact address: ferragina@di.unipi.it		                           *
 *                                                                         *
 *   Description: This program uses the library xbzip.h to implement	   *
 *   the compressor for XML documents proposed in						   *                                                                      
 *                                                                         *
 *    P. Ferragina, F. Luccio, G. Manzini, S. Muthukrishnan                *                               
 *    "Structuring labeled trees for optimal succinctness, and beyond"     *                                                                   
 *    IEEE Foundations of Computer Science, Pittsburg, USA 2005            *                                                     *
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

//------ YOU MUST INCLUDE THIS FOR USING XBZIP -----------
#include "xbzip.h"  
int BLOCK_ALPHA_LEN  = 8000;	// default value, in #symbols
int NUM1_IN_BLOCK    = 1000;	// default value, in #1
int Verbose=0;

// For search statistics
int Alpha_Block_Counter=0;
int Alpha_Byte_Counter=0;
int Last_Block_Counter=0;
int Last_Byte_Counter=0;
int Pcdata_Block_Counter=0;
int Pcdata_Byte_Counter=0;
//--------------------------------------------------------



int main(int argc, char **argv) {
  int getopt (int argc, char * const *argv, const char *options);
  char *strndup(const char *s, size_t n);
  double start_partial_timer, end_partial_timer, tot_partial_timer;
  double end_timer, start_timer, tot_timer;
  extern char *optarg;
  extern int optind, opterr, optopt;
  struct stat info;
  int fd = -1;
  FILE *outfile; 
  UChar *ctext, *text, *tmp, *path_string, **path, *snippet, cc;
  UInt32 text_len, ctext_len;
  int visualize, decompress, compress, compr_type, indexing, extracting, searching, printing;
  int first_row, last_row, i, j, path_len, num_occ, path_occ, startc, row2text, snippetLength;
  int printedRow, navigating, *navigate_array;
  char c, *infile_name, *outfile_name;
  xbwt_index_type index;

 if (argc<2) {
    printf("\n_______________________________________________________________________\n\n");
	printf("XBzip -- A BWT-based Transform to compress and index XML files\n");
	printf("Version: 1.0 (May 2005)\n"); 
	printf("Author:  Paolo Ferragina, University of Pisa, Italy\n");
	printf("Internet: www.di.unipi.it/~ferragin\n");
	printf("Description:\n");
	printf("    P. Ferragina, F. Luccio, G. Manzini, S. Muthukrishnan\n");
	printf("    \"Structuring labeled trees for optimal succinctness, and beyond\"\n");
	printf("    IEEE Symposium on the Foundations of Computer Science, 2005.\n");
    printf("_________________________________________________________________________\n\n");
	printf("\n--- Usage as a compressor:\n\n");
    printf("xbzip [-c TYPE][-d TYPE] [-o outFileName] inFileName\n\n");
    printf("\t-c to compress, TYPE is \n");
	printf("\t\t 0 Kth order Compressor over two pieces: Last fused with Salpha, and Pcdata\n");
	printf("\t\t 1 fuse Last with Salpha and then concatenate with Pcdata (plain)\n");
	printf("\t\t 2 Last with DeltaCompressor, Kth order Compressor over Salpha and Pcdata\n");
	printf("\t\t 3 Kth order Compressor over Last, over Pcdata, and Salpha with Mtf+MultiHuff\n");
	printf("\t\t 4 Kth order Compressor over each of the three distinct pieces\n");
    printf("\t-d to decompress, TYPE is as for -c\n");
    printf("\t-o name of the compressed file \n");
	printf("\t-v verbose mode\n\n");
	printf("inFileName must have extension .xml with -c, and .xbz with -d.\n");
	printf("Option -c (and not -o) generates a file with name inFileName_TYPE.xbz.\n");
	printf("Option -o must specify a file name ending with .xbz.\n");
	printf("Option -d needs a file name ending with .xbz.\n\n\n");
	printf("--- Usage as a compressed indexer:\n\n");
    printf("xbzip [ -i [-l NUM1][-a NUMS] ] [-e] [-p row] [-s \"PATH\" [-w]] [-o outFileName] inFileName \n\n");
	printf("\t-i to index\n");
	printf("\t    -l NUM1 is the #1s in a Last's block (default is 1000)\n");
	printf("\t    -a NUMS is the #symbols in an Alpha's block (default is 8000)\n");
	printf("\t-s PATH searches for PATH in the document (see below)\n");
	printf("\t-t test navigation speed\n");
	printf("\t-w visualize the snippet of Pcdata where the searched path occurs\n");
	printf("\t-e extracting the whole indexed document\n");
	printf("\t-p ROW well-formed print of the subtree descending from the input ROW [0 = whole doc]\n");
	printf("\t-v verbose mode (-v -v for detailed printing)\n\n");
	printf("inFileName must have extension .xml with -i, and .xbzi with -e or -s.\n");
	printf("Option -i generates a file with name inFileName.xbzi if [-o] is not included.\n\n");
	printf("PATH is composed by using <tagname, @attrname, =substring-value.\n");
	printf("Examples of Xpath expressions ----> PATH\n");
	printf("//chapter/section[author=\"Paolo\"] ----> <chapter<section@author=Paolo\n");
	printf("//chapter/section[.=\"Paolo\"] ----> <chapter<section=Paolo\n");
	printf("\n\n");
	return 0;
	}

  printf("\n__________________________________________________________\n\n");
  printf("XBzip - A BWT-based Transform to compress XML files\n");
  printf("Version : 1.0 (May 2005)\n"); 
  printf("Author : Paolo Ferragina, University of Pisa, Italy\n");
  printf("Internet: www.di.unipi.it/~ferragin\n");
  printf("__________________________________________________________\n");
 
  decompress=0; compress=0; indexing=0; extracting = 0; searching = 0; compr_type=0;
  visualize = 0; infile_name=NULL;outfile_name=NULL; printing = 0; row2text = 0;
  opterr=0; navigating = 0;
  path_string=NULL;
  while ((c=getopt(argc, argv, "tvwl:a:ip:es:c:d:o:")) != -1) {
    switch (c)
      {
        case 'v':
          Verbose++;  
		  break;
        case 'w':
          visualize++;  
		  break;
        case 't':
          navigating++;  
		  break;
        case 'c':
          compress = 1; 
		  compr_type = (UChar) atoi(optarg);
		  break;
        case 'd':
          decompress = 1;
		  compr_type = (UChar) atoi(optarg);
		  break;
        case 'i':
          indexing = 1;  
		  break;
        case 'e':
          extracting = 1;  
		  break;
        case 'l':
          NUM1_IN_BLOCK = atoi(optarg);  
		  break;
        case 'a':
          BLOCK_ALPHA_LEN = atoi(optarg);  
		  break;
         case 'o':
          outfile_name = optarg;  
		  break;
         case 's':
          path_string = optarg;  
		  searching = 1; 
		  break;
         case 'p':
          row2text = atoi(optarg);  
		  printing = 1; 
		  break;
        case '?':
          printf("Unknown option: %c (main)\n", optopt);
	  exit(1);
      }
  }

  if (visualize && (!searching))
	  fatal_error("Use -w together with -s!\n");

  if ( (NUM1_IN_BLOCK <= 0) || (BLOCK_ALPHA_LEN <= 0) )
	  fatal_error("The size of the block features must be grater than 0! (MAIN)\n");

  if ( (decompress + navigating + compress + extracting + indexing + searching + printing == 0) )
	  fatal_error("You must specify either (de)comression or (de)indexing or searching!\n");

  if ( (decompress + navigating + compress + extracting + indexing + searching + printing > 1) )
	  fatal_error("You must specify just one among (de)comression, (de)indexing, searching!\n");

  if ( (compr_type < 0) || (compr_type > 4) )
	  fatal_error("Please, look at the options for -c or -d !\n");

  printf("We use the following settings:\n");
  printf("\t#1 in a Last-block          = %d\n",NUM1_IN_BLOCK);
  printf("\tByte-size of a Salpha-block = %d\n",BLOCK_ALPHA_LEN);

  // Manage the input file

  if (optind<argc){
    infile_name=argv[optind];
	tmp = infile_name + strlen(infile_name) - 4;
	if (decompress && strcmp(tmp,".xbz"))
		fatal_error("File to decompress must end with .xbz!\n");
	if ((compress || indexing) && strcmp(tmp,".xml"))
		fatal_error("File to compress must end with .xml!\n");
	if ((extracting || searching || printing || navigating) && strcmp(tmp,"xbzi"))
		fatal_error("File to extract must end with .xbzi!\n");
  }

  if ((fd = open(infile_name, O_RDONLY)) < 0)
   	  fatal_error("Cannot open the input file for reading\n");

  // Manage the output file
  /* add "_TYPE.xbz" for compression ".y" for decompression */
  if (outfile_name == NULL) {
	outfile_name = (char *) malloc(strlen(infile_name)+10);
	outfile_name = strcpy(outfile_name, infile_name);
	if( compress ){
		switch (compr_type) {
			case 0: 
				outfile_name = strcat(outfile_name, "_0");
				break;
			case 1:
				outfile_name = strcat(outfile_name, "_1");
				break;
			case 2:
				outfile_name = strcat(outfile_name, "_2");
				break;
			case 3:
				outfile_name = strcat(outfile_name, "_3");
				break;
			case 4:
				outfile_name = strcat(outfile_name, "_4");
				break;
		}
	}

	if( decompress || extracting ) outfile_name = strcat(outfile_name, ".y");
	if( compress ) outfile_name = strcat(outfile_name, ".xbz");
	if( indexing ) outfile_name = strcat(outfile_name, ".xbzi");
	  } 

  // Opening the output file, in case of not searching
  if( !(searching || printing || navigating) ) { 
		outfile = fopen( outfile_name, "wb"); // b is for binary: required by DOS
		if (! outfile)
			fatal_error("Cannot open output file! (MAIN)\n");
	  } else {
		  outfile = NULL; // useless in case of searching
		  }

  //--------- start measuring time
  start_timer=getTime();

  if( decompress ) {

		// MMAPping the input compressed text to an internal memory array
		stat(infile_name, &info); 
  		ctext_len = (UInt32) info.st_size;
		ctext = (UChar *) mmap(0, ctext_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!ctext) fatal_error("MMAPping the input compressed text failed\n");

		// decompress the XBWT data
		xbzip_decompress(ctext, ctext_len, &text, &text_len, compr_type);

		// Writing the uncompressed text to disk
		fwrite(text, sizeof(UChar), text_len, outfile);
		
		free(text); munmap(ctext,ctext_len);		

  } 

  if( compress ) {

		// MMAPping the input text to an internal memory array
		stat(infile_name, &info); 
  		text_len = (UInt32) info.st_size;
		text = (UChar *) mmap(0, text_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!text) fatal_error("Failed MMAPping the input file!\n");
	
		// Compressing the XML doc
		xbzip_compress(text,text_len,&ctext,&ctext_len, compr_type);

		// Write to disk and free the memory
		fwrite(ctext, sizeof(UChar), ctext_len, outfile);
		free(ctext); munmap(text,text_len);
  }	

  if( indexing ) {

		// MMAPping the input text to an internal memory array
		stat(infile_name, &info); 
  		text_len = (UInt32) info.st_size;
		text = (UChar *) mmap(0, text_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!text) fatal_error("Failed MMAPping the input file!\n");
	
		// Compressing the XML doc
		xbzip_index(text,text_len,&ctext,&ctext_len);

		// Write to disk and free the memory
		fwrite(ctext, sizeof(UChar), ctext_len, outfile);
		free(ctext); munmap(text,text_len);
  }	

  if( extracting ) {

		// MMAPping the input text to an internal memory array
		stat(infile_name, &info); 
  		ctext_len = (UInt32) info.st_size;
		ctext = (UChar *) mmap(0, ctext_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!ctext) fatal_error("Failed MMAPping the input file!\n");
	
		// Decompressing the XML doc
		xbzip_deindex(ctext,ctext_len,&text,&text_len);

		// Write to disk and free the memory
		fwrite(text, sizeof(UChar), text_len, outfile);
		free(text); munmap(ctext,ctext_len);
  }	

  if( searching ) {

		// MMAPping the input text to an internal memory array
		stat(infile_name, &info); 
  		ctext_len = (UInt32) info.st_size;
		ctext = (UChar *) mmap(0, ctext_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!ctext) fatal_error("Failed MMAPping the input file!\n");

		// Construct the path with at most 50 items
		path = (UChar **) malloc(sizeof(UChar *) * 50);
		if (!path) fatal_error("Error in allocating the path space! (MAIN)\n");

		path_len = 0; 
		for(startc = 0; startc < (int) strlen(path_string); ){
			
			// -------------------------------------------------
			// We are missing a format-check on the path query
			// -------------------------------------------------

			i=startc+1; // skip first char, is = or < or @
			while ( (i < (int) strlen(path_string)) &&
					(path_string[i] != '@') &&
					(path_string[i] != '<') &&
					(path_string[i] != '=')) {
					i++;
				}

			// Set the next item of the path
			path[path_len] = strndup(path_string+startc, i-startc);
			path_len++;
			if(path_len > 40)
				fatal_error("\n\nWe allow less than 40 items in the path! (MAIN)\n");

			// Set the starting position of the next item
			startc = i;
			}
		
		// Loading the serialized index into its proper data type
		printf("\nindex loading\n");
		__START_TIMER__;
		disk2index(ctext, ctext_len, &index);
		__END_TIMER__;
		printf("...overall the index loading took %.4f seconds\n\n", tot_partial_timer);

		if( Verbose )
			print_index(&index);

		// Searching
		printf("index searching...\n");
		__START_TIMER__;
		xbzip_search(&index, path, path_len, &first_row, &last_row, &path_occ, &num_occ, visualize);		
		__END_TIMER__;
		printf("...overall searching took %.4f seconds\n\n", tot_partial_timer);

		printf("-------- Search Statistics for Path Search---------------\n\n");
		printf("We accessed:\n");
		printf("%5d compressed blocks in Last:        %6d bytes.\n",	Last_Block_Counter,Last_Byte_Counter);
		printf("%5d compressed blocks in Alpha:       %6d bytes.\n",	Alpha_Block_Counter,Alpha_Byte_Counter);
		printf("%5d compressed indexes in Pcdata over %6d bytes.\n\n",Pcdata_Block_Counter,Pcdata_Byte_Counter);
		printf("---------------------------------------------------------\n\n");

		munmap(ctext,ctext_len);
  }	

  if( printing ) {
		
		// MMAPping the input text to an internal memory array
		stat(infile_name, &info); 
  		ctext_len = (UInt32) info.st_size;
		ctext = (UChar *) mmap(0, ctext_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!ctext) fatal_error("Failed MMAPping the input file!\n");
		
		// Loading the serialized index into its proper data type
		printf("\nindex loading\n");
		__START_TIMER__;
		disk2index(ctext, ctext_len, &index);
		__END_TIMER__;
		printf("...it took %.4f seconds\n\n", tot_partial_timer);

		if ((row2text < 0) || (row2text > index.SItemsNum))
			fatal_error("The input row is <0 or > #rows.\n");

		if( Verbose )
			print_index(&index);

		// Traversing the subtree and print it
		__START_TIMER__;
		Subtree2Text(&index, row2text, &printedRow, &snippet, &snippetLength);
		__END_TIMER__;
		printf("...it took %.4f seconds\n\n", tot_partial_timer);
		printf("------------- Subtree of row %d (asked %d)\n\n",printedRow,row2text);
		print_pretty_len(snippet,snippetLength);
		printf("\n\n------------------------------------------------------\n\n");

		printf("-------- Search Statistics for Path Search---------------\n\n");
		printf("We accessed:\n");
		printf("%5d compressed blocks in Last:   %6d bytes.\n",	Last_Block_Counter,Last_Byte_Counter);
		printf("%5d compressed blocks in Alpha:  %6d bytes.\n",	Alpha_Block_Counter,Alpha_Byte_Counter);
		printf("%5d compressed blocks in Pcdata: %6d bytes.\n\n",Pcdata_Block_Counter,Pcdata_Byte_Counter);
		printf("---------------------------------------------------------\n\n");

		munmap(ctext,ctext_len);
  }	

  if( navigating ) {

		// MMAPping the input text to an internal memory array
		stat(infile_name, &info); 
  		ctext_len = (UInt32) info.st_size;
		ctext = (UChar *) mmap(0, ctext_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!ctext) fatal_error("Failed MMAPping the input file!\n");

		// Loading the serialized index into its proper data type
		printf("\nindex loading\n");
		__START_TIMER__;
		disk2index(ctext, ctext_len, &index);
		__END_TIMER__;
		printf("...overall the index loading took %.4f seconds\n\n", tot_partial_timer);


		navigate_array = (int *) malloc(sizeof(int) * index.SItemsNum);
	
		printf("Setting up the test...");
		
		// No root
		for(i = 0; i < 2000 ; ){
			j = (int) ((double) ( (index.SItemsNum - 10) * rand()) / (RAND_MAX+1.0)) + 2;
			cc = get_node_type(&index, j);
			if (cc == '<' || cc == '@')
				navigate_array[i++] = j;
			}
		printf("   done!\n\n");


		// Check parent
		printf("index navigation for parent (avg over 1000 internal nodes)...\n");
		__START_TIMER__;
		for(i = 0; i < 1000 ; i++)
			get_parent(&index, navigate_array[i]);
		__END_TIMER__;
		printf("...each parent computation took %.4f seconds\n\n", tot_partial_timer / 1000.0);

		// Check child
		printf("index navigation for children (avg over 5000 internal nodes)...\n");
		__START_TIMER__;
		for(i = 1000; i < 2000 ; i++)
			get_children(&index, navigate_array[i], &first_row, &last_row);
		__END_TIMER__;
		printf("...each child-group computation took %.4f seconds\n\n", tot_partial_timer / 1000.0);

		munmap(ctext,ctext_len);
  }	


  //------------ stop measuring time
  end_timer = getTime();
  tot_timer = end_timer - start_timer;
	  
  if (! (searching || printing || navigating)) {
	  //------------ prints the resulting figures
	printf("\n\n--------------- PERFORMANCE INFOS ---------------\n\n");
	if(decompress || extracting){
	printf("Input file name: %s\n",infile_name);
	printf("Input file size: %d bytes\n",ctext_len);
	printf("Output file name: %s\n",outfile_name);	  
	printf("Output file size %d bytes\n\n",text_len);	  
	printf("Compression ratio: %.2f %%\n", 100 * ((double) ctext_len)/text_len);
	printf("Decompression time:  %.4f seconds\n", tot_timer);
	} else {
	printf("Input file name: %s\n",infile_name);
	printf("Input file size: %d bytes\n",text_len);
	printf("Output file name: %s\n",outfile_name);	  
	printf("Output file size %d bytes\n\n",ctext_len);	  
	printf("Compression ratio: %.2f %%\n", 100 * ((double) ctext_len)/text_len);
	printf("Compression time:  %.4f seconds\n\n", tot_timer);
	}

	if ((compr_type == PLAIN) && (compress))
	  printf("\n----> Remember to use a post-compressor like bzip2, gzip, ppmd,.... <----\n\n\n");
  }
  return 0;
}  /* End main */

