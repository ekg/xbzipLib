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

/* Derived 5/2001 from PPMd sources by James Cheney to interface to XMLPPM
 */

#include "Coder.h"

SUBRANGE SubRange;
DWORD low, code, range;

void
ariInitEncoder (FILE * stream)
{
  low = 0;
  range = DWORD (-1);
}

void
ariEncodeSymbol ()
{
  low += SubRange.LowCount * (range /= SubRange.scale);
  range *= SubRange.HighCount - SubRange.LowCount;
}


void
ariShiftEncodeSymbol (UINT SHIFT)
{
  low += SubRange.LowCount * (range >>= SHIFT);
  range *= SubRange.HighCount - SubRange.LowCount;
}

int
ariGetCurrentCount ()
{
  return (code - low) / (range /= SubRange.scale);
}

UINT
ariGetCurrentShiftCount (UINT SHIFT)
{
  return (code - low) / (range >>= SHIFT);
}

void
ariRemoveSubrange ()
{
  low += range * SubRange.LowCount;
  range *= SubRange.HighCount - SubRange.LowCount;
}
