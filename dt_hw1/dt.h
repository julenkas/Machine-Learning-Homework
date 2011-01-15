/**************************************************************************
 *
 * dt.h
 *
 * Header file to dt.c
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#ifndef DT_H
#define DT_H 1

#ifndef MIN_LEAF_MEMBERS
#define MIN_LEAF_MEMBERS 1
#endif // MIN_LEAF_MEMBERS

#define MAX_STRING_LEN 1024

/* Tree node definition. */
typedef struct dtnode {
  int num_members;              /* Number of examples within the subtree
				   rooted at this node. */
  int num_pos;                  /* Number of positive examples in subtree. */
  int num_neg;                  /* Number of negative examples in subtree. */

  /* ---- The following fields are only used for internal nodes. ---- */
  int num_children;             /* Number of children; 0 if a leaf. */
  struct dtnode **children;     /* Array of pointers to children
				   subtrees. */
  int test_attrib;              /* Index of attribute to test at this node,
				   if non-leaf. */
  double threshold;             /* Value to test against at this node if
				   continuous, compare using <. */

  /* ---------------------------------------------------------------- */
} DTNODE;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auxi.h"
#include "entropy.h"
#include "bitarray.h"
#include "ssv.h"

/* Function prototypes. */
DTNODE *CreateDecisionTree(void **data, int num_data, int num_features,
			   double approx_prune_pct, double approx_test_pct,
			   uchar *train_members, int num_train,
			   SSVINFO *ssvinfo);
DTNODE *CreateDecisionSubTreeBinary(void **data, int num_data,
				    uchar *train_members, int num_train,
				    int num_features, int attr,
				    SSVINFO *ssvinfo);
DTNODE *CreateDecisionSubTreeDiscrete(void **data, int num_data,
				      uchar *train_members, int num_train,
				      int num_features, int attr,
				      SSVINFO *ssvinfo);
DTNODE *CreateDecisionSubTreeContinuous(void **data, int num_data,
					uchar *train_members, int num_train,
					int num_features,
					int attr, double threshold,
					SSVINFO *ssvinfo);
DTNODE *CreateDecisionTreeAux(void **data, int num_data,
			      uchar *train_members, int num_train,
			      int num_features, SSVINFO *ssvinfo);
void FreeDecisionTreeNode(DTNODE *node);
void FreeDecisionTreeChildren(DTNODE *node);
void FreeDecisionTree(DTNODE *root);
int CountNodes(DTNODE *root);
int CountNodesDepth(DTNODE *root, int depth);

#endif // DT_H
/**************************************************************************/
