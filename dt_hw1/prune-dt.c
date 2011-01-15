/**************************************************************************
 *
 * prune-dt.c
 *
 * Source file containing routines related to post-pruning a  decision tree
 * in order to improve classification performance as measured over a set of
 * examples.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include "prune-dt.h"
#include "bitarray.h"
#include "ssv.h"

/* ----------------------------------------------------------------------

   Recursively check the correctness of an example as classified by the
   decision tree up to a certain depth.  Returns 1 if correct and 0 otherwise.

   ---------------------------------------------------------------------- */

int CheckCorrectness(DTNODE *node, void **data, int num_data,
		     double pos_prior, int example, SSVINFO *ssvinfo, int depth)
{
  int correct, prediction;
  int child;

  if (node == NULL)
    return 0;
  if ((node->num_children == 0) || (depth == 1)) {  /* Leaf, check what tree predicts. */
    prediction = (node->num_pos >= (pos_prior * node->num_members));
    correct = (READ_ATTRIB_B(data, example, 0) == prediction);
  } else {    /* Internal node, check appropriate child. */
    switch (ssvinfo->types[node->test_attrib]) {
      case 'b': /* Attrib tested is binary. */
	child = READ_ATTRIB_B(data, example, node->test_attrib);
	break;
    case 'd':
	child = READ_ATTRIB_I(data, example, node->test_attrib);
      break;
    case 'c': /* Attribute tested at node is continuous. */
      child =
	(READ_ATTRIB_C(data, example, node->test_attrib) >= node->threshold);
      break;
    default:
      USER_ERROR1("Unknown attribute type '%c'",
		  ssvinfo->types[node->test_attrib]);
    }
    correct = CheckCorrectness(node->children[ child ],
			       data, num_data, pos_prior,
			       example, ssvinfo, depth-1);
  }

  return correct;
}

/* ----------------------------------------------------------------------

   Compute classification accuracy over a set of examples of a decision
   tree up to a certain depth for binary target attributes.  
   Returns a number in [0.0, 1.0].

   ---------------------------------------------------------------------- */

void DecisionTreeAccuracyBinary(DTNODE *root,
				void **data, int num_data,
				uchar *train_members, int num_train,
				uchar *test_members, int num_test,
				int *num_negatives, int *num_false_negatives,
				int *num_positives, int *num_false_positives,
				SSVINFO *ssvinfo, int depth)
{
  int example;
  int num_all_pos, num_all_neg;
  double pos_prior;

  /* Assume nothing is known about priors */
  pos_prior = 0.5;

  *num_false_positives = *num_false_negatives = 0;
  *num_positives = *num_negatives = 0;
  for (example = 0; example < num_data; example++) {
    if (READ_BITARRAY(test_members, example)) {
      if (READ_ATTRIB_B(data, example, 0) == 0) {
	(*num_negatives)++;
	*num_false_negatives += (1 - CheckCorrectness(root, data, num_data,
						      pos_prior,
						      example, ssvinfo, depth));
      } else {
	(*num_positives)++;
	*num_false_positives += (1 - CheckCorrectness(root, data, num_data,
						      pos_prior,
						      example, ssvinfo, depth));
      }
    }
  }
}


/* ----------------------------------------------------------------------

   Compute classification accuracy over a set of examples of a decision
   tree.  Returns a number in [0.0, 1.0].

   ---------------------------------------------------------------------- */

double DecisionTreeAccuracy(DTNODE *root, void **data, int num_data,
			    uchar *test_members, int num_test,
			    SSVINFO *ssvinfo)
{
  int num_correct, num_pos, num_neg;
  int example;
  double pos_prior = 0.5;

  //CountExamples(data, num_data, NULL, 0, &num_pos, &num_neg);
  //pos_prior = ((double) num_pos) / (num_pos + num_neg);
  num_correct = 0;
  for (example = 0; example < num_data; example++) {
    if (READ_BITARRAY(test_members, example)) {
      num_correct += CheckCorrectness(root, data, num_data, pos_prior,
				      example, ssvinfo, 0);
    }
  }

  return (double) num_correct / (double) num_test;
}

/* ----------------------------------------------------------------------

   Colect all target values for examples in the subtree rooted at this node,
   and return them int the char * array (targets are always discrete).  Also
   return the number of elements filled-in.  Assume that the array has
   enough space to hold all members.

   ---------------------------------------------------------------------- */

void CountDTPosNeg(DTNODE *root, int *num_pos, int *num_neg)
{
  int i;
  int num_pos_child, num_neg_child;
  
  if (root == NULL) {
    *num_pos = *num_neg = 0;
    return;
  }
  if (root->num_children == 0) {  /* Leaf. */
    *num_pos = root->num_pos;
    *num_neg = root->num_neg;
  } else {
    *num_pos = *num_neg = 0;
    for (i = 0; i < root->num_children; i++) {
      CountDTPosNeg(root->children[i], &num_pos_child, &num_neg_child);
      *num_pos += num_pos_child;
      *num_neg += num_neg_child;
    }
    root->num_pos = *num_pos;
    root->num_neg = *num_neg;
  }
}

/* ----------------------------------------------------------------------

   Post-prune the decision tree by temporarily making the current node
   a leaf and comparing the accuracy with that of the unaltered tree.

   *** YOU MUST MODIFY THIS FUNCTION *** 

   You will need to make this function recursively check *all* the nodes
   in the tree for pruning - as given, it only checks the root node.

   The two possible places to insert the recursive call are labeled below -
   you will need to experiment on both choices and explain the trade-offs involved.

   Argument structure:
   -------------------

   *root is a pointer to the root of the decision tree - used for
         evaluating the performance of the data set over the entire tree
	 
   *node is a pointer to the node currently being considered for pruning

   **data is a pointer to the entire dataset

   num_data is the number of examples in the dataset

   *pruning_set is a bitmask (over **data) indicating which examples should
                be used as the pruning dataset
		
   num_prune is the number of examples that should be used for pruning
             (equal to the number of non-zero elements in pruning_set)

   *ssvinfo stores general information about the dataset, such as the names
            of the features.

   ---------------------------------------------------------------------- */

void PruneDecisionTree(DTNODE *root, DTNODE *node,
			      void **data, int num_data,
			      uchar *pruning_set, int num_prune,
			      SSVINFO *ssvinfo)
{
  DTNODE *temp_node;
  double acc_before;
  int i;
  int save_the_children;
  double a,b,acc_after,acc_after2;

  /* Do nothing if already a leaf. */
  if (node == NULL || node->num_children == 0)
    return;


  /*******************************************************************
     You could insert the recursive call BEFORE you check the node 
  *******************************************************************/


  


  /* First, we check the accuracy of the tree assuming we keep the current node */
  acc_before = DecisionTreeAccuracy(root, data, num_data, pruning_set, num_prune, ssvinfo);

  /* Then, we "hide" the children by temporarily setting num_children to 0... */
  CountDTPosNeg(node, &node->num_pos, &node->num_neg);
  save_the_children = node->num_children;
  node->num_children = 0;

  /* ... and compute the accuracy again, while this nodes pretends to be a leaf */
  acc_after = DecisionTreeAccuracy(root, data, num_data, pruning_set, num_prune, ssvinfo);

  /* Restore the value of num_children*/
  node->num_children = save_the_children;

  /* If the new accuracy exceeds the old one by more than EPSILON, we'll prune */
  if ((acc_after-acc_before)>EPSILON) {

    /* Only print intermediate results if not in batch mode */
    if (ssvinfo->batch == 0) {
      printf("Pruning    : %s (accuracy: %g -> %g)\n",
	     ssvinfo->feat_names[node->test_attrib], acc_before, acc_after);
    }
    
    /* Actually remove the children to make this a leaf node */
    FreeDecisionTreeChildren(node);

  } else { 

    /* Only print intermediate results if not in batch mode */
    if (ssvinfo->batch == 0) {
      printf("Not pruning: %s (accuracy: %g -> %g)\n",
	     ssvinfo->feat_names[node->test_attrib], acc_before, acc_after);
    }

    /*******************************************************************
       Or you could do the recursive call AFTER you check the node
       (given that you decided to keep it)
    *******************************************************************/
  
  
  

  }

}

/**************************************************************************/
