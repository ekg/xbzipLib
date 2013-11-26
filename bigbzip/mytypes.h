
	
/* ------------- Standard Include ---------- */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/times.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>
#include <math.h>

/* ---------- types and costants ----------- */
typedef int          Int32;
typedef unsigned int UInt32;
typedef unsigned short UInt16;
typedef unsigned char UChar;
typedef unsigned char Bool;
typedef unsigned long long UInt64;
#define True   ((Bool)1)
#define False  ((Bool)0)

#ifndef min
#define min(a, b) ((a)<=(b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a)>=(b) ? (a) : (b))
#endif


