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

/* StringArray.cpp: simple string dictionaries for tokenizing XML tags */

#include "StringArray.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" char *strdup (const char *);

StringArray::StringArray (int size)
{
  int i;
  arr = new char *[size];
  for (i = 0; i < size; i++)
    arr[i] = NULL;
  len = 0;
  max = size;
}

int
StringArray::lookup (const char *elt)
{
  int i;
  for (i = 0; i < len; i++)
    {
      if (strcmp (elt, arr[i]) == 0)
	{
	  return i;
	}
    }
  return -1;
}

void
StringArray::add (const char *elt)
{
  if (lookup (elt) != -1)
    {
      return;
    }
  else if (len == max)
    {
      fprintf (stderr, "Too many element/attribute symbols encountered."
	       "Please pester the XMLPPM developer to fix this.\n");
      exit (-1);
    }
  arr[len] = strdup (elt);
  len++;
  return;
}


int
StringArray::isFull ()
{
  return max == len;
}
