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

/* xmlunppm.cpp: an XML decompressor */

#include <cstdio>
#include "Args.h"
#include "Model.h"
#include "ScmModel.h"

#ifdef __CYGWIN__
extern "C" char* strdup(const char*);
#endif

#define BUFFSIZE 8192


int
main (int argc, char **argv)
{
  PPM_DECODER * modelo_defecto;
  args_t args = getDecoderArgs( argc, argv );
  readHeader( &args, args.infp );
  struct level_settings s = settings[args.level];

  int c;

  modelo_defecto = new PPM_DECODER( s.chr.size, s.chr.order, args.infp );
  ARI_INIT_DECODER( args.infp );

  // uno a uno...
  while (args.size--)
    { c = modelo_defecto->DecodeChar();
      fputc( c, args.outfp );
    }
    
  /* clean up */
  fclose( args.infp );
  fclose( args.outfp );
  delete modelo_defecto;

  return 0;
}       /* End of main */
