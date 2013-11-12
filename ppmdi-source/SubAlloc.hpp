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
 *  Contents: memory allocation routines                                    *
 ****************************************************************************/
/* Derived 1/2003 from PPMd sources by James Cheney to use with XMLPPM 
 */
#ifndef __SUBALLOC_HPP__
#define __SUBALLOC_HPP__

const unsigned int UNIT_SIZE = 12;
const unsigned int N1 = 4;
const unsigned int N2 = 4;
const unsigned int N3 = 4;
const unsigned int N4 = (128+3-1*N1-2*N2-3*N3)/4;
const unsigned int N_INDEXES = N1 + N2 + N3 + N4;

#pragma pack(1)
struct BLK_NODE
{
  DWORD Stamp;
  BLK_NODE *next;
  BOOL avail () const
  {
    return (next != NULL);
  }
  void link (BLK_NODE * p)
  {
    p->next = next;
    next = p;
  }
  void unlink ()
  {
    next = next->next;
  }
  void *remove ()
  {
    BLK_NODE *p = next;
    unlink ();
    Stamp--;
    return p;
  }
  inline void insert (void *pv, int NU);
};





struct MEM_BLK:public BLK_NODE
{
  DWORD NU;
}
_PACK_ATTR;
#pragma pack()


inline void
BLK_NODE::insert (void *pv, int NU)
{
  MEM_BLK *p = (MEM_BLK *) pv;
  link (p);
  p->Stamp = ~0UL;
  p->NU = NU;
  Stamp++;
}

inline UINT
U2B (UINT NU)
{
  return 8 * NU + 4 * NU;
}


struct SubAlloc {
  struct BLK_NODE BList[N_INDEXES];

  BYTE Indx2Units[N_INDEXES], Units2Indx[128];	// constants
  DWORD GlueCount, SubAllocatorSize;
  BYTE *HeapStart, *pText, *UnitsStart, *LoUnit, *HiUnit;
  UINT Restarts;

  SubAlloc() {
    unsigned int i,k;
    Restarts = 0;
    SubAllocatorSize = 0;
    for (i = 0, k = 1; i < N1; i++, k += 1)
      Indx2Units[i] = k;
    for (k++; i < N1 + N2; i++, k += 2)
      Indx2Units[i] = k;
    for (k++; i < N1 + N2 + N3; i++, k += 3)
      Indx2Units[i] = k;
    for (k++; i < N1 + N2 + N3 + N4; i++, k += 4)
      Indx2Units[i] = k;
    for (k = i = 0; k < 128; k++)
      {
	i += (Indx2Units[i] < k + 1);
	Units2Indx[k] = i;
      }
  }
  inline void
  SplitBlock (void *pv, UINT OldIndx, UINT NewIndx)
  {
    UINT i, k, UDiff = Indx2Units[OldIndx] - Indx2Units[NewIndx];
    BYTE *p = ((BYTE *) pv) + U2B (Indx2Units[NewIndx]);
    if (Indx2Units[i = Units2Indx[UDiff - 1]] != UDiff)
      {
	k = Indx2Units[--i];
	BList[i].insert (p, k);
	p += U2B (k);
	UDiff -= k;
      }
    BList[Units2Indx[UDiff - 1]].insert (p, UDiff);
  }
  
  DWORD
  GetUsedMemory ()
  {
    DWORD i, RetVal =
      SubAllocatorSize - (HiUnit - LoUnit) - (UnitsStart - pText);
    for (i = 0; i < N_INDEXES; i++)
      RetVal -= UNIT_SIZE * Indx2Units[i] * BList[i].Stamp;
    return RetVal;
  }
  
  void
  StopSubAllocator ()
  {
    if (SubAllocatorSize)
      {
	SubAllocatorSize = 0;
	delete[]HeapStart;
      }
  }
  BOOL
  StartSubAllocator (UINT SASize)
  {
    DWORD t = SASize << 10U;
    if (SubAllocatorSize == t)
      return TRUE;
    StopSubAllocator ();
    if ((HeapStart = new BYTE[t]) == NULL)
      return FALSE;
    SubAllocatorSize = t;
    return TRUE;
  }
  inline void
  InitSubAllocator ()
  {
    Restarts++;
    memset (BList, 0, sizeof (BList));
    HiUnit = (pText = HeapStart) + SubAllocatorSize;
    UINT Diff = UNIT_SIZE * (SubAllocatorSize / 8 / UNIT_SIZE * 7);
    LoUnit = UnitsStart = HiUnit - Diff;
    GlueCount = 0;
  }
  void
  GlueFreeBlocks ()
  {
    UINT i, k, sz;
    MEM_BLK s0, *p, *p0, *p1;
    if (LoUnit != HiUnit)
      *LoUnit = 0;
    for (i = 0, (p0 = &s0)->next = NULL; i < N_INDEXES; i++)
      while (BList[i].avail ())
	{
	  p = (MEM_BLK *) BList[i].remove ();
	  if (!p->NU)
	    continue;
	  while ((p1 = p + p->NU)->Stamp == ~0UL)
	    {
	      p->NU += p1->NU;
	      p1->NU = 0;
	    }
	  p0->link (p);
	  p0 = p;
	}
    while (s0.avail ())
      {
	p = (MEM_BLK *) s0.remove ();
	sz = p->NU;
	if (!sz)
	  continue;
	for (; sz > 128; sz -= 128, p += 128)
	  BList[N_INDEXES - 1].insert (p, 128);
	if (Indx2Units[i = Units2Indx[sz - 1]] != sz)
	  {
	    k = sz - Indx2Units[--i];
	    BList[k - 1].insert (p + (sz - k), k);
	  }
	BList[i].insert (p, Indx2Units[i]);
      }
    GlueCount = 1 << 13;
  }
  void *
  AllocUnitsRare (UINT indx)
  {
    UINT i = indx;
    if (!GlueCount)
      {
	GlueFreeBlocks ();
	if (BList[i].avail ())
	  return BList[i].remove ();
    }
    do
      {
	if (++i == N_INDEXES)
	  {
	    GlueCount--;
	    i = U2B (Indx2Units[indx]);
	    return (UnitsStart - pText > (int)i) ? (UnitsStart -= i) : (NULL);
	  }
      }
    while (!BList[i].avail ());
    void *RetVal = BList[i].remove ();
    SplitBlock (RetVal, i, indx);
    return RetVal;
  }
  inline void *
  AllocUnits (UINT NU)
  {
    UINT indx = Units2Indx[NU - 1];
    if (BList[indx].avail ())
      return BList[indx].remove ();
    void *RetVal = LoUnit;
    LoUnit += U2B (Indx2Units[indx]);
    if (LoUnit <= HiUnit)
      return RetVal;
    LoUnit -= U2B (Indx2Units[indx]);
    return AllocUnitsRare (indx);
  }
  inline void *
  AllocContext ()
  {
    if (HiUnit != LoUnit)
      return (HiUnit -= UNIT_SIZE);
    else if (BList->avail ())
      return BList->remove ();
    else
      return AllocUnitsRare (0);
  }
  inline void
  UnitsCpy (void *Dest, void *Src, UINT NU)
  {
    DWORD *p1 = (DWORD *) Dest, *p2 = (DWORD *) Src;
    do
      {
	p1[0] = p2[0];
	p1[1] = p2[1];
	p1[2] = p2[2];
	p1 += 3;
	p2 += 3;
      }
    while (--NU);
  }
  inline void *
  ExpandUnits (void *OldPtr, UINT OldNU)
  {
    UINT i0 = Units2Indx[OldNU - 1], i1 = Units2Indx[OldNU - 1 + 1];
    if (i0 == i1)
      return OldPtr;
    void *ptr = AllocUnits (OldNU + 1);
    if (ptr)
      {
	UnitsCpy (ptr, OldPtr, OldNU);
	BList[i0].insert (OldPtr, OldNU);
      }
    return ptr;
  }
  inline void *
  ShrinkUnits (void *OldPtr, UINT OldNU, UINT NewNU)
  {
    UINT i0 = Units2Indx[OldNU - 1], i1 = Units2Indx[NewNU - 1];
    if (i0 == i1)
      return OldPtr;
    if (BList[i1].avail ())
      {
	void *ptr = BList[i1].remove ();
	UnitsCpy (ptr, OldPtr, NewNU);
	BList[i0].insert (OldPtr, Indx2Units[i0]);
	return ptr;
      }
    else
      {
	SplitBlock (OldPtr, i0, i1);
	return OldPtr;
      }
  }
  inline void
  FreeUnits (void *ptr, UINT NU)
  {
    UINT indx = Units2Indx[NU - 1];
    BList[indx].insert (ptr, Indx2Units[indx]);
  }
  inline void
  SpecialFreeUnit (void *ptr)
  {
    if ((BYTE *) ptr != UnitsStart)
      BList->insert (ptr, 1);
    else
      {
	*(DWORD *) ptr = ~0UL;
	UnitsStart += UNIT_SIZE;
      }
  }
  inline void *
  MoveUnitsUp (void *OldPtr, UINT NU)
  {
    UINT indx = Units2Indx[NU - 1];
    if ((BYTE *) OldPtr > UnitsStart + 16 * 1024
	|| (BLK_NODE *) OldPtr > BList[indx].next)
      return OldPtr;
    void *ptr = BList[indx].remove ();
    UnitsCpy (ptr, OldPtr, NU);
    NU = Indx2Units[indx];
    if ((BYTE *) OldPtr != UnitsStart)
      BList[indx].insert (OldPtr, NU);
    else
      UnitsStart += U2B (NU);
    return ptr;
  }
  inline void
  ExpandTextArea ()
  {
    BLK_NODE *p;
    UINT Count[N_INDEXES];
    memset (Count, 0, sizeof (Count));
    while ((p = (BLK_NODE *) UnitsStart)->Stamp == ~0UL)
      {
	MEM_BLK *pm = (MEM_BLK *) p;
	UnitsStart = (BYTE *) (pm + pm->NU);
	Count[Units2Indx[pm->NU - 1]]++;
	pm->Stamp = 0;
      }
    for (UINT i = 0; i < N_INDEXES; i++)
      for (p = BList + i; Count[i] != 0; p = p->next)
	while (!p->next->Stamp)
	  {
	    p->unlink ();
	    BList[i].Stamp--;
	    if (!--Count[i])
	      break;
	  }
  }
};
#endif
