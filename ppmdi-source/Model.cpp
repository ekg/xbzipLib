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
#include "stdlib.h"
#include "Model.h"
#include "SubAlloc.hpp"

inline void
SWAP (STATE & s1, STATE & s2)
{
  WORD t1 = (WORD &) s1;
  PPM_CONTEXT *t2 = s1.Successor;
  (WORD &) s1 = (WORD &) s2;
  s1.Successor = s2.Successor;
  (WORD &) s2 = t1;
  s2.Successor = t2;
}
inline void
StateCpy (STATE & s1, const STATE & s2)
{
  (WORD &) s1 = (WORD &) s2;
  s1.Successor = s2.Successor;
}

void
PPM_MODEL::refresh (PPM_CONTEXT * ctxt, int OldNU, BOOL Scale)
{
  int i = ctxt->NumStats, EscFreq;
  STATE *p = ctxt->STATS = (STATE *) sa->ShrinkUnits (ctxt->STATS, OldNU, (i + 2) >> 1);
  ctxt->Flags = (ctxt->Flags & (0x10 + 0x04 * Scale)) + 0x08 * (p->Symbol >= 0x40);
  EscFreq = ctxt->SUMMFREQ - p->Freq;
  ctxt->SUMMFREQ = (p->Freq = (p->Freq + Scale) >> Scale);
  do
    {
      EscFreq -= (++p)->Freq;
      ctxt->SUMMFREQ += (p->Freq = (p->Freq + Scale) >> Scale);
      ctxt->Flags |= 0x08 * (p->Symbol >= 0x40);
    }
  while (--i);
  ctxt->SUMMFREQ += (EscFreq = (EscFreq + Scale) >> Scale);
} 

#define P_CALL(F,C) (p->Successor=F(p->Successor,Order+1))

inline 
PPM_CONTEXT *
PPM_MODEL::cutOff (PPM_CONTEXT * ctxt, int Order)
{
  int i, tmp;
  STATE *p;
  if (!ctxt->NumStats)
    {
      if ((BYTE *) (p = &ctxt->ONESTATE)->Successor >= sa->UnitsStart)
	{
	  if (Order < MaxOrder)
	    P_CALL (cutOff,ctxt);
	  else
	    p->Successor = NULL;
	  if (!p->Successor && Order > O_BOUND)
	    goto REMOVE;
	  return ctxt;
	}
      else
	{
	REMOVE:sa->SpecialFreeUnit (this);
	  return NULL;
	}
    }
  ctxt->STATS = (STATE *) sa->MoveUnitsUp (ctxt->STATS, tmp = (ctxt->NumStats + 2) >> 1);
  for (p = ctxt->STATS + (i = ctxt->NumStats); p >= ctxt->STATS; p--)
    if ((BYTE *) p->Successor < sa->UnitsStart)
      {
	p->Successor = NULL;
	SWAP (*p, ctxt->STATS[i--]);
      }
    else if (Order < MaxOrder)
      P_CALL (cutOff,ctxt);
    else
      p->Successor = NULL;
  if (i != ctxt->NumStats && Order)
    {
      ctxt->NumStats = i;
      p = ctxt->STATS;
      if (i < 0)
	{
	  sa->FreeUnits (p, tmp);
	  goto REMOVE;
	}
      else if (i == 0)
	{
	  ctxt->Flags = (ctxt->Flags & 0x10) + 0x08 * (p->Symbol >= 0x40);
	  StateCpy (ctxt->ONESTATE, *p);
	  sa->FreeUnits (p, tmp);
	  ctxt->ONESTATE.Freq = (ctxt->ONESTATE.Freq + 11) >> 3;
	}
      else
	refresh (ctxt, tmp, ctxt->SUMMFREQ > 16 * i);
    }
  return ctxt;
}

PPM_CONTEXT *
PPM_MODEL::removeBinConts (PPM_CONTEXT * ctxt, int Order)
{
  STATE *p;
  if (!ctxt->NumStats)
    {
      p = &ctxt->ONESTATE;
      if ((BYTE *) p->Successor >= sa->UnitsStart && Order < MaxOrder)
	P_CALL (removeBinConts,ctxt);
      else
	p->Successor = NULL;
      if (!p->Successor && (!ctxt->Suffix->NumStats || ctxt->Suffix->Flags == 0xFF))
	{
	  sa->FreeUnits (this, 1);
	  return NULL;
	}
      else
	return ctxt;
    }
  for (p = ctxt->STATS + ctxt->NumStats; p >= ctxt->STATS; p--)
    if ((BYTE *) p->Successor >= sa->UnitsStart && Order < MaxOrder)
      P_CALL (removeBinConts,ctxt);
    else
      p->Successor = NULL;
  return ctxt;
}

void
PPM_MODEL::rescale (PPM_CONTEXT * ctxt)
{
  UINT OldNU, Adder, EscFreq, i = ctxt->NumStats;
  STATE tmp, *p1, *p;
  for (p = FoundState; p != ctxt->STATS; p--)
    SWAP (p[0], p[-1]);
  p->Freq += 4;
  ctxt->SUMMFREQ += 4;
  EscFreq = ctxt->SUMMFREQ - p->Freq;
  Adder = (OrderFall != 0);
  ctxt->SUMMFREQ = (p->Freq = (p->Freq + Adder) >> 1);
  do
    {
      EscFreq -= (++p)->Freq;
      ctxt->SUMMFREQ += (p->Freq = (p->Freq + Adder) >> 1);
      if (p[0].Freq > p[-1].Freq)
	{
	  StateCpy (tmp, *(p1 = p));
	  do
	    StateCpy (p1[0], p1[-1]);
	  while (tmp.Freq > (--p1)[-1].Freq);
	  StateCpy (*p1, tmp);
	}
    }
  while (--i);
  if (p->Freq == 0)
    {
      do
	{
	  i++;
	}
      while ((--p)->Freq == 0);
      EscFreq += i;
      OldNU = (ctxt->NumStats + 2) >> 1;
      if ((ctxt->NumStats -= i) == 0)
	{
	  StateCpy (tmp, *ctxt->STATS);
	  tmp.Freq = (2 * tmp.Freq + EscFreq - 1) / EscFreq;
	  if (tmp.Freq > MAX_FREQ / 3)
	    tmp.Freq = MAX_FREQ / 3;
	  sa->FreeUnits (ctxt->STATS, OldNU);
	  StateCpy (ctxt->ONESTATE, tmp);
	  ctxt->Flags = (ctxt->Flags & 0x10) + 0x08 * (tmp.Symbol >= 0x40);
	  FoundState = &ctxt->ONESTATE;
	  return;
	}
      ctxt->STATS = (STATE *) sa->ShrinkUnits (ctxt->STATS, OldNU, (ctxt->NumStats + 2) >> 1);
      ctxt->Flags &= ~0x08;
      i = ctxt->NumStats;
      ctxt->Flags |= 0x08 * ((p = ctxt->STATS)->Symbol >= 0x40);
      do
	{
	  ctxt->Flags |= 0x08 * ((++p)->Symbol >= 0x40);
	}
      while (--i);
    }
  ctxt->SUMMFREQ += (EscFreq -= (EscFreq >> 1));
  ctxt->Flags |= 0x04;
  FoundState = ctxt->STATS;
}

// Tabulated escapes for exponential symbol distribution
static const BYTE ExpEscape[16] =
  { 25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))
inline void
PPM_MODEL::encodeBinSymbol (PPM_CONTEXT * ctxt, int symbol)
{
  BYTE indx = NS2BSIndx[ctxt->Suffix->NumStats] + PrevSuccess + ctxt->Flags;
  STATE & rs = ctxt->ONESTATE;
  WORD & bs =
    BinSumm[QTable[rs.Freq - 1]][indx +
					   ((RunLength >> 26) & 0x20)];
  if (rs.Symbol == symbol)
    {
      FoundState = &rs;
      rs.Freq += (rs.Freq < 196);
      SubRange.LowCount = 0;
      SubRange.HighCount = bs;
      bs += INTERVAL - GET_MEAN (bs, PERIOD_BITS, 2);
      PrevSuccess = 1;
      RunLength++;
    }
  else
    {
      SubRange.LowCount = bs;
      bs -= GET_MEAN (bs, PERIOD_BITS, 2);
      SubRange.HighCount = BIN_SCALE;
      InitEsc = ExpEscape[bs >> 10];
      CharMask[rs.Symbol] = EscCount;
      NumMasked = PrevSuccess = 0;
      FoundState = NULL;
    }
}
inline void
PPM_MODEL::decodeBinSymbol (PPM_CONTEXT * ctxt)
{
  BYTE indx = NS2BSIndx[ctxt->Suffix->NumStats] + PrevSuccess + ctxt->Flags;
  STATE & rs = ctxt->ONESTATE;
  WORD & bs =
    BinSumm[QTable[rs.Freq - 1]][indx +
					   ((RunLength >> 26) & 0x20)];
  if (ariGetCurrentShiftCount (TOT_BITS) < bs)
    {
      FoundState = &rs;
      rs.Freq += (rs.Freq < 196);
      SubRange.LowCount = 0;
      SubRange.HighCount = bs;
      bs += INTERVAL - GET_MEAN (bs, PERIOD_BITS, 2);
      PrevSuccess = 1;
      RunLength++;
    }
  else
    {
      SubRange.LowCount = bs;
      bs -= GET_MEAN (bs, PERIOD_BITS, 2);
      SubRange.HighCount = BIN_SCALE;
      InitEsc = ExpEscape[bs >> 10];
      CharMask[rs.Symbol] = EscCount;
      NumMasked = PrevSuccess = 0;
      FoundState = NULL;
    }
}
inline void
PPM_MODEL::update1 (PPM_CONTEXT * ctxt, STATE * p)
{
  (FoundState = p)->Freq += 4;
  ctxt->SUMMFREQ += 4;
  if (p[0].Freq > p[-1].Freq)
    {
      SWAP (p[0], p[-1]);
      FoundState = --p;
      if (p->Freq > MAX_FREQ)
	rescale (ctxt);
    }
}
inline void
PPM_MODEL::encodeSymbol1 (PPM_CONTEXT * ctxt, int symbol)
{
  UINT LoCnt;
  int i = ctxt->STATS->Symbol;
  STATE *p = ctxt->STATS;
  SubRange.scale = ctxt->SUMMFREQ;
  if (i == symbol)
    {
      PrevSuccess =
	(2 * (SubRange.HighCount = p->Freq) >= SubRange.scale);
      (FoundState = p)->Freq += 4;
      ctxt->SUMMFREQ += 4;
      RunLength += PrevSuccess;
      if (p->Freq > MAX_FREQ)
	rescale (ctxt);
      SubRange.LowCount = 0;
      return;
    }
  LoCnt = p->Freq;
  i = ctxt->NumStats;
  PrevSuccess = 0;
  while ((++p)->Symbol != symbol)
    {
      LoCnt += p->Freq;
      if (--i == 0)
	{
	  SubRange.LowCount = LoCnt;
	  CharMask[p->Symbol] = EscCount;
	  i = NumMasked = ctxt->NumStats;
	  FoundState = NULL;
	  do
	    {
	      CharMask[(--p)->Symbol] = EscCount;
	    }
	  while (--i);
	  SubRange.HighCount = SubRange.scale;
	  return;
	}
    }
  SubRange.HighCount = (SubRange.LowCount = LoCnt) + p->Freq;
  update1 (ctxt,p);
}
inline void
PPM_MODEL::decodeSymbol1 (PPM_CONTEXT * ctxt)
{
  UINT i, count, HiCnt = ctxt->STATS->Freq;
  STATE *p = ctxt->STATS;
  SubRange.scale = ctxt->SUMMFREQ;
  if ((count = ariGetCurrentCount ()) < HiCnt)
    {
      PrevSuccess = (2 * (SubRange.HighCount = HiCnt) >= SubRange.scale);
      (FoundState = p)->Freq = (HiCnt += 4);
      ctxt->SUMMFREQ += 4;
      RunLength += PrevSuccess;
      if (HiCnt > MAX_FREQ)
	rescale (ctxt);
      SubRange.LowCount = 0;
      return;
    }
  i = ctxt->NumStats;
  PrevSuccess = 0;
  while ((HiCnt += (++p)->Freq) <= count)
    if (--i == 0)
      {
	SubRange.LowCount = HiCnt;
	CharMask[p->Symbol] = EscCount;
	i = NumMasked = ctxt->NumStats;
	FoundState = NULL;
	do
	  {
	    CharMask[(--p)->Symbol] = EscCount;
	  }
	while (--i);
	SubRange.HighCount = SubRange.scale;
	return;
      }
  SubRange.LowCount = (SubRange.HighCount = HiCnt) - p->Freq;
  update1 (ctxt, p);
}
inline void
PPM_MODEL::update2 (PPM_CONTEXT * ctxt, STATE * p)
{
  (FoundState = p)->Freq += 4;
  ctxt->SUMMFREQ += 4;
  if (p->Freq > MAX_FREQ)
    rescale (ctxt);
  EscCount++;
  RunLength = InitRL;
}
inline SEE2_CONTEXT *
PPM_MODEL::makeEscFreq2 (PPM_CONTEXT * ctxt)
{
  int t = 2 * ctxt->NumStats;
  SEE2_CONTEXT *psee2c;
  if (ctxt->NumStats != 0xFF)
    {
      t = ctxt->Suffix->NumStats;
      psee2c =
	SEE2Cont[QTable[ctxt->NumStats + 2] - 3] + (ctxt->SUMMFREQ >
							11 * (ctxt->NumStats + 1));
      psee2c += 2 * (2 * ctxt->NumStats < t + NumMasked) + ctxt->Flags;
      SubRange.scale = psee2c->getMean ();
    }
  else
    {
      psee2c = &DummySEE2Cont;
      SubRange.scale = 1;
    }
  return psee2c;
}
inline void
PPM_MODEL::encodeSymbol2 (PPM_CONTEXT * ctxt, int symbol)
{
  SEE2_CONTEXT *psee2c = makeEscFreq2 (ctxt);
  int Sym;
  UINT LoCnt = 0, i = ctxt->NumStats - NumMasked;
  STATE *p1, *p = ctxt->STATS - 1;
  do
    {
      do
	{
	  Sym = p[1].Symbol;
	  p++;
	}
      while (CharMask[Sym] == EscCount);
      CharMask[Sym] = EscCount;
      if (Sym == symbol)
	goto SYMBOL_FOUND;
      LoCnt += p->Freq;
    }
  while (--i);
  SubRange.HighCount = (SubRange.scale += (SubRange.LowCount = LoCnt));
  psee2c->Summ += SubRange.scale;
  NumMasked = ctxt->NumStats;
  return;
SYMBOL_FOUND:
  SubRange.LowCount = LoCnt;
  SubRange.HighCount = (LoCnt += p->Freq);
  for (p1 = p; --i;)
    {
      do
	{
	  Sym = p1[1].Symbol;
	  p1++;
	}
      while (CharMask[Sym] == EscCount);
      LoCnt += p1->Freq;
    }
  SubRange.scale += LoCnt;
  psee2c->update ();
  update2 (ctxt, p);
}
inline void
PPM_MODEL::decodeSymbol2 (PPM_CONTEXT * ctxt)
{
  SEE2_CONTEXT *psee2c = makeEscFreq2 (ctxt);
  UINT Sym, count, HiCnt = 0, i = ctxt->NumStats - NumMasked;
  STATE *ps[256], **pps = ps, *p = ctxt->STATS - 1;
  do
    {
      do
	{
	  Sym = p[1].Symbol;
	  p++;
	}
      while (CharMask[Sym] == EscCount);
      HiCnt += p->Freq;
      *pps++ = p;
    }
  while (--i);
  SubRange.scale += HiCnt;
  count = ariGetCurrentCount ();
  p = *(pps = ps);
  if (count < HiCnt)
    {
      HiCnt = 0;
      while ((HiCnt += p->Freq) <= count)
	p = *++pps;
      SubRange.LowCount = (SubRange.HighCount = HiCnt) - p->Freq;
      psee2c->update ();
      update2 (ctxt, p);
    }
  else
    {
      SubRange.LowCount = HiCnt;
      SubRange.HighCount = SubRange.scale;
      i = ctxt->NumStats - NumMasked;
      NumMasked = ctxt->NumStats;
      do
	{
	  CharMask[(*pps)->Symbol] = EscCount;
	  pps++;
	}
      while (--i);
      psee2c->Summ += SubRange.scale;
    }
}

/* PPM_MODEL */


PPM_MODEL::PPM_MODEL(int SASize, int MaxOrder) {
  UINT i, k, m, Step;

  sa = new SubAlloc();
  if (!sa || !sa->StartSubAllocator (SASize))
    {
      fprintf (stderr, "Out of memory!\n");
      exit (-1);
    }

  NS2BSIndx[0] = 2 * 0;
  NS2BSIndx[1] = 2 * 1;
  memset (NS2BSIndx + 2, 2 * 2, 9);
  memset (NS2BSIndx + 11, 2 * 3, 256 - 11);
  for (i = 0; i < UP_FREQ; i++)
    QTable[i] = i;
  for (m = i = UP_FREQ, k = Step = 1; i < 260; i++)
    {
      QTable[i] = m;
      if (!--k)
	{
	  k = ++Step;
	  m++;
	}
    }
  (DWORD &) DummySEE2Cont = PPMdSignature;
  this->MaxOrder = MaxOrder;
  StartModelRare();  
}

void
PPM_MODEL::StartModelRare ()
{
  UINT i, k, m;
  memset (CharMask, 0, sizeof (CharMask));
  EscCount = PrintCount = 1;  
  OrderFall = this->MaxOrder = MaxOrder;
  sa->InitSubAllocator ();
  RunLength = InitRL = -((MaxOrder < 12) ? MaxOrder : 12) - 1;
  MaxContext = (PPM_CONTEXT *) sa->AllocContext ();
  MaxContext->Suffix = NULL;
  MaxContext->SUMMFREQ = (MaxContext->NumStats = 255) + 2;
  MaxContext->STATS = (STATE *) sa->AllocUnits (256 / 2);
  for (PrevSuccess = i = 0; i < 256; i++)
    {
      MaxContext->STATS[i].Symbol = i;
      MaxContext->STATS[i].Freq = 1;
      MaxContext->STATS[i].Successor = NULL;
    }
  static const WORD InitBinEsc[] =
    { 0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051 };
  for (i = m = 0; m < 25; m++)
    {
      while (QTable[i] == m)
	i++;
      for (k = 0; k < 8; k++)
	BinSumm[m][k] = BIN_SCALE - InitBinEsc[k] / (i + 1);
      for (k = 8; k < 64; k += 8)
	memcpy (BinSumm[m] + k, BinSumm[m], 8 * sizeof (WORD));
    }
  for (i = m = 0; m < 24; m++)
    {
      while (QTable[i + 3] == m + 3)
	i++;
      SEE2Cont[m][0].init (2 * i + 5);
      for (k = 1; k < 32; k++)
	SEE2Cont[m][k] = SEE2Cont[m][0];
    }
}

void
PPM_MODEL::RestoreModelRare (PPM_CONTEXT * pc1,
			     PPM_CONTEXT * MinContext,
			     PPM_CONTEXT * FSuccessor)
{
  PPM_CONTEXT *pc;
  STATE * p;
  for (pc = MaxContext, sa->pText = sa->HeapStart; pc != pc1; pc = pc->Suffix)
    if (--(pc->NumStats) == 0)
      {
	pc->Flags = (pc->Flags & 0x10) + 0x08 * (pc->STATS->Symbol >= 0x40);
	p = pc->STATS;
	StateCpy (pc->ONESTATE, *p);
	sa->SpecialFreeUnit (p);
	pc->ONESTATE.Freq = (pc->ONESTATE.Freq + 11) >> 3;
      }
    else
      refresh (pc, (pc->NumStats + 3) >> 1, FALSE);
  for (; pc != MinContext; pc = pc->Suffix)
    if (!pc->NumStats)
      pc->ONESTATE.Freq -= pc->ONESTATE.Freq >> 1;
    else if ((pc->SUMMFREQ += 4) > 128 + 4 * pc->NumStats)
      refresh (pc, (pc->NumStats + 2) >> 1, TRUE);
  
  StartModelRare ();
  EscCount = 0;
  PrintCount = 0xFF;
}

PPM_CONTEXT *
PPM_MODEL::ReduceOrder (STATE * p, PPM_CONTEXT * pc)
{
  STATE * p1, *ps[MAX_O], **pps = ps;
  PPM_CONTEXT *pc1 = pc, *UpBranch = (PPM_CONTEXT *) sa->pText;
  BYTE tmp, sym = FoundState->Symbol;
  *pps++ = FoundState;
  FoundState->Successor = UpBranch;
  OrderFall++;
  if (p)
    {
      pc = pc->Suffix;
      goto LOOP_ENTRY;
    }
  for (;;)
    {
      if (!pc->Suffix)
	{
	  return pc;
	}
      pc = pc->Suffix;
      if (pc->NumStats)
	{
	  if ((p = pc->STATS)->Symbol != sym)
	    do
	      {
		tmp = p[1].Symbol;
		p++;
	      }
	    while (tmp != sym);
	  tmp = 2 * (p->Freq < MAX_FREQ - 9);
	  p->Freq += tmp;
	  pc->SUMMFREQ += tmp;
	}
      else
	{
	  p = &(pc->ONESTATE);
	  p->Freq += (p->Freq < 32);
	}
    LOOP_ENTRY:
      if (p->Successor)
	break;
      *pps++ = p;
      p->Successor = UpBranch;
      OrderFall++;
    }
  if (p->Successor <= UpBranch)
    {
      p1 = FoundState;
      FoundState = p;
      p->Successor = CreateSuccessors (FALSE, NULL, pc);
      FoundState = p1;
    }
  if (OrderFall == 1 && pc1 == MaxContext)
    {
      FoundState->Successor = p->Successor;
      sa->pText--;
    }
  return p->Successor;
}

PPM_CONTEXT *
PPM_MODEL::CreateSuccessors (BOOL Skip, STATE * p,
			     PPM_CONTEXT * pc)
{
  PPM_CONTEXT ct, *UpBranch = FoundState->Successor;
  STATE * ps[MAX_O], **pps = ps;
  UINT cf, s0;
  BYTE tmp, sym = FoundState->Symbol;
  if (!Skip)
    {
      *pps++ = FoundState;
      if (!pc->Suffix)
	goto NO_LOOP;
    }
  if (p)
    {
      pc = pc->Suffix;
      goto LOOP_ENTRY;
    }
  do
    {
      pc = pc->Suffix;
      if (pc->NumStats)
	{
	  if ((p = pc->STATS)->Symbol != sym)
	    do
	      {
		tmp = p[1].Symbol;
		p++;
	      }
	    while (tmp != sym);
	  tmp = (p->Freq < MAX_FREQ - 9);
	  p->Freq += tmp;
	  pc->SUMMFREQ += tmp;
	}
      else
	{
	  p = &(pc->ONESTATE);
	  p->Freq += (!pc->Suffix->NumStats & (p->Freq < 24));
	}
    LOOP_ENTRY:
      if (p->Successor != UpBranch)
	{
	  pc = p->Successor;
	  break;
	}
      *pps++ = p;
    }
  while (pc->Suffix);
NO_LOOP:
  if (pps == ps)
    return pc;
  ct.NumStats = 0;
  ct.Flags = 0x10 * (sym >= 0x40);
  ct.ONESTATE.Symbol = sym = *(BYTE *) UpBranch;
  ct.ONESTATE.Successor = (PPM_CONTEXT *) (((BYTE *) UpBranch) + 1);
  ct.Flags |= 0x08 * (sym >= 0x40);
  if (pc->NumStats)
    {
      if ((p = pc->STATS)->Symbol != sym)
	do
	  {
	    tmp = p[1].Symbol;
	    p++;
	  }
	while (tmp != sym);
      s0 = pc->SUMMFREQ - pc->NumStats - (cf = p->Freq - 1);
      ct.ONESTATE.Freq =
	1 + ((2 * cf <= s0) ? (5 * cf > s0) : ((cf + 2 * s0 - 3) / s0));
    }
  else
    ct.ONESTATE.Freq = pc->ONESTATE.Freq;
  do
    {
      PPM_CONTEXT *pc1 = (PPM_CONTEXT *) sa->AllocContext ();
      if (!pc1)
	return NULL;
      ((DWORD *) pc1)[0] = ((DWORD *) & ct)[0];
      ((DWORD *) pc1)[1] = ((DWORD *) & ct)[1];
      pc1->Suffix = pc;
      (*--pps)->Successor = pc = pc1;
    }
  while (pps != ps);
  return pc;
}

inline void
PPM_MODEL::UpdateModel (PPM_CONTEXT * MinContext)
{
  STATE * p = NULL;
  PPM_CONTEXT *Successor, *FSuccessor, *pc, *pc1 = MaxContext;
  UINT ns1, ns, cf, sf, s0, FFreq = FoundState->Freq;
  BYTE Flag, sym, FSymbol = FoundState->Symbol;
  FSuccessor = FoundState->Successor;
  pc = MinContext->Suffix;
  if (FFreq < MAX_FREQ / 4 && pc)
    {
      if (pc->NumStats)
	{
	  if ((p = pc->STATS)->Symbol != FSymbol)
	    {
	      do
		{
		  sym = p[1].Symbol;
		  p++;
		}
	      while (sym != FSymbol);
	      if (p[0].Freq >= p[-1].Freq)
		{
		  SWAP (p[0], p[-1]);
		  p--;
		}
	    }
	  cf = 2 * (p->Freq < MAX_FREQ - 9);
	  p->Freq += cf;
	  pc->SUMMFREQ += cf;
	}
      else
	{
	  p = &(pc->ONESTATE);
	  p->Freq += (p->Freq < 32);
	}
    }
  if (!OrderFall && FSuccessor)
    {
      FoundState->Successor = CreateSuccessors (TRUE, p, MinContext);
      if (!FoundState->Successor)
	goto RESTART_MODEL;
      MaxContext = FoundState->Successor;
      return;
    }
  *sa->pText++ = FSymbol;
  Successor = (PPM_CONTEXT *) sa->pText;
  if (sa->pText >= sa->UnitsStart)
    goto RESTART_MODEL;
  if (FSuccessor)
    {
      if ((BYTE *) FSuccessor < sa->UnitsStart)
	FSuccessor = CreateSuccessors (FALSE, p, MinContext);
    }
  else
    FSuccessor = ReduceOrder (p, MinContext);
  if (!FSuccessor)
    goto RESTART_MODEL;
  if (!--OrderFall)
    {
      Successor = FSuccessor;
      sa->pText -= (MaxContext != MinContext);
    }
  s0 = MinContext->SUMMFREQ - (ns = MinContext->NumStats) - FFreq;
  for (Flag = 0x08 * (FSymbol >= 0x40); pc1 != MinContext; pc1 = pc1->Suffix)
    {
      if ((ns1 = pc1->NumStats) != 0)
	{
	  if ((ns1 & 1) != 0)
	    {
	      p = (STATE *) sa->ExpandUnits (pc1->STATS,
						      (ns1 + 1) >> 1);
	      if (!p)
		goto RESTART_MODEL;
	      pc1->STATS = p;
	    }
	  pc1->SUMMFREQ += (3 * ns1 + 1 < ns);
	}
      else
	{
	  p = (STATE *) sa->AllocUnits (1);
	  if (!p)
	    goto RESTART_MODEL;
	  StateCpy (*p, pc1->ONESTATE);
	  pc1->STATS = p;
	  if (p->Freq < MAX_FREQ / 4 - 1)
	    p->Freq += p->Freq;
	  else
	    p->Freq = MAX_FREQ - 4;
	  pc1->SUMMFREQ = p->Freq + InitEsc + (ns > 2);
	}
      cf = 2 * FFreq * (pc1->SUMMFREQ + 6);
      sf = s0 + pc1->SUMMFREQ;
      if (cf < 6 * sf)
	{
	  cf = 1 + (cf > sf) + (cf >= 4 * sf);
	  pc1->SUMMFREQ += 4;
	}
      else
	{
	  cf = 4 + (cf > 9 * sf) + (cf > 12 * sf) + (cf > 15 * sf);
	  pc1->SUMMFREQ += cf;
	}
      p = pc1->STATS + (++pc1->NumStats);
      p->Successor = Successor;
      p->Symbol = FSymbol;
      p->Freq = cf;
      pc1->Flags |= Flag;
    }
  MaxContext = FSuccessor;
  return;
RESTART_MODEL:
  RestoreModelRare (pc1, MinContext, FSuccessor);
}


inline void
PPM_MODEL::ClearMask ()
{
  EscCount = 1;
  memset (CharMask, 0, sizeof (CharMask));
}

void PPM_MODEL::EncodeChar(FILE * EncodedFile, 
				    int c) 
{
  PPM_CONTEXT* MinContext;
  BYTE ns = (MinContext = MaxContext)->NumStats;
  if (ns)
    {
      encodeSymbol1 (MinContext, c);
      ariEncodeSymbol ();
    }
  else
    {
      encodeBinSymbol (MinContext, c);
      ariShiftEncodeSymbol (TOT_BITS);
    }
  while (!FoundState)
    {
      ARI_ENC_NORMALIZE (EncodedFile);
      do
	{
	  OrderFall++;
	  MinContext = MinContext->Suffix;
	  if (!MinContext)
	    return;
	}
      while (MinContext->NumStats == NumMasked);
      encodeSymbol2 (MinContext, c);
      ariEncodeSymbol ();
    }
  if (!OrderFall && (BYTE *) FoundState->Successor >= sa->UnitsStart)
    MaxContext = FoundState->Successor;
  else
    {
      UpdateModel (MinContext);
      if (EscCount == 0)
	ClearMask ();
    }
  ARI_ENC_NORMALIZE (EncodedFile);
  DecodedSize++;
}

/* should be identical to EncodeChar except without any encoding actions */
void PPM_MODEL::PreloadChar(int c) 
{
  PPM_CONTEXT* MinContext;
  BYTE ns = (MinContext = MaxContext)->NumStats;
  if (ns)
    {
      encodeSymbol1 (MinContext, c);
    }
  else
    {
      encodeBinSymbol (MinContext, c);
    }
  while (!FoundState)
    {
      do
	{
	  OrderFall++;
	  MinContext = MinContext->Suffix;
	  if (!MinContext)
	    return;
	}
      while (MinContext->NumStats == NumMasked);
      encodeSymbol2 (MinContext, c);
    }
  if (!OrderFall && (BYTE *) FoundState->Successor >= sa->UnitsStart)
    MaxContext = FoundState->Successor;
  else
    {
      UpdateModel (MinContext);
      if (EscCount == 0)
	ClearMask ();
    }
}

int PPM_MODEL::DecodeChar(FILE * EncodedFile) 
{
  PPM_CONTEXT *MinContext = MaxContext;
  BYTE ns = MinContext->NumStats;
  int c;
  (ns) ? (decodeSymbol1 (MinContext)) : (decodeBinSymbol (MinContext));
  ariRemoveSubrange ();
  while (!FoundState)
    {
      ARI_DEC_NORMALIZE (EncodedFile);
      do
	{
	  OrderFall++;
	  MinContext = MinContext->Suffix;
	  if (!MinContext)
	    return EOF;
	}
      while (MinContext->NumStats == NumMasked);
      decodeSymbol2 (MinContext);
      ariRemoveSubrange ();
    }
  c = FoundState->Symbol;
  if (!OrderFall && (BYTE *) FoundState->Successor >= sa->UnitsStart)
    MaxContext = FoundState->Successor;
  else
    {
      UpdateModel (MinContext);
      if (EscCount == 0)
	ClearMask ();
    }
  ns = (MinContext = MaxContext)->NumStats;
  ARI_DEC_NORMALIZE (EncodedFile);
  return c;
}

