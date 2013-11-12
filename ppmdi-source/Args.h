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

#ifndef __ARGS_H__
#define __ARGS_H__

#include <stdio.h>

struct size_order {
  int size;
  int order;
};


extern struct level_settings {
  struct size_order elt;
  struct size_order att;
  struct size_order sym;
  struct size_order chr;
} settings[10];

typedef struct args
{
  char *infile;
  FILE *infp;
  char *outfile;
  FILE *outfp;
  int standalone;
  int level;
  unsigned size;
}
args_t;

args_t getEncoderArgs(int argc, char** argv);
args_t getDecoderArgs(int argc, char** argv);
void writeHeader(args_t * args, FILE* fp);
void readHeader(args_t * args, FILE* fp);
void printArgs(args_t* args);

#endif
