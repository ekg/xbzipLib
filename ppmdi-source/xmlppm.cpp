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

/* xmlppm.cpp: an XML compressor */

#include <cstdio>
#include "Args.h"
#include "Model.h"
#include "SubAlloc.hpp"
#include "ScmModel.h"

//#define BUFFSIZE	8192
//char Buff[BUFFSIZE];

int
main (int argc, char **argv)
{
  /* set up the encoder */
  PPM_ENCODER * modelo_defecto;
  args_t args = getEncoderArgs( argc, argv );
  writeHeader( &args, args.outfp );
  struct level_settings s = settings[args.level];
  unsigned char c;

  modelo_defecto = new PPM_ENCODER( s.chr.size, s.chr.order, args.outfp );
  
  // compress the file
  ariInitEncoder( args.outfp );
  
// --- Opción 1 ---
// --- Leer caracter a caracter ---

   c = fgetc( args.infp );
   
   while( !feof(args.infp) )
   {
        modelo_defecto->EncodeChar( c );
        
        c = fgetc( args.infp );
   }

// --- Opción 2 ---
// --- Leer bloque a bloque. Revisar ---
/*
  for(;;)
  {      
      len = fread( Buff, 1, BUFFSIZE, args.infp );
              
      if( ferror(args.infp) )
      {
            fprintf (stderr, "Read error\n");
            exit (-1);
      }
        
      done = feof( args.infp );

      for( int i = 0; i < len; i++ )
            modelo_defecto->EncodeChar( Buff[i] );
      
      if( done ) break;  
  }
*/

  // end of compression proccess
  // modelo_defecto->EncodeChar( ENDFILE );
  ARI_FLUSH_ENCODER (args.outfp);

  /* clean up */
  fclose (args.infp);
  fclose (args.outfp);
  delete modelo_defecto;
  
  return 0;
}
