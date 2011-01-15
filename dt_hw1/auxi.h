/**************************************************************************
 *
 * auxi.h
 *
 * Header file for auxi.c
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>

#ifndef AUX_H
#define AUX_H 1

/* Global variables. */
extern char *progname;  /* Should be defined by all programs who call this
			   library, and should contain the program name. */

#define SYS_ERROR1(format, arg)					\
{								\
  char tmp_buf[256];						\
  sprintf(tmp_buf, "%s: %s:%d %s(): " format,			\
	  progname, __FILE__, __LINE__, __FUNCTION__, arg);	\
  perror(tmp_buf);						\
  exit(1);							\
}
#define SYS_ERROR2(format, arg1, arg2)					\
{									\
  char tmp_buf[256];							\
  sprintf(tmp_buf, "%s: %s:%d %s(): " format,				\
	  progname, __FILE__, __LINE__, __FUNCTION__, arg1, arg2);	\
  perror(tmp_buf);							\
  exit(1);								\
}
#define SYS_ERROR3(format, arg1, arg2, arg3)				 \
{									 \
  char tmp_buf[256];							 \
  sprintf(tmp_buf, "%s: %s:%d %s(): " format,				 \
	  progname, __FILE__, __LINE__, __FUNCTION__, arg1, arg2, arg3); \
  perror(tmp_buf);							 \
  exit(1);								 \
}
#define USER_ERROR1(format, arg)				\
{								\
  fprintf(stderr, "%s: %s:%d %s(): " format "\n",		\
	  progname, __FILE__, __LINE__, __FUNCTION__, arg);	\
  exit(1);							\
}
#define USER_ERROR2(format, arg1, arg2)					\
{									\
  fprintf(stderr, "%s: %s:%d %s(): " format "\n",			\
	  progname, __FILE__, __LINE__, __FUNCTION__, arg1, arg2);	\
  exit(1);								\
}
#define USER_ERROR3(format, arg1, arg2, arg3)				 \
{									 \
  fprintf(stderr, "%s: %s:%d %s(): " format "\n",			 \
	  progname, __FILE__, __LINE__, __FUNCTION__, arg1, arg2, arg3); \
  exit(1);								 \
}
#ifndef SQUARE
#define SQUARE(x) ((x) * (x))
#endif // SQUARE
#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : -(x))
#endif // ABS
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif // MIN
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif // MAX
#define PI M_PI
#define DEG * 180.0 / PI  /* Convert to degrees. */
#define RAD * PI / 180.0  /* Convert to radians. */
#ifdef sun
#define RAND_MAX (pow(2.0, 31.0) - 1.0)
#endif // sun
#ifndef EPSILON
#define EPSILON 0.005
#endif

/* Global variables. */
extern int random_seed;

/* Declarations. */
void *getmem(size_t bytes);
double uniform(double a, double b);

#endif // AUX_H
/**************************************************************************/
