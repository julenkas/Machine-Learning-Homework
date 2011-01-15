/**************************************************************************
 *
 * bitarray.h
 *
 * Macros related to the creation and manipulation of a bit array.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#ifndef BITARRAY_H
#define BITARRAY_H 1

#include "auxi.h"

#ifndef uchar
#define uchar unsigned char
#endif // uchar

/* Create a bit array. */
#define CREATE_BITARRAY(size)				\
  ((uchar *) getmem(((size)/8 + 1) * sizeof(uchar)))

/* Change (increase) the size of a bitarray. */
#define REALLOC_BITARRAY(bitarray, newsize)		\
   (bitarray) = (uchar *)				\
     realloc((bitarray),				\
	     ((newsize) / 8 + 1) * sizeof(uchar));	\
   if ((bitarray) == (uchar *) NULL)			\
     USER_ERROR1("realloc(%d bytes)",			\
		 ((newsize) + 1) / 8 * sizeof(uchar));

/* Read an element (bit) from a bit array. */
#define READ_BITARRAY(bitarray, offset)					\
  (((((uchar *) (bitarray))[(offset) >> 3] >> ((offset) & 0x7))) & 0x1)

/* Write an element (bit) to a bit array. */
#define WRITE_BITARRAY(bitarray, offset, value)	{		\
  uchar mask = (1 << ((offset) % 8));				\
  ((uchar *) (bitarray))[(offset)/8] &= (~mask) & 0xff;		\
  ((uchar *) (bitarray))[(offset)/8] |=				\
     (((uchar) (value)) << ((offset) % 8)) & mask & 0xff;	\
}

/* Set bits from begin to end inclusive to 1. */
#define SET_BITARRAY_RANGE(bitarray,begin,end){                 \
  int i;                                                        \
  for(i=begin;i<=end;i++)                                        \
    WRITE_BITARRAY(bitarray,i,1);                               \
}

/* Fill a bit array with zeros. */
#define ZERO_BITARRAY(bitarray, size) {				\
  int i;							\
  for (i = 0; i < (size)/8/sizeof(long); i++)			\
    ((long *) (bitarray))[i] = (long) 0;			\
  for (i = (size)/8/sizeof(long); i < (size)/8 + 1; i++)	\
    ((uchar *) (bitarray))[i] = '\0';				\
}

/* Cpoy a bitarray into another. */
#define COPY_BITARRAY(dest_bitarray, source_bitarray, size) {		 \
  int i;								 \
  for (i = 0; i < (size); i++) {					 \
    WRITE_BITARRAY(dest_bitarray, i, READ_BITARRAY(source_bitarray, i)); \
  }									 \
}

#define COPY_BITARRAY_RANGE(dest_bitarray, dest_begin, source_bitarray, source_begin, size) {    \
  int i;                                                                                         \
  for (i = 0; i< (size); i++) {                                                                  \
    WRITE_BITARRAY(dest_bitarray, i+dest_begin, READ_BITARRAY(source_bitarray, i+source_begin)); \
  }								                                 \
}

#endif // BITARRAY_H
/**************************************************************************/
