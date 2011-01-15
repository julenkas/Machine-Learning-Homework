/**************************************************************************
 *
 * auxi.c
 *
 * Auxiliary routines.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include "auxi.h"

/* -------------------------------------------------------------------------
 
   Memory allocator.  Exits program if there is not enough memory.
 
  ------------------------------------------------------------------------- */

void *getmem(size_t bytes)
{
  void *ptr;

  if ((ptr = malloc(bytes)) == (void *) NULL)
    USER_ERROR1("memory request for %d bytes failed\n", bytes);

  return ptr;
}

/* -------------------------------------------------------------------------
 
   Generate a random number in the interval [a, b).  Gets seed from current
   time the first time it is called.

  ------------------------------------------------------------------------- */

double uniform(double a, double b)
{
  return (a == b) ? 0.0 : ((double) random()) / RAND_MAX * (b - a) + a;
}

/***************************************************************************/
