/* XMLPPM: an XML compressor
Copyright (C) 2003 James Cheney

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Contacting the author:
James Cheney
Computer Science Department
Cornell University
Ithaca, NY 14850

jcheney@cs.cornell.edu
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#if defined(WIN32) || defined(__CYGWIN__)
#include <io.h>
#endif
#include "Args.h"
#include "Version.h"

extern "C"
{
#if defined(__CYGWIN__)
  int _setmode (int handle, int mode);
  int _fileno (FILE * stream);
#endif
  char *strdup (const char *);
}


struct level_settings settings[10] = { 
  { {1024,6},{1024,6},{1024,6},{1024,6} },
  { {1024,7},{1024,7},{1024,7},{2048,7} },
  { {1024,8},{1024,8},{1024,8},{3072,8} },
  { {1024,9},{1024,9},{1024,9},{4096,9} },
  { {2048,10},{1024,10},{1024,10},{5120,10} },
  { {2048,12},{1024,12},{1024,12},{6154,10} },
  { {3072,16},{1024,16},{1024,16},{8192,10} },
  { {3072,16},{2048,16},{1024,16},{10240,10} },
  { {4096,16},{2048,16},{1024,16},{15360,10} },
  { {8192,16},{4096,16},{1024,16},{20480,10} },
};


void printVersion () {
  fprintf(stderr, 
	  "XMLPPM version 0.%d\n"
	  "(C) 2003 %s\n", 
	  xmlppm_version,
	  xmlppm_authors);
  exit(0);
}

void encoderUsage () {
  fprintf (stderr, "Usage: xmlppm [-v] [-s] [-l lev] [infile] [outfile]\n");
  exit (-1);
}

void decoderUsage () {
  fprintf (stderr, "Usage: xmlunppm [-v] [infile] [outfile]\n");
  exit (-1);
}



FILE *
fopen_safe (char *filename, char *mode)
{
  FILE *file = fopen64 (filename, mode);
  if (!file)
    {
      fprintf (stderr, "%s: %s could not be opened %s\n", filename,
	       strerror (errno), mode);
      exit (-1);
    }
  return file;
}



args_t
getEncoderArgs (int argc, char **argv)
{
  args_t args = { NULL, NULL, NULL, NULL, -1, -1};
  int version = -1;
  // ignore first arg (executable name)
  argc--;
  argv++;
  
  while(argc > 0) {
    if (strcmp(argv[0],"-s") == 0) {
      // standalone
      if(args.standalone != -1) {
	fprintf(stderr,"duplicate option -s\n");
	encoderUsage();
      }
      args.standalone = 0;
      argv++;
      argc--;
      continue;
    } else if (strcmp (*argv,"-l") == 0) {
      if(args.level != -1) {
	fprintf(stderr,"redundant option -l\n");
	encoderUsage();
      }
      argv++;
      argc--;
      if(argc <= 0) {
	encoderUsage();
      }
      args.level = atoi(*argv);;
      if(args.level < 0 || args.level > 10) 
	{
	  fprintf(stderr,"level must be between 0 and 11");
	  exit(-1);
	}
      argv++;
      argc--;
      continue;
    } else if (strcmp (*argv,"-v") == 0) {
      if(version != -1) {
	encoderUsage();
      }
      version = 1;
      argv++;
      argc--;
      continue;
    }
    break;
  }
  if(args.level == -1) args.level = 6;
  if(version == 1) printVersion();
  switch (argc)
    {
    case 0:
      args.infp = stdin;
      args.outfp = stdout;
#if defined(WIN32) || defined(__CYGWIN__)
      if (_setmode (_fileno (stdout), _O_BINARY) == -1)
	{
	  perror ("Cannot set stdout to binary");
	  exit (-1);
	}
#endif
      break;
    case 1:
      {
	args.infile = strdup (argv[0]);
	args.outfile =
	  (char *) malloc (strlen (argv[0]) + strlen (xmlppm_suffix) + 1);
	strcpy (args.outfile, argv[0]);
	strcat (args.outfile, xmlppm_suffix);
	args.infp = fopen_safe (args.infile, "r");
	args.outfp = fopen_safe (args.outfile, "wb");
	break;
      }
    case 2:
      args.infile = strdup (argv[0]);
      args.outfile = strdup (argv[1]);
      args.infp = fopen_safe (argv[0], "r");
      args.outfp = fopen_safe (argv[1], "wb");
      break;
    default:
      encoderUsage();
    }
  return args;
}

void writeHeader(args_t* args, FILE* fp) {
  struct stat64 buf;
  unsigned size;
  stat64 (args->infile,&buf);
  fwrite(xmlppm_magic,sizeof(char),4,fp);
  fputc(0,fp); /* spacer */
  size = (unsigned) buf.st_size;
  fwrite(&size,sizeof(unsigned),1,fp);
  fputc(args->standalone,fp); /* 1 bit for standalone */
  fputc(xmlppm_version,fp); /* identifies encoder major version */
  fputc(args->level,fp) ; /* 0-9 compression level, 4 bits available */
  if(args->infile != NULL) {
    fwrite(args->infile, sizeof(char), strlen(args->infile), fp);
  }
  fputc(0,fp);
}



args_t
getDecoderArgs (int argc, char **argv)
{
  args_t args = { NULL, NULL, NULL, NULL, -1, -1};
  int version = -1;
  while(argc > 0) {
    if (strcmp (*argv,"-v") == 0) {
      if(version != -1) {
	encoderUsage();
      }
      version = 1;
      continue;
    }
    break;
  }
  if(version == 1) printVersion();
  switch (argc)
    {
    case 1:
      args.infp = stdin;
      args.outfp = stdout;
#if defined(WIN32) || defined(__CYGWIN__)
      if (_setmode (_fileno (stdin), _O_BINARY) == -1)
	{
	  perror ("Cannot set stdin to binary");
	  exit (-1);
	}
#endif
      break;

    case 2:
      {
	args.infile = strdup (argv[1]);
	args.infp = fopen_safe (args.infile, "rb");
	break;
      }
    case 3:
      args.infile = strdup (argv[1]);
      args.outfile = strdup (argv[2]);
      args.infp = fopen_safe (argv[1], "rb");
      args.outfp = fopen_safe (argv[2], "w");
      break;
    default:
      decoderUsage();
    }
  return args;
}

void readHeader(args_t *args, FILE* fp) {
  static char buf[1000];
  int i, version;
  fread(buf,sizeof(char),4,fp);
  buf[4] = 0;
  if(strcmp(buf,xmlppm_magic) != 0) {
    fprintf(stderr,"Input file is not an XMLPPM-compressed file\n");
    exit(-1);
  }
  fgetc(fp); /* spacer */
  fread(&args->size,sizeof(unsigned),1,fp);
  args->standalone = fgetc(fp) & 0x1; /* 1 bit for standalone */
  version = fgetc(fp); /* identifies encoder major version */
  if(version != xmlppm_version) {
    fprintf(stderr,
	    "Input file is compressed with an incompatible version %d.%d of XMLPPM\n"
	    "(this is version %d.%d)\n", 
	    version / 100, version % 100,
	    xmlppm_version / 100, xmlppm_version % 100);
    exit(-1);
  }
  args->level = fgetc(fp) ; /* 0-9 compression level, 4 bits available */
  i = 0;
  while( (buf[i++] = fgetc(fp)) != '\0' ) {
    if(buf[i] == EOF) {
      fprintf(stderr,"EOF encountered in XMLPPM file header\n");
    }
  }
  if(args->outfile == NULL) { /* use name from command line if given */
    if(buf[0] != '\0') { 
      /* if it was saved and no override specified, use it */
      args->outfile = strdup(buf);
      args->outfp = fopen_safe (args->outfile, "wb");
    } else {   /* Try to do something reasonable if no filename was saved */
      if(args->infile != NULL) {
	/* find suffix */
	char* ptr = strrchr (args->infile, '.');
	if (ptr == NULL || strcmp (ptr, xmlppm_suffix) != 0) {
	  fprintf (stderr, "Input file %s does not have the %s suffix, decompressing to standard output\n",
		   args->infile, xmlppm_suffix);
	  args->outfile = strdup("<STDOUT>");
	  args->outfp = stdout;
	  return;
	} else {
	  *ptr = '\0'; /* truncate infile */
	  args->outfile = (char *) malloc (strlen (args->infile) + 1);
	  strcpy (args->outfile, args->infile);
	  *ptr = '.'; /* restore infile */
	  args->outfp = fopen_safe (args->outfile, "w");
	}
      } else { /* args.infile == NULL */
	args->infile = strdup("<STDIN>");
	args->outfile = strdup("<STDOUT>");
      }
    }
  }
}
void printArgs(args_t * args) {
  fprintf(stderr,"Input: %s\nOutput: %s\nStandalone: %s\nLevel: %d\n",
	  args->infile, args->outfile, args->standalone? "true" : "false",
	  args->level);
	  
}
