
//------------------------------------------------------
//
// This is a wrapper that allows to easily change the 
// kth order entropy compressor used in each Pcdata block or
// on the overall Salpha.
//
//------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/resource.h>

void data_compress(unsigned char *s, int slen, unsigned char **t, int *tlen)
{

  FILE *Outfile; 
  FILE *Infile;  
  int i;

  system("rm -f FileXbzipTmp.*");
  Outfile = fopen( "FileXbzipTmp.dat", "wb");
  if (!Outfile){
    printf("Error in opening Outfile! (DataCompress)");
	exit(-1);
	}
  fwrite(s, sizeof(unsigned char), slen, Outfile);
  fclose(Outfile);
  system("./ppmd.exe e -m100 -o10 FileXbzipTmp.dat");

  Infile=fopen("FileXbzipTmp.pmd", "rb"); 
  if (!Infile){
    printf("Error in opening Infile! (DataCompress)");
	exit(-1);
	}
  if(fseek(Infile,0,SEEK_END)!=0) exit(-1);
  *tlen=ftell(Infile);
  *t=malloc(*tlen);   
  if (!(*t)){
    printf("Error in Malloc! (DataCompress)");
	exit(-1);
	}
  rewind(Infile); 
  i=fread(*t, (size_t) 1, (size_t) *tlen, Infile);
  if (i != *tlen){
    printf("Incorrect fread! (DataCompress)");
	exit(-1);
	}
  fclose(Infile);
  system("rm -f FileXbzipTmp.*");
}

void data_decompress(unsigned char *s, int slen, unsigned char **t, int *tlen)
{

  FILE *Outfile;   
  FILE *Infile;    
  int i;


  system("rm -f FileXbzipTmp.*");
  Outfile = fopen( "FileXbzipTmp.pmd", "wb"); 
  if (!Outfile){
    printf("Error in opening Outfile! (DataDeCompress)");
	exit(-1);
	}
  fwrite(s, sizeof(unsigned char), slen, Outfile);
  fclose(Outfile);

  system("./ppmd.exe d FileXbzipTmp.pmd");

  Infile=fopen("FileXbzipTmp.dat", "rb"); 
  if (!Infile){
    printf("Error in opening Infile! (DataDeCompress)");
	exit(-1);
	}
  if(fseek(Infile,0,SEEK_END)!=0) exit(-1);
  *tlen=ftell(Infile);
  *t=malloc(*tlen);   
  if (!(*t)){
    printf("Error in Malloc! (DataDeCompress)");
	exit(-1);
	}
  rewind(Infile); 
  i=fread(*t, (size_t) 1, (size_t) *tlen, Infile);
  if (i != *tlen){
    printf("Incorrect fread! (DataDeCompress)");
	exit(-1);
	}
  fclose(Infile);
  system("rm -f FileXbzipTmp.*");
}
