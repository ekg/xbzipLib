CC = g++

#__CYGWIN__ = 1
INSTALLDIR = $(HOME)/bin

ifdef DEBUG
DEBUG_FLAGS = -g
else 
CODE_FLAGS = -fno-exceptions -fno-rtti -fcheck-new \
    -Wall -pedantic -Wno-sign-compare -Wno-unknown-pragmas -Wno-conversion
OPT_FLAGS = -O4 
CODE_FLAGS =
endif 

ifdef PROF
CPPFLAGS += -pg
CFLAGS += -pg
LDFLAGS += -pg
else
ifdef DEBUG
else
CODE_FLAGS += -fomit-frame-pointer
endif
endif

CPPFLAGS += $(CODE_FLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) 
CFLAGS += $(CPPFLAGS)
#LDFLAGS += -lexpat
ifdef __CYGWIN__
LDFLAGS += -liconv
CPPFLAGS += -D__CYGWIN__
SUF = .exe
endif
MODULES = Args Coder Model  
SRCS = $(addsuffix .cpp,$(MODULES))
OBJS = $(addsuffix .o,$(MODULES))

BINARIES = ppmdi$(SUF) unppmdi$(SUF)

all:	$(BINARIES)

ppmdi$(SUF):  xmlppm.o $(OBJS)
	$(CC)   $(CFLAGS) -o $@ $^ $(LDFLAGS) 

unppmdi$(SUF): xmlunppm.o $(OBJS)
	$(CC)  $(CFLAGS) -o $@ $^  $(LDFLAGS) 

clean: 
	rm -f $(BINARIES) *.exe *.new *.ppm *.xppm* *.o *.stackdump core *~

distclean: clean
	rm -rf Debug Release

install: all
	cp $(BINARIES) $(INSTALLDIR)
