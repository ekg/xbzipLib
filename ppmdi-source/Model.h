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
/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: PPMII model description and encoding/decoding routines        *
 ****************************************************************************/
/* Changes 1/2003 by James Cheney to interface to XMLPPM
 */

#ifndef __MODEL_H__
#define __MODEL_H__

#include <string.h>
#include "PPMdType.h"
#include "Coder.h"
#include "SubAlloc.hpp"

enum
{ UP_FREQ = 5, INT_BITS = 7, PERIOD_BITS = 7, TOT_BITS =
    INT_BITS + PERIOD_BITS,
  INTERVAL = 1 << INT_BITS, BIN_SCALE = 1 << TOT_BITS, MAX_FREQ =
    124, O_BOUND = 9
};

#pragma pack(1)
struct SEE2_CONTEXT
{				// SEE-contexts for PPM-contexts with masked symbols
  WORD Summ;
  BYTE Shift, Count;
  void init (UINT InitVal)
  {
    Summ = InitVal << (Shift = PERIOD_BITS - 4);
    Count = 7;
  }
  UINT getMean ()
  {
    UINT RetVal = (Summ >> Shift);
    Summ -= RetVal;
    return RetVal + (RetVal == 0);
  }
  void update ()
  {
    if (Shift < PERIOD_BITS && --Count == 0)
      {
	Summ += Summ;
	Count = 3 << Shift++;
      }
  }
} _PACK_ATTR;

struct PPM_CONTEXT;

struct STATE
{				
  BYTE Symbol, Freq;		
  PPM_CONTEXT *Successor;	
}  _PACK_ATTR;

#define STATS States.SummStats.Stats
#define SUMMFREQ States.SummStats.SummFreq
#define ONESTATE States.OneState




struct PPM_CONTEXT
{				
  BYTE NumStats, Flags;		
  union {
    struct {
      WORD SummFreq;		//  number of symbols minus 1
      struct STATE *Stats;		//  ABCD    context
    } SummStats;
    struct STATE OneState;
  } States;
  PPM_CONTEXT *Suffix;		//   BCD    suffix
}
_PACK_ATTR;


struct PPM_MODEL
{
  struct PPM_CONTEXT *MaxContext;
  struct SEE2_CONTEXT SEE2Cont[24][32], DummySEE2Cont;
  BYTE NS2BSIndx[256], QTable[260];	// constants
  STATE * FoundState;	// found next state transition
  int InitEsc, OrderFall, RunLength, InitRL, MaxOrder;
  BYTE CharMask[256], NumMasked, PrevSuccess, EscCount, PrintCount;
  WORD BinSumm[25][64];		// binary SEE-contexts
  SubAlloc* sa;
  int DecodedSize;
  PPM_MODEL (int SASize, int MaxOrder);
  ~PPM_MODEL() {sa->StopSubAllocator(); delete sa;}
  void StartModelRare();
  void RestoreModelRare (PPM_CONTEXT * pc1, PPM_CONTEXT * MinContext,
			 PPM_CONTEXT * FSuccessor);

  PPM_CONTEXT * CreateSuccessors (BOOL Skip,
				  STATE * p,
				  PPM_CONTEXT * pc);
  PPM_CONTEXT * ReduceOrder (STATE * p,
			     PPM_CONTEXT * pc);
  void UpdateModel (PPM_CONTEXT * MinContext);
  void ClearMask ();
  void EncodeChar (FILE * EncodedFile,
			    int c);
  void PreloadChar (int c);
  int DecodeChar (FILE * EncodedFile);

  inline void encodeBinSymbol (PPM_CONTEXT * ctxt, int symbol);	//   BCDE   successor
  inline void encodeSymbol1 (PPM_CONTEXT * ctxt, int symbol);	// other orders:
  inline void encodeSymbol2 (PPM_CONTEXT * ctxt, int symbol);	//   BCD    context
  inline void decodeBinSymbol (PPM_CONTEXT * ctxt);	//    CD    suffix
  inline void decodeSymbol1 (PPM_CONTEXT * ctxt);	//   BCDE   successor
  inline void decodeSymbol2 (PPM_CONTEXT * ctxt);
  inline void update1 (PPM_CONTEXT * ctxt, STATE * p);
  inline void update2 (PPM_CONTEXT * ctxt, STATE * p);
  inline SEE2_CONTEXT *makeEscFreq2 (PPM_CONTEXT * ctxt);
  void rescale (PPM_CONTEXT * ctxt);
  void refresh (PPM_CONTEXT * ctxt, int OldNU, BOOL Scale);
  PPM_CONTEXT *cutOff (PPM_CONTEXT * ctxt, int Order);
  PPM_CONTEXT *removeBinConts (PPM_CONTEXT * ctxt, int Order);

}
_PACK_ATTR;

struct PPM_ENCODER : PPM_MODEL {
  FILE* outfile;
  PPM_ENCODER(int SASize, int MaxOrder, FILE* fp) 
    : PPM_MODEL(SASize, MaxOrder),
    outfile(fp) {}
  void EncodeChar (int c) {
    PPM_MODEL::EncodeChar(outfile,c);
  }
};

struct PPM_DECODER : public PPM_MODEL {
  FILE* infile;
  PPM_DECODER(int SASize, int MaxOrder, FILE* fp) 
    : PPM_MODEL(SASize,MaxOrder),
    infile(fp) {}
  int DecodeChar() {
    return PPM_MODEL::DecodeChar(infile);
  }
};

#endif



