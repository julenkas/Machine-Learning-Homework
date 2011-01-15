/**************************************************************************
 *
 * print-dt.c
 *
 * Source file containing routines related printing or otherwise displaying
 * a decision tree.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include <stdio.h>
#include "ssv.h"
#include "dt.h"
#include "print-dt.h"

/* ----------------------------------------------------------------------

   Do the right amount of indentation.

   ---------------------------------------------------------------------- */

static void IndentPrint(int indent)
{
  int i;

  for (i = 0; i < indent; i++)
    printf(" ");
}

/* ----------------------------------------------------------------------

   Print a formatted section header

   ---------------------------------------------------------------------- */

void PrintSection(char *section) 
{
  printf("\n"
	 "----------------------------------------------------------\n"
	 "%s\n"
	 "----------------------------------------------------------\n\n",section);
}

/* ----------------------------------------------------------------------

   Print the stats for a decision tree.

   This does a breadth-first traversal of the tree, printing the stats
   up to a certain depth at each pass.  Finally, it just prints
   the stats for the entire tree.

   ---------------------------------------------------------------------- */

void PrintStats(DTNODE *tree, void **data, int num_data, uchar *train_members, 
		       int num_train, uchar *test_members, int num_test, SSVINFO *ssvinfo)
{
  double train_accuracy, test_accuracy;
  int depth=1, count, prev_count = 0;
  int num_negatives, num_false_negatives;
  int num_positives, num_false_positives;

  CountDTPosNeg(tree, &num_positives, &num_negatives);

  printf("-------------------------------\n"
	 "Max\t# of\tCorrect\tCorrect\n"
	 "depth\tnodes\ttrain %\ttest %\n"
	 "-------------------------------\n");
  
  count = CountNodesDepth(tree,depth);
    
  while (count != prev_count) {
    DecisionTreeAccuracyBinary(tree, data, num_data,
			       train_members, num_train,
			       train_members, num_train,
			       &num_negatives, &num_false_negatives,
			       &num_positives, &num_false_positives,
			       ssvinfo,depth);
    train_accuracy =
      (100.0 * (num_train - num_false_positives - num_false_negatives)) /
      num_train;
    if (num_test>0) {
      DecisionTreeAccuracyBinary(tree, data, num_data,
				 train_members, num_train,
				 test_members, num_test,
				 &num_negatives, &num_false_negatives,
				 &num_positives, &num_false_positives,
				 ssvinfo,depth);
      test_accuracy = 
	(100.0 * (num_test - num_false_positives - num_false_negatives)) /
	num_test;
      printf("%d\t%d\t%.1f\t%.1f\n", depth, count, train_accuracy, test_accuracy);
    } else {
      printf("%d\t%d\t%.1f\n", depth, count, train_accuracy);
    }
    prev_count = count;
    depth++;
    count = CountNodesDepth(tree,depth);
  }

  printf("-------------------------------\n");
  if (num_test>0) {
    printf("FINAL\t%d\t%.1f\t%.1f\n", count, train_accuracy, test_accuracy);
  } else {
    printf("FINAL\t%d\t%.1f\n", count, train_accuracy);
  }
  printf("-------------------------------\n");
}

/* ----------------------------------------------------------------------

   Print the decision tree.

   ---------------------------------------------------------------------- */

static void PrintDecisionTreeStructureAux(DTNODE *root, SSVINFO *ssvinfo,
					  int indent)
{
  int i, val;

  if (root == (DTNODE *) NULL) {
    IndentPrint(indent);
    printf("%s == NO : 0\n", ssvinfo->feat_names[0]);
    IndentPrint(indent);
    printf("%s == YES : 0\n", ssvinfo->feat_names[0]);
    return;
  }
  if (root->num_children == 0) {   /* Leaf. */
    IndentPrint(indent);
    printf("%s == NO : %d\n", ssvinfo->feat_names[0], root->num_neg);
    IndentPrint(indent);
    printf("%s == YES : %d\n", ssvinfo->feat_names[0], root->num_pos);
  } else {   /* Internal node. */
    switch(ssvinfo->types[root->test_attrib]) {
    case 'b':  /* Binary attribute. */
      for (i = 0; i < root->num_children; i++) {
	IndentPrint(indent);
	printf("%s : %d\n", ssvinfo->feat_names[root->test_attrib], i);
	PrintDecisionTreeStructureAux(root->children[i], ssvinfo,
				      indent + 4);
      }
      break;
    case 'd':
	for (val = 0;
	     val < ssvinfo->num_discrete_vals[root->test_attrib]; val++) {
	  IndentPrint(indent);
	  printf("%s == \"%s\"\n", ssvinfo->feat_names[root->test_attrib],
		 ssvinfo->discrete_vals[root->test_attrib][val]);
	  PrintDecisionTreeStructureAux(root->children[val], ssvinfo,
					indent + 4);
	}
	break;
    case 'c': /* Continuous attribute. */
      for (i = 0; i <= 1; i++) {
	IndentPrint(indent);
	printf("%s %s %g\n",
	       (ssvinfo->feat_names)[root->test_attrib],
	       (i == 0) ? " < " : " >= ",
	       root->threshold);
	PrintDecisionTreeStructureAux(root->children[i], ssvinfo,
				      indent + 4);
      }
      break;
    default:
      USER_ERROR1("type unknown ('%c')",
		  (ssvinfo->types)[root->test_attrib]);
    }
  }
}

void PrintDecisionTreeStructure(DTNODE *root, SSVINFO *ssvinfo)
{
  PrintDecisionTreeStructureAux(root, ssvinfo, 0);
  printf("\n");
}

/* ----------------------------------------------------------------------

   Print a series of tests leading to each leaf, together with the
   probability of that leaf at the end.

   ---------------------------------------------------------------------- */

static void PrintAllPathsAux(DTNODE *root, SSVINFO *ssvinfo,
				 char **prefix)
{
  int val, num_branches;

  if (root == (DTNODE *) NULL)
    return;
  if (root->num_children == 0) {
    printf("%s : ", *prefix);
    printf(" %s: YES %d, NO %d\n", (ssvinfo->feat_names)[0],
	   root->num_pos, root->num_neg);
  } else {
    char *feat_name = (ssvinfo->feat_names)[root->test_attrib];
    int l = strlen(*prefix);
    *prefix = (char *) realloc(*prefix, l + 100);
    switch((ssvinfo->types)[root->test_attrib]) {
    case 'b':
      sprintf(*prefix + l, "( %s == 0 ) ? ", feat_name);
      PrintAllPathsAux(root->children[0], ssvinfo, prefix);
      sprintf(*prefix + l, "( %s == 1 ) ? ", feat_name);
      PrintAllPathsAux(root->children[1], ssvinfo, prefix);
      break;
    case 'd':
      num_branches = ssvinfo->num_discrete_vals[root->test_attrib];
      for (val = 0; val < num_branches; val++) {
	sprintf(*prefix + l, "( %s == \"%s\" ) ? ", feat_name,
		ssvinfo->discrete_vals[root->test_attrib][val]);
	PrintAllPathsAux(root->children[val], ssvinfo, prefix);
      }
      break;
    case 'c':
      sprintf(*prefix + l, "( %s < %g ) ? ", feat_name, root->threshold);
      PrintAllPathsAux(root->children[0], ssvinfo, prefix);
      sprintf(*prefix + l, "( %s >= %g ) ? ", feat_name, root->threshold);
      PrintAllPathsAux(root->children[1], ssvinfo, prefix);
      break;
    default:
	USER_ERROR1("type unknown ('%c')",
		    (ssvinfo->types)[root->test_attrib]);
    }
  }
}

void PrintAllPaths(DTNODE *root, char *filename, SSVINFO *ssvinfo)
{
  char *prefix = (char *) getmem(10240);

  if (filename != NULL)
    printf("\nBEGIN DECISION-TREE %s\n\n",
	   filename);
  strcpy(prefix, "");
  PrintAllPathsAux(root, ssvinfo, &prefix);
  printf("\nEND DECISION-TREE\n");
  free(prefix);
}

/**************************************************************************/
