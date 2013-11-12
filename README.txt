Release notes for XBzip,   version 1.0
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


------------------------------------------------
------------------- IMPORTANT NOTE
------------------------------------------------

If you are running the library under Linux, you have to substitute
the two files data_compressor.c and makefile, with the corresponding
ones having the label "-FORUNIX".

You also need to download and install the EXPAT Library, by James Clark,
from: http://sourceforge.net/projects/expat/. Other libraries that
are needed by XBZIP have been made available within the package and
will be compiled and installed during the MAKE.

----------------------------------------------------------------
----------------------------------------------------------------

The library xbzip.a offers an implementation of a Transform suitable
for XML compression and indexing. This transform has been proposed in the
paper:

P. Ferragina, F. Luccio, G. Manzini, S. Muthukrishnan
Structuring labeled trees for succinctness, and beyond
IEEE Symposium on Foundations of Computer Science, 2005


In order to use the XBZIP library you need the following additional
packages (which are included within the XBzip package):

1. You need the ds_ssort.a library, by Giovanni Manzini.
http://www.mfn.unipmn.it/~manzini/lightweight/index.html

2. You also need the bigbzip.a library, by myself.
http://www.di.unipi.it/~ferragin/index.html

3. You need the libz.a library. http://www.zlib.net/

4. You need the fm_index.a library, by myself and R. Venturini.
http://www.di.unipi.it/~ferragin/fmindex/FM-pages/fmindex_vers2.tar.gz

5. You need the source-code of the PPMDI compressor, to be used under
Linux. http://compression.ru/ds/ppmdi1.rar

You also need to download the EXPAT Library, by James Clark,
from: http://sourceforge.net/projects/expat/

To compile the files you must issue the command MAKE in the XBZIP directory. This
creates the command "xbzip", and the archive "xbzip.a" which
provides a simplified access to the compression and decompression routines.

Please have a look at the xbzip.h file for the APIs and the list of the additional
functions we have implemented.

Don't forget to include the file xbzip.h in your source code
before calling the various procedures.
If you need a working example, please have a look at xbzip.c.

All the cited libraries follow the GNU-GPL license, as ours.
The present software should be considered an ALPHA version.
I will be glad to receive your comments and bug reports.

Paolo Ferragina
ferragina@di.unipi.it
