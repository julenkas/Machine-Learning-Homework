/**************************************************************************
 *
 * prune-dt.h
 *
 * Header file to prune-dt.c
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#ifndef PRUNE_DT_H
#define PRUNE_DT_H 1

#include "dt.h"
#include "ssv.h"
#include "bitarray.h"

/* Function prototypes. */
int CheckCorrectness(DTNODE *node, void **data, int num_data,
		     double pos_prior, int example, SSVINFO *ssvinfo, int depth);
void DecisionTreeAccuracyBinary(DTNODE *root,
				void **data, int num_data,
				uchar *train_members, int num_train,
				uchar *test_members, int num_test,
				int *num_negatives, int *num_false_negatives,
				int *num_positives, int *num_false_positives,
				SSVINFO *ssvinfo, int depth);
double DecisionTreeAccuracy(DTNODE *root, void **data, int num_data,
			    uchar *test_members, int num_test,
			    SSVINFO *ssvinfo);
void CountDTPosNeg(DTNODE *root, int *num_pos, int *num_neg);
void PruneDecisionTree(DTNODE *root, DTNODE *node,
			      void **data, int num_data,
			      uchar *pruning_set, int num_prune,
			      SSVINFO *ssvinfo);

#endif // PRUNE_DT_H
/**************************************************************************/
