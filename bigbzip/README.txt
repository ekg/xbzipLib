Release notes for BigBzip,   version 1.0
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Copyright (C) 2005
Paolo Ferragina (ferragina@di.unipi.it)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


This Library offers an implementation of a BWT-based compressor consisting
of four stages:
(i) Burrows-Wheeler Transform of the text;
(ii) Move-To-Front coding of the string output by (i);
(iii) Run-Length-Encoding by Wheeler's code of the 0-runs output by (ii);
(iv) 0th order compression of the output by (iii) via the Multi-Table
Huffman compressor.

In order to use this Library you need the ds_ssort library
to build the suffix array, by Giovanni Manzini.
Please download this library from the site:

http://www.mfn.unipmn.it/~manzini/lightweight/index.html

You also need the bzip2 and/or libbzip2 library to 0-order compress
via the Multi-Table Huffman, by Julian Seward.
However, you do not need to download these libraries since
everything is available in the present package.

All the cited libaries follow the GNU-GPL license, as the present one.

Let us denote by <HOME> the directory in which you copied the BigBzip library.
To build all the stuff you need for BigBzip you need to do the following.
Issue the command MAKE in <HOME>. This creates the command "bigbzip",
and the archive "bigbzip.a" which provides a simplified access to the compression
and decompression routines.

If you wish to use the BigBzip library you need to use just the
procedures bigbzip_compress() and bigbzip_decompress() defined
in the file bigbzip_fnct.c. Details follow,

void bigbzip_compress(unsigned char text[], int text_len,
                        unsigned char *ctext[], int *ctext_len)
where
  text[]   is an array of length (text_len) which contains the
           input string to be compressed.
  *ctext[] is an array allocated within this procedure and has
           length (*ctext_len). At the end of the computation
           *ctext[] will contain the compressed text.
         *ctext_len will be set inside the current procedure.

void bigbzip_decompress(unsigned char ctext[], int ctext_len,
                        unsigned char *text[], int *text_len)

where
    the role of the parameters is inverted with respect to
    bigbzip_compress().

Don't forget to include the file bigbzip.h in your source code
before calling the procedures bigbzip_compress() and
bigbzip_decompress(). If you need a working example, please have
a look at bigbzip.c.

The software in this archive should be considered an ALPHA version.
I will be glad to receive your comments and bug reports.

Paolo Ferragina
ferragina@di.unipi.it
