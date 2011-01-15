/**************************************************************************
 *
 * dt.c
 *
 * Source file containing routines related to the creation and query of a
 * decision tree.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include "dt.h"
#include "prune-dt.h"
#include "main.h"
#include "bitarray.h"
#include "ssv.h"
#include "entropy.h"

/* ----------------------------------------------------------------------

   Create a decision tree based on the examples that contain floating point
   or discrete (multi-valued) or binary attributes.  Split the examples into
   three sets: training, pruning and testing.  Training is used to grow the
   tree, pruning to post-prune it and test is not used, but a membership
   array of flags is returned (for accuracy testing perhaps by the caller).

   ---------------------------------------------------------------------- */

DTNODE *CreateDecisionTree(void **data, int num_data, int num_features,
			   double approx_prune_pct, double approx_test_pct,
			   uchar *train_members, int num_train,
			   SSVINFO *ssvinfo)
{
  DTNODE *root;

  /* Call the auxiliary recursive subroutine to create the tree. */
  root = CreateDecisionTreeAux(data, num_data, train_members, num_train,
			       num_features, ssvinfo);

  return root;
}

/* ......................................................................

   Create a decision subtree having a root test on the binary-valued
   attribute "attr".

   ...................................................................... */

DTNODE *CreateDecisionSubTreeBinary(void **data, int num_data,
				    uchar *train_members, int num_train,
				    int num_features, int attr,
				    SSVINFO *ssvinfo)
{
  int val;
  uchar *members_temp;
  int memb, num_members_temp;
  DTNODE *node;

  /* Allocate space for a temporary copy of the members array. */
  members_temp = CREATE_BITARRAY(num_data);

  node = (DTNODE *) getmem(sizeof(DTNODE));
  node->children = (DTNODE **) getmem(2 * sizeof(DTNODE *));
  node->test_attrib = attr;
  node->num_children = 2;

  for (val = 0; val <= 1; val++) {
    num_members_temp = 0;
    ZERO_BITARRAY(members_temp, num_data);
    for (memb = 0; memb < num_data; memb++) {
      if (READ_BITARRAY(train_members, memb) &&
	  READ_ATTRIB_B(data, memb, attr) == val) {
	WRITE_BITARRAY(members_temp, memb, 1);
	num_members_temp++;
      }
    }
    if (num_members_temp == num_train || num_members_temp == 0) {
      /* Create leaf node. */
      free(node->children);
      node->children = (DTNODE **) NULL;
      node->num_children = 0;
      node->test_attrib = 0;
      CountExamples(data, num_data, train_members, num_train,
		    &(node->num_pos), &(node->num_neg));
      break;
    } else {
      node->children[val] =
	CreateDecisionTreeAux(data, num_data, members_temp,
			      num_members_temp, num_features,
			      ssvinfo);
    }
  }
  node->num_members = num_train;
  free(members_temp);

  return node;
}

/* ......................................................................

   Create a decision subtree having a root test on the binary-valued
   attribute "attr".

   ...................................................................... */

DTNODE *CreateDecisionSubTreeDiscrete(void **data, int num_data,
				      uchar *train_members, int num_train,
				      int num_features, int attr,
				      SSVINFO *ssvinfo)
{
  int val;
  uchar *members_temp;
  int memb, num_members_temp;
  DTNODE *node;
  int num_branches = ssvinfo->num_discrete_vals[attr];

  /* Allocate space for a temporary copy of the members array. */
  members_temp = CREATE_BITARRAY(num_data);

  node = (DTNODE *) getmem(sizeof(DTNODE));
  node->children = (DTNODE **) getmem(num_branches * sizeof(DTNODE *));
  node->test_attrib = attr;
  node->num_children = num_branches;

  for (val = 0; val < num_branches; val++) {
    num_members_temp = 0;
    ZERO_BITARRAY(members_temp, num_data);
    for (memb = 0; memb < num_data; memb++) {
      if (READ_BITARRAY(train_members, memb) &&
	  READ_ATTRIB_I(data, memb, attr) == val) {
	WRITE_BITARRAY(members_temp, memb, 1);
	num_members_temp++;
      }
    }
    if (num_members_temp == num_train) {
      /* Create leaf node. */
      node->children = (DTNODE **) NULL;
      node->num_children = 0;
      node->test_attrib = 0;
      CountExamples(data, num_data, train_members, num_train,
		    &(node->num_pos), &(node->num_neg));
      break;
    } else {
      node->children[val] =
	CreateDecisionTreeAux(data, num_data, members_temp,
			      num_members_temp, num_features,
			      ssvinfo);
    }
  }
  node->num_members = num_train;
  free(members_temp);

  return node;
}

/* ......................................................................

   Create a decision subtree having a root test on the continuous-valued
   attribute "attr".

   ...................................................................... */

DTNODE *CreateDecisionSubTreeContinuous(void **data, int num_data,
					uchar *train_members, int num_train,
					int num_features,
					int attr, double threshold,
					SSVINFO *ssvinfo)
{
  int memb, num_members_smaller, num_members_larger;
  uchar *members_smaller, *members_larger;
  DTNODE *node;

  /* Allocate space for a temporary copy of the members array. */
  members_smaller = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(members_smaller, num_data);
  members_larger = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(members_larger, num_data);

  /* Split node recursively according to threshold. */
  node = (DTNODE *) getmem(sizeof(DTNODE));
  node->children = (DTNODE **) getmem(2 * sizeof(DTNODE *));
  node->test_attrib = attr;
  node->threshold = threshold;
  node->num_children = 2;
  node->num_members = num_train;

  /* Split elements into smaller and larger or equal to the threshold. */
  num_members_smaller = num_members_larger = 0;
  for (memb = 0; memb < num_data; memb++) {
    if (READ_BITARRAY(train_members, memb)) {
      if (READ_ATTRIB_C(data, memb, attr) < threshold) {
	WRITE_BITARRAY(members_smaller, memb, 1);
	num_members_smaller++;
      } else {
	WRITE_BITARRAY(members_larger, memb, 1);
	num_members_larger++;
      }
    }
  }

  if (num_members_smaller == 0 || num_members_larger == 0) {
    /* Create leaf node. */
    free(node->children);
    node->children = (DTNODE **) NULL;
    node->num_children = 0;
    node->test_attrib = 0;
    CountExamples(data, num_data, train_members, num_train,
		  &(node->num_pos), &(node->num_neg));
  } else {
    node->children[0] =
      CreateDecisionTreeAux(data, num_data,
			    members_smaller, num_members_smaller,
			    num_features, ssvinfo);
    node->children[1] =
      CreateDecisionTreeAux(data, num_data,
			    members_larger, num_members_larger,
			    num_features, ssvinfo);
  }
  free(members_smaller);
  free(members_larger);

  return node;
}

/* ......................................................................

   Create a decision tree based on the examples that contain floating point
   or discrete (multi-valued) or binary attributes and multi-valued
   predicted attribute.

   ...................................................................... */

DTNODE *CreateDecisionTreeAux(void **data, int num_data,
			      uchar *train_members, int num_train,
			      int num_features, SSVINFO *ssvinfo)
{
  DTNODE *node = NULL;
  int min_gain_attr;
  double best_threshold;
  int create_leaf_node;

  if (num_train == 0)
    return (DTNODE *) NULL;

  create_leaf_node = 0;
  /* Check if all examples belong to the same class. */
  if (num_train <= MIN_LEAF_MEMBERS) {
    create_leaf_node = 1;
  } else {  /* Else split and recurse. */
    min_gain_attr = MaxGainAttribute(data, num_data, num_features,
				     train_members, num_train,
				     &best_threshold, ssvinfo);
    if (min_gain_attr == -1) {
      create_leaf_node = 1;
    } else {
      switch((ssvinfo->types)[min_gain_attr]) {
      case 'b': /* Binary min-gain attribute. */
	node = CreateDecisionSubTreeBinary(data, num_data,
					   train_members, num_train,
					   num_features, min_gain_attr,
					   ssvinfo);
	break;
      case 'd':
	node = CreateDecisionSubTreeDiscrete(data, num_data,
					     train_members, num_train,
					     num_features, min_gain_attr,
					     ssvinfo);
	break;
      case 'c':
	node = CreateDecisionSubTreeContinuous(data, num_data,
					       train_members, num_train,
					       num_features, min_gain_attr,
					       best_threshold, ssvinfo);
	break;
      default:
	USER_ERROR1("type unknown ('%c')", ssvinfo->types[min_gain_attr]);
      }
    }
  }

  /* Create leaf node. */
  if (create_leaf_node) {
    node = (DTNODE *) getmem(sizeof(DTNODE));
    node->num_children = 0;
    node->children = (DTNODE **) NULL;
    node->test_attrib = 0;
    CountExamples(data, num_data, train_members, num_train,
		  &(node->num_pos), &(node->num_neg));
    node->num_members = num_train;
  }

  return node;
}

/* ----------------------------------------------------------------------

   Free all memory associated with a node of the decision tree.

   ---------------------------------------------------------------------- */

void FreeDecisionTreeNode(DTNODE *node)
{
  if (node == NULL)
    return;
  if (node->num_children == 0) {
    free(node);
  } else {
/*     free(node->test_attrib_name); */
  }
}

/* ----------------------------------------------------------------------

   Free all memory associated with the children of the decision tree.

   ---------------------------------------------------------------------- */

void FreeDecisionTreeChildren(DTNODE *node)
{
  int i;

  if (node == NULL)
    return;
  for (i = 0; i < node->num_children; i++)
    FreeDecisionTree(node->children[i]);
  node->num_children = 0;
}

/* ----------------------------------------------------------------------

   Free all memory associated with a decision tree.

   ---------------------------------------------------------------------- */

void FreeDecisionTree(DTNODE *root)
{
  if (root == NULL)
    return;
  FreeDecisionTreeChildren(root);
  FreeDecisionTreeNode(root);
}

/* ----------------------------------------------------------------------

   Count the number of nodes of a decision tree.

   ---------------------------------------------------------------------- */

int CountNodesDepth(DTNODE *root, int depth)
{
  int i, nodes = 1;

  if ((root == NULL) || (depth == 1))
    return 1;
  for (i = 0; i < root->num_children; i++)
    nodes += CountNodesDepth(root->children[i],depth-1);

  return nodes;
}
/* ----------------------------------------------------------------------

   Count the number of nodes of a decision tree.

   ---------------------------------------------------------------------- */

int CountNodes(DTNODE *root)
{
  int i, nodes = 1;

  if (root == NULL)
    return 1;
  for (i = 0; i < root->num_children; i++)
    nodes += CountNodes(root->children[i]);

  return nodes;
}

/**************************************************************************/
