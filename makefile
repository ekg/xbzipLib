SHELL=/bin/sh

CC=gcc

#these are for maximum speed
CFLAGS=-g -O3 -fomit-frame-pointer -W -Wall -Winline -DDEBUG=0 -DNDEBUG=1 -lm

#these are for testing
#CFLAGS = -pg -W -Wall -Winline -O2


.PHONY: all
all: xbzip


# This is the library of Paolo Ferragina and Rossano Venturini for FMindex 2.0
fm_index.a:
	make -C ./fm-index/ 
	cp -f ./fm-index/fm_index.a .; cp -f ./fm-index/interface.h .
	cp -f ./fm-index/ds_ssort/ds_ssort.a .; cp -f ./fm-index/ds_ssort/ds_ssort.h .


# This is the library of Paolo Ferragina and Rossano Venturini for FMindex 2.0
libz.a:
	make -C ./zlib/ 
	cp -f ./zlib/libz.a .; cp -f ./zlib/zlib.h .; cp -f ./zlib/zconf.h .
	# Taken from XMLPPM by James Cheney, adapted to just PPMDI by Joaquin Adiego
	make -C ./ppmdi-source/ 
	chmod 755 ./ppmdi-source/ppmdi; chmod 755 ./ppmdi-source/unppmdi
	cp -f ./ppmdi-source/ppmdi . ; cp -f ./ppmdi-source/unppmdi .

# This is the library of Paolo Ferragina for BWT-compression with one large block
bigbzip.a: 
	make -C ./bigbzip/; cp -f ./bigbzip/bigbzip.a .; cp -f ./bigbzip/bigbzip.h .
	#cp -f ./bigbzip/ds_ssort/ds_ssort.a .; cp -f ./bigbzip/ds_ssort/ds_ssort.h .

# archive containing the xbzip algorithm
xbzip.a: bigbzip.a libz.a fm_index.a xbzip_fnct_compr.o xbzip_fnct_index.o xbzip_aux.o xbzip_parser.o xbzip_hash.o data_compressor.o
	ar rc xbzip.a libz.a fm_index.a bigbzip.a xbzip_fnct_compr.o xbzip_fnct_index.o xbzip_aux.o xbzip_parser.o xbzip_hash.o data_compressor.o 

# Use of expat and xbzip library
xbzip: fm_index.a libz.a bigbzip.a xbzip.a xbzip.c  
	$(CC) $(CFLAGS) -o xbzip xbzip.c xbzip.a libz.a bigbzip.a ds_ssort.a fm_index.a -lexpat


# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@


clean: 
	rm -f *.o *.a *.exe
	rm -f bigbzip/ds_ssort/*.o bigbzip/ds_ssort/*.exe bigbzip/ds_ssort/*.a 
	rm -f bigbzip/*.o bigbzip/*.exe bigbzip/*.a
	rm -f fm-index/*.o fm-index/*.exe fm-index/*.a;  
	rm -f fm-index/ds_ssort/*.o fm-index/ds_ssort/*.exe fm-index/ds_ssort/*.a
	rm -f zlib/*.o zlib/*.exe zlib/*.a
	rm -f ppmdi unppmdi ppmdi-source/*.o ppmdi-source/ppmdi ppmdi-source/unppmdi

tarfile:
	make clean; 
	tar zcvf xbzipLib.tgz makefile makefile-FORUNIX makefile-FORWIN *.c *.h README.txt COPYRIGHT.txt xml_normalize.pl bigbzip/ zlib/ fm-index/ ppmdi-source/
