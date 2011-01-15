/**************************************************************************
 *
 * entropy.c
 *
 * Source file containing routines related to computation of entropy.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include "auxi.h"
#include "dt.h"
#include "entropy.h"
#include "ssv.h"
#include "bitarray.h"

/* ----------------------------------------------------------------------

   Auxiliary function used by qsort to sort an array of doubles.

   ---------------------------------------------------------------------- */

static int comp_doubles(double *a, double *b)
{
  return (*a == *b) ? 0 : ((*a < *b) ? -1 : 1);
}

/* ----------------------------------------------------------------------

   Auxiliary function.  Count the positive and negative examples that are
   members of the "members" array.  If "members" is NULL, all examples are
   assumed to be members.

   ---------------------------------------------------------------------- */

void CountExamples(void **data, int num_data,
		   uchar *members, int num_members,
		   int *num_pos, int *num_neg)
{
  int memb;

  *num_pos = *num_neg = 0;
  for (memb = 0; memb < num_data; memb++) {
    if (members == NULL || READ_BITARRAY(members, memb)) {
      if (READ_ATTRIB_B(data, memb, 0) == 1)
	(*num_pos)++;
      else
	(*num_neg)++;
    }
  }
}

/* ----------------------------------------------------------------------

   Return  the log base 2 of x

   ---------------------------------------------------------------------- */

double LogBase2(double x){
  return log(x)/log(2);
}

/* ----------------------------------------------------------------------

   Given the number of positive and negative examples, calculate the
   entropy.  

   *** YOU MUST MODIFY THIS FUNCTION ***

   ---------------------------------------------------------------------- */

double Entropy(int num_pos, int num_neg) {
  
  double entropy=0.0;


  return entropy;
}

/* ----------------------------------------------------------------------

   Calculate the entropy of a data set.  Input is an array of attributes,
   their number, and a array of binary flags indicating the values that
   should be operated on.  Values for which the corresponding flag is off
   (zero) will be ignored.

   ---------------------------------------------------------------------- */

double DataEntropy(void **data, int num_data,
	       uchar *members, int num_members,
	       SSVINFO *ssvinfo)
{
  int num_pos, num_neg;

  if (num_members == 0)
    return 0.0;

  CountExamples(data, num_data, members, num_members, &num_pos, &num_neg);

  return Entropy(num_pos,num_neg);
}

/* ----------------------------------------------------------------------

   Compute the partial entropy that would result if the data set was split
   according to the binary attribute "attr".

   ---------------------------------------------------------------------- */

double PartialEntropyBinary(void **data, int num_data,
			    uchar *members, int num_members, int attr,
			    SSVINFO *ssvinfo)
{
  double partial_entropy;
  int example, num_split, val;
  uchar *split_members;

  if (num_members == 0)
    return 0.0;

  split_members = CREATE_BITARRAY(num_data);
  partial_entropy = 0.0;
  for (val = 0; val <= 1; val++) {
    num_split = 0;
    ZERO_BITARRAY(split_members, num_data);
    for (example = 0; example < num_data; example++) {
      if (READ_BITARRAY(members, example) &&
	  READ_ATTRIB_B(data, example, attr) == val) {
	WRITE_BITARRAY(split_members, example, 1);
	num_split++;
      }
    }
    partial_entropy += num_split * DataEntropy(data, num_data, split_members,
					   num_split, ssvinfo);
  }
  partial_entropy /= (double) num_members;
  free(split_members);

  return partial_entropy;
}
 
/* ----------------------------------------------------------------------

   Compute the partial entropy that would result if the data set was split
   according to the discrete (multi-valued) attribute "attr".

   ---------------------------------------------------------------------- */

double PartialEntropyDiscrete(void **data, int num_data,
			      uchar *members, int num_members, int attr,
			      SSVINFO *ssvinfo)
{
  double partial_entropy;
  int example, num_split, val;
  uchar *split_members;

  if (num_members == 0)
    return 0.0;

  split_members = CREATE_BITARRAY(num_data);
  partial_entropy = 0.0;
  for (val = 0; val < ssvinfo->num_discrete_vals[attr]; val++) {
    num_split = 0;
    ZERO_BITARRAY(split_members, num_data);
    for (example = 0; example < num_data; example++) {
      if (READ_BITARRAY(members, example) &&
	  READ_ATTRIB_I(data, example, attr) == val) {
	WRITE_BITARRAY(split_members, example, 1);
	num_split++;
      }
    }
    partial_entropy += num_split * DataEntropy(data, num_data, split_members,
					   num_split, ssvinfo);
  }
  partial_entropy /= (double) num_members;
  free(split_members);

  return partial_entropy;
}
 
/* ----------------------------------------------------------------------

   Compute the partial entropy that would result if the data set was split
   according to the continuous attribute "attr".  Return the best threshold
   value for that split also (the one that gives the maximum reduction in
   entropy).

   ---------------------------------------------------------------------- */

double PartialEntropyContinuous(void **data, int num_data,
				uchar *members, int num_members,
				int attr, double *best_threshold)
{
  int num_smaller_0, num_smaller_1;
  int num_larger_0, num_larger_1;
  int num_smaller, num_larger;
  int example, pos;
  double partial_entropy;
  double *vals = (double *) getmem(2 * num_members * sizeof(double));
  int num_vals;
  double min_partial_entropy = HUGE_VAL;
  double val;

  num_vals = 0;
  num_larger_0 = num_larger_1 = 0;
  for (example = 0; example < num_data; example++) {
    if (READ_BITARRAY(members, example)) {
      vals[num_vals++] = READ_ATTRIB_C(data, example, attr);
      val = (double) READ_ATTRIB_B(data, example, 0);
      if (val == 0.0)
	num_larger_0++;
      else 
	num_larger_1++;
      vals[num_vals++] = val;
    }
  }
  qsort(vals, num_vals / 2, 2 * sizeof(double), (int (*)()) comp_doubles);

  num_smaller_0 = num_smaller_1 = 0;
  for (pos = 0; pos <= num_vals - 4; pos += 2) {
    switch ((int) vals[pos+1]) {
    case 0:
      num_smaller_0++;
      num_larger_0--;
      break;
    case 1:
      num_smaller_1++;
      num_larger_1--;
      break;
    default:   /* Sanity check. */
      USER_ERROR2("vals[%d] contains %g (not 0 or 1)",
		  pos+1, vals[pos+1]);
    }
    if (vals[pos] == vals[pos+2] || pos > num_vals - 4)
      continue;

    /* Compute entropy for this threshold. */
    num_smaller = num_smaller_0 + num_smaller_1;
    num_larger = num_larger_0 + num_larger_1;

    partial_entropy =
      (double) num_smaller / (double) num_members *
      Entropy(num_smaller_0, num_smaller_1) +
      (double) num_larger / (double) num_members *
      Entropy(num_larger_0, num_larger_1);

    if (partial_entropy < min_partial_entropy) {
      min_partial_entropy = partial_entropy;
      *best_threshold = (vals[pos] + vals[pos+2]) / 2.0;
    }
  }

  free(vals);

  return min_partial_entropy;
}

/* ----------------------------------------------------------------------

   Return the attribute that results in the greatest information gain
   (lowest entropy).  If it is continuous, also return the best splitting
   threshold.

   ---------------------------------------------------------------------- */

int MaxGainAttribute(void **examples, int num_examples, int num_attribs,
		     uchar *members, int num_members,
		     double *best_threshold, SSVINFO *ssvinfo)
{
  double entropy_orig, new_entropy;
  int attr, max_gain_attr;
  double gain, max_gain, threshold;

  entropy_orig = DataEntropy(examples, num_examples,
			 members, num_members, ssvinfo);

  max_gain = 0.0;
  max_gain_attr = -1;
  for (attr = 1; attr < num_attribs; attr++) {
    switch (ssvinfo->types[attr]) {
    case 'b':
      new_entropy = PartialEntropyBinary(examples, num_examples,
					 members, num_members,
					 attr, ssvinfo);
      break;
    case 'd':
      new_entropy = PartialEntropyDiscrete(examples, num_examples,
					   members, num_members,
					   attr, ssvinfo);
      break;
    case 'c':
      new_entropy = PartialEntropyContinuous(examples, num_examples,
					     members, num_members,
					     attr, &threshold);
      break;
    default:
      USER_ERROR1("Unknown attribute type '%c'", ssvinfo->types[attr]);
    }
    gain = entropy_orig - new_entropy;

    /* Only print intermediate results if not in batch mode */
    /* DISABLED - uncomment this if you want to see more details
    if (ssvinfo->batch==0) {
      printf("Considered attribute \"%s\" (Gain = %g)\n",ssvinfo->feat_names[attr],gain);
    }
    */

    if (gain > max_gain) {
      max_gain = gain;
      max_gain_attr = attr;
      *best_threshold = threshold;
    }
  }

  if (max_gain<=0) {
    max_gain_attr = -1;
  }

  /* Only print intermediate results if not in batch mode */
  if (ssvinfo->batch==0) {
    if (max_gain_attr >= 1) {
      printf("Selected attribute \"%s\" (Gain = %g)",ssvinfo->feat_names[max_gain_attr],max_gain);
      if (ssvinfo->types[max_gain_attr] == 'c') {
	printf("\t(Threshold = %g)", *best_threshold);
      }
      printf("\n"); fflush(stdout);
    }  
  }

  return max_gain_attr;
}

/**************************************************************************/
