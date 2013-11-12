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
 *  Contents: 'Carryless rangecoder' by Dmitry Subbotin                     *
 *  Comments: this implementation is claimed to be a public domain          *
 ****************************************************************************/
/* Changes 1/2003 by James Cheney to interface to XMLPPM
 */
#ifndef __CODER_HPP_
#define __CODER_HPP_

#include "PPMdType.h"
#include <stdio.h>

struct SUBRANGE {
    DWORD LowCount, HighCount, scale;
};
extern SUBRANGE SubRange;
const DWORD TOP=1 << 24, BOT=1 << 15;
extern DWORD low, code, range;

void ariInitEncoder(FILE* stream);

#define ARI_ENC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        putc(low >> 24,stream);                                             \
        range <<= 8;                                                        \
        low <<= 8;                                                          \
    }                                                                       \
}
void ariEncodeSymbol();

void ariShiftEncodeSymbol(UINT SHIFT);

#define ARI_FLUSH_ENCODER(stream) {                                         \
    for (int i=0;i < 4;i++) {                                               \
        putc(low >> 24,stream);                                             \
        low <<= 8;                                                          \
    }                                                                       \
}
#define ARI_INIT_DECODER(stream) {                                          \
    low=code=0;                                                             \
    range=DWORD(-1);                                                        \
    for (int i=0;i < 4;i++)                                                 \
      code=(code << 8) | getc(stream);                                      \
}
#define ARI_DEC_NORMALIZE(stream) {                                         \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        code=(code << 8) | getc(stream);                                    \
        range <<= 8;                                                        \
        low <<= 8;                                                          \
    }                                                                       \
}
int ariGetCurrentCount();
UINT ariGetCurrentShiftCount(UINT SHIFT);
void ariRemoveSubrange();

#endif
