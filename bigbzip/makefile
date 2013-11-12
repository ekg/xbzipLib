SHELL=/bin/sh

CC=gcc

#these are for maximum speed
CFLAGS=-g -O3 -fomit-frame-pointer -W -Wall -Winline -DDEBUG=0 -DNDEBUG=1 


.PHONY: all
all : ds_ssort bigbzip

# This is the library of Giovanni Manzini for Suffix Array construction
ds_ssort.a: 
	make -C ./ds_ssort/; cp -f ./ds_ssort/ds_ssort.a .; cp -f ./ds_ssort/ds_ssort.h . 


# archive containing the big_bzip algorithm
bigbzip.a: ds_ssort.a mng_bits.o huffman.o multihuf.o bigbzip_fnct.o 
	ar rc bigbzip.a mng_bits.o huffman.o multihuf.o bigbzip_fnct.o

# bigbzip command
bigbzip: bigbzip.c bigbzip.a
	 $(CC) $(CFLAGS) -o bigbzip bigbzip.c bigbzip.a ds_ssort.a  

# pattern rule for all objects files
%.o: %.c *.h
	$(CC) -c $(CFLAGS) $< -o $@

clean: 
	rm -f *.o *.a ds_ssort/*.o ds_ssort/*.a 

tarfile:
	make clean; tar zcvf bigbzip.tgz makefile *.c *.h COPYRIGHT.txt README.txt ds_ssort/
