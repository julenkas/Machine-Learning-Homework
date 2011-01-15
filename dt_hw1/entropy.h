/**************************************************************************
 *
 * entropy.h
 *
 * Header file to entropy.c
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#ifndef ENTROPY_H
#define ENTROPY_H 1

#include "bitarray.h"
#include "ssv.h"

/* Function prototypes. */
void CountExamples(void **data, int num_data,
		   uchar *train_members, int num_train,
		   int *num_pos, int *num_neg);
double Entropy(int num_pos, int num_neg);
double DataEntropy(void **data, int num_data,
	       uchar *members, int num_members,
	       SSVINFO *ssvinfo);
double PartialEntropyBinary(void **data, int num_data,
			    uchar *members, int num_members, int attr,
			    SSVINFO *ssvinfo);
double PartialEntropyDiscrete(void **data, int num_data,
			      uchar *members, int num_members, int attr,
			      SSVINFO *ssvinfo);
double PartialEntropyContinuous(void **data, int num_data,
				uchar *members, int num_members,
				int attr, double *best_threshold);
int MaxGainAttribute(void **examples, int num_examples, int num_attribs,
		     uchar *members, int num_members,
		     double *best_threshold, SSVINFO *ssvinfo);

#endif // ENTROPY_H
/**************************************************************************/
