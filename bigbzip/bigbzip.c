/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   bigbzip.c 
   Ver 1.0    20-may-2005

   Program to compute the compressed version of a file 
   based on the Burrows-Wheeler Transform applied onto the 
   whole data. We use the combination: BWT+MTF+RLE+MultiTable Huffamn.
   
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

#include "mytypes.h"
#include "bigbzip.h"



/*!
  Function main: read the parameters from the command line and prints the time usage
*/

int main (int argc, char *argv[])
{
  int getopt (int argc, char * const *argv, const char *options);
  void fatal_error(char *s);
  clock_t times(struct tms *buffer);
  clock_t end, start;
  struct tms r;
  extern char *optarg;
  extern int optind, opterr, optopt;
  struct stat info;
  int fd = -1;
  FILE *outfile; 
  UChar *ctext, *text;
  UInt32 text_len, ctext_len;
  double tot_time;            // time usage
  int decompress;
  char c, *infile_name, *outfile_name;

  fprintf(stderr, "\n ----------------------------------------------------------\n");
  fprintf(stderr, "                   BigBzip vers 1.0  (May 2005)              \n"); 
  fprintf(stderr, "    by Paolo Ferragina, University of Pisa             \n\n");
  fprintf(stderr, "    Compressor based on the Burrows-Wheeler Transform.  \n");
  fprintf(stderr, "    Its specialty is the use of the BWT on the whole   \n");
  fprintf(stderr, "    file plus Multi-Table Huffman as order0 compressor.     \n");
  fprintf(stderr, " ----------------------------------------------------------\n");

 if (argc<2) {
  fprintf(stderr, "\nUsage:\n\t");
  fprintf(stderr, "%s [-d] infile [-o outfile]\n\n", argv[0]);
  fprintf(stderr, "\t-d \t\tto decompress;\n");
  fprintf(stderr, "\t-o outfile      output filename;\n");
  fprintf(stderr, "\n\n");
 }
  decompress=0;
  infile_name=outfile_name=NULL;
  opterr=0;
  while ((c=getopt(argc, argv, "do:")) != -1) {
    switch (c)
      {
        case 'd':
          decompress = 1;  break;
        case 'o':
          outfile_name = optarg;  break;
        case '?':
          fprintf(stderr, "Unknown option: %c (main)\n", optopt);
	  exit(1);
      }
  }

  // Manage the input file
  if (optind<argc)
    infile_name=argv[optind];
  if ((fd = open(infile_name, O_RDONLY)) < 0)
   	  fatal_error("Cannot open the input file for reading\n");

  // Manage the output file
  if(outfile_name==NULL) {
    /* add ".bw" for compression ".y" for decompression */
    outfile_name = (char *) malloc(strlen(infile_name)+5);
    outfile_name = strcpy(outfile_name, infile_name);
    if(decompress) outfile_name = strcat(outfile_name, ".y");
		else outfile_name = strcat(outfile_name, ".bbz");
	}

  outfile = fopen( outfile_name, "wb"); // b is for binary: required by DOS
  if(outfile==NULL) fatal_error("Error in opening the output file!\n");
 
  //--------- start measuring time
  times(&r);
  start=(r.tms_utime=r.tms_stime);
	
  if(!decompress) {

		// MMAPping the input file to an internal memory array
		stat(infile_name, &info); 
  		text_len = (UInt32) info.st_size;
		text = (UChar *) mmap(0, text_len, PROT_READ, MAP_SHARED, fd, 0) ;
		if (!text) fatal_error("Failed MMAPping the input file!\n");
	
		// Issuing the compress function
		bigbzip_compress(text, text_len, &ctext, &ctext_len);

		// Writing the compressed file to disk
		// NOTE: The first 4 bytes encode the original text length
		// NOTE: The next 4 bytes encode the eof_pos of the BWT
		fwrite(ctext, sizeof(UChar), ctext_len, outfile);

		free(ctext); munmap(text,text_len);
	  }	else {

		// MMAPping the input file to an internal memory array
		stat(infile_name, &info); 
  		ctext_len = (UInt32) info.st_size;
		ctext = (UChar *) mmap(0, ctext_len, PROT_READ, MAP_SHARED, fd, 0) ;
		text = NULL;
		if (!ctext) fatal_error("MMAPping the input compressed text failed\n");

		// Issuing the decompress function
		bigbzip_decompress(ctext, ctext_len, &text, &text_len);
		
		// Writing the uncompressed text to disk
		fwrite(text, sizeof(UChar), text_len, outfile);
		
		free(text); munmap(ctext,ctext_len);		
	}

 //------------ stop measuring time
  times(&r);
  end=(r.tms_utime+r.tms_stime);
  tot_time = ((double) (end-start))/CLK_TCK;
  
 //------------ prints the resulting figures
  if(decompress){
	  printf("\n\nInput file: %s, size %d bytes\n",outfile_name, ctext_len);
 	  printf("Output file: %s, size %d bytes\n",infile_name,text_len);	  
	  printf("Compression ratio: %.3f %% \n", 100 * ((double) ctext_len)/text_len);
	  printf("Decompression time:  %f seconds\n", tot_time);
	  } else {
		printf("\n\nInput file: %s, size %d bytes\n",infile_name, text_len);
 		printf("Output file: %s, size %d bytes\n",outfile_name,ctext_len);	  
		printf("Compression ratio: %.3f %%\n", 100 * ((double) ctext_len)/text_len);
		printf("Compression time:  %f seconds\n", tot_time);
		  }

  return 0;
}

