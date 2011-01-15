/**************************************************************************
 *
 * main.c
 *
 * Main source file.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "dt.h"
#include "prune-dt.h"
#include "print-dt.h"
#include "ssv.h"
#include "bitarray.h"
#include "main.h"

#define USAGE "\nProduce a decision tree for a set of attributes.\n\n"	 \
              "Usage: %s [ -s <seed> | -b <number>] "		         \
	      "<train %%> <prune %%> <test %%> "			 \
              "<filename>\n\n"                                           \
              "OR\n\n"		                        	         \
              "%s [-tpt <trainfile> <prunefile> <testfile> | "           \
              "-tp <trainfile> <prunefile> | "                           \
              "-tt <trainfile> <testfile>]\n\n"                          \
	      "(Note: the random seed is taken from the computer clock " \
	      "if not specified.)\n\n"

/* Global variables. */
char *progname;

/* ----------------------------------------------------------------------

   Auxiliary function, a wrapper around strdup() really that aborts the
   program if there's not enough memory.

   ---------------------------------------------------------------------- */

char *my_strdup(char *s)
{
  char *ptr;

  if ((ptr = strdup(s)) == NULL)
    SYS_ERROR1("can't allocate %d byte(s).\n", strlen(s));

  return ptr;
}

/* ----------------------------------------------------------------------

   Same as my_strdup() inly the number of characters to dplicate is
   specified.  The actual string allocated has length "n"+1.

   ---------------------------------------------------------------------- */

char *my_strndup(char *s, int n)
{
  char *ptr;

  ptr = getmem(n + 1);
  strncpy(ptr, s, n);
  ptr[n] = '\0';
  return ptr;
}


/* Calculates the mean and the standard deviation of the sample mean *
 *                                                                   *
 * Observe that there is a difference between the standard deviation *
 * of the sample, which is an unbiased estimator of the standard     *
 * deviation of the underlying distribution, and the standard        *
 * deviation of the sample mean, which is the standard deviation of  *
 * the sample mean over several random samples.                      *
 *                                                                   *
 * The latter is a measure of how much variance one would expect to  *
 * see in our measurement of the sample mean if we ran the program   *
 * several times.                                                    */
void CalculateMeanStandardDeviation(double *data, int number, double *mean_ptr, double *stddev_ptr){
  double sum = 0, sum_squared_difference = 0;
  double sample_var = 0, mean_var = 0;
  double mean = 0, stddev = 0;
  int i;

  for(i=0;i<number;i++){
    sum += data[i];
  }

  mean = sum/number;

  for(i=0;i<number;i++){
    sum_squared_difference += (data[i]-mean)*(data[i]-mean);
  }
  
  sample_var = sum_squared_difference/(number-1);

  mean_var = sample_var/(number-1);
  stddev = sqrt(mean_var);

  *mean_ptr = mean;
  *stddev_ptr = stddev; 
}

void BatchMain(void **data, int num_data, int num_features, 
	       double train_pct, double prune_pct, double test_pct, SSVINFO *ssvinfo)
{
  DTNODE *tree;
  uchar *test_members, *train_members, *prune_members;
  int num_test, num_train, num_prune;
  int num_negatives, num_false_negatives;
  int num_positives, num_false_positives;
  int i;
  double train_accuracy = 0, test_accuracy = 0;
  double count_mean, count_stddev;
  double train_mean, train_stddev;
  double test_mean, test_stddev;

  double *train_list, *test_list;
  int count;
  double *count_list;

  count_list = (double *) getmem(ssvinfo->batch * sizeof(double));
  train_list = (double *) getmem(ssvinfo->batch * sizeof(double));
  test_list = (double *) getmem(ssvinfo->batch * sizeof(double));

  for (i=0; i<ssvinfo->batch; i++) {

    /* Partition examples in train, test and prune sets. */
    PartitionExamples(data, &num_data, num_features,
		      &train_members, &num_train,
		      &test_members, &num_test,
		      &prune_members, &num_prune,
		      train_pct, prune_pct, test_pct,
		      ssvinfo);

    if (num_train == 0) {
      fprintf(stderr, "%s: no examples to train on!\n", progname);
      exit(1);
    }
    
    tree = CreateDecisionTree(data, num_data, num_features, prune_pct, test_pct,
			      train_members, num_train, ssvinfo);
    
    
    /* Post-prune the decision tree. */
    if (num_prune > 0) {
      PruneDecisionTree(tree, tree, data, num_data,
			       prune_members, num_prune, ssvinfo);
    }
    
    count = CountNodes(tree);
    count_list[i] = count;
    /* count_sum += count; */

    DecisionTreeAccuracyBinary(tree, data, num_data, train_members, num_train, train_members, 
			       num_train, &num_negatives, &num_false_negatives,
			       &num_positives, &num_false_positives, ssvinfo, 0);
    train_accuracy = (100.0 * (num_train - num_false_positives - num_false_negatives))/num_train;
    train_list[i] = train_accuracy;
    /* train_sum += train_accuracy; */

    if (num_test>0) {
      DecisionTreeAccuracyBinary(tree, data, num_data, train_members, num_train, test_members, 
				 num_test, &num_negatives, &num_false_negatives,
				 &num_positives, &num_false_positives, ssvinfo, 0);
      test_accuracy = (100.0 * (num_test - num_false_positives - num_false_negatives))/num_test;
    }
    test_list[i] = test_accuracy;
    /* test_sum += test_accuracy; */
    
    free(train_members);
    free(test_members);
    free(prune_members);
  }

  CalculateMeanStandardDeviation(count_list,ssvinfo->batch,&count_mean,&count_stddev);
  CalculateMeanStandardDeviation(train_list,ssvinfo->batch,&train_mean,&train_stddev);
  CalculateMeanStandardDeviation(test_list,ssvinfo->batch,&test_mean,&test_stddev);

  printf("----------------------------------------------\n");
  printf("#nodes\t#nodes\ttrain%%\ttrain%%\ttest%%\ttest%%\n");
  printf("mean\tstd\tmean\tstd\tmean\tstd\n");
  printf("----------------------------------------------\n");
  printf("%6.2lf\t%6.2lf\t%6.2lf\t%6.2lf\t%6.2lf\t%6.2lf\n",
	 count_mean, count_stddev, train_mean, train_stddev, test_mean, test_stddev);
  printf("----------------------------------------------\n");

  free(count_list);
  free(train_list);
  free(test_list);
  
}

/* ----------------------------------------------------------------------

   Main function.

   ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
  char *data_filename, *deref_filename;
  double train_pct, prune_pct, test_pct;
  double train_accuracy, test_accuracy;
  DTNODE *tree;
  uchar *test_members, *train_members, *prune_members;
  int multiple_input_files;
  char *train_filename, *prune_filename, *test_filename;
  int num_test, num_train, num_prune, num_features, num_data;
  int depth, count, prev_count;
  int num_negatives, num_false_negatives;
  int num_positives, num_false_positives;
  void **data;
  struct timeval tv;
  unsigned int random_seed;
  SSVINFO ssvinfo;

  ssvinfo.batch = 0;

  progname = (char *) rindex(argv[0], '/');
  argv[0] = progname = (progname != NULL) ? (progname + 1) : argv[0];

  multiple_input_files = 0;
  if (argc>2){
    if (!strcmp(argv[1],"-tpt") && (argc==5)){
      train_filename = argv[2];
      prune_filename = argv[3];
      test_filename = argv[4];
      data = ReadTPT(train_filename, prune_filename, test_filename,
                     &train_members, &prune_members, &test_members,
                     &num_train, &num_prune, &num_test,
                     &num_data, &num_features, &ssvinfo);
      multiple_input_files = 1;
    } else if (!strcmp(argv[1],"-tp") && (argc==4)){
      train_filename = argv[2];
      prune_filename = argv[3];
      data = ReadTwo(train_filename, prune_filename,
		     &train_members, &prune_members,
                     &num_train, &num_prune,
                     &num_data, &num_features, &ssvinfo);
      num_test = 0;
      test_members = CREATE_BITARRAY(num_data);
      ZERO_BITARRAY(test_members,num_data);
      multiple_input_files = 1;
    } else if (!strcmp(argv[1],"-tt")&& (argc==4)){
      train_filename = argv[2];
      test_filename = argv[3];
      data = ReadTwo(train_filename, test_filename,
		     &train_members, &test_members,
                     &num_train, &num_test,
                     &num_data, &num_features, &ssvinfo);
      num_prune = 0;
      prune_members = CREATE_BITARRAY(num_data);
      ZERO_BITARRAY(prune_members,num_data);
      multiple_input_files = 1;
    }
  }

  if (!multiple_input_files){
    if (argc != 5 && argc != 7) {
      fprintf(stderr, USAGE, progname);
      exit(1);
    }
    switch (argc) {
    case 5:
      if (gettimeofday(&tv, NULL) == -1)
	SYS_ERROR1("gettimeofday(%s)", "");
      random_seed = (unsigned int) tv.tv_usec;
      train_pct = atof(argv[1]);
      prune_pct = atof(argv[2]);
      test_pct = atof(argv[3]);
      data_filename = argv[4];
      break;
    case 7:
      if ((argv[1][1] == 's') || (argv[1][1] == 'S')) {
	random_seed = atoi(argv[2]);
      } else if ((argv[1][1] == 'b') || (argv[1][1] == 'B')) {
	if (gettimeofday(&tv, NULL) == -1)
	  SYS_ERROR1("gettimeofday(%s)", "");
	random_seed = (unsigned int) tv.tv_usec;
	ssvinfo.batch = atoi(argv[2]);
      } else {
	fprintf(stderr, USAGE, progname);
	exit(1);
      }
      train_pct = atof(argv[3]);
      prune_pct = atof(argv[4]);
      test_pct = atof(argv[5]);
      data_filename = argv[6];
      break;
    default:
      fprintf(stderr, USAGE, progname);
      exit(1);
    }
    if ((random_seed < 0) ||
	(train_pct <= 0.0) || (train_pct > 1.0) ||
	(prune_pct < 0.0) || (prune_pct > 1.0) ||
	(test_pct < 0.0) || (test_pct > 1.0) ||
	(train_pct + prune_pct + test_pct > 1.00000001)) {
      fprintf(stderr, USAGE, progname);
      exit(1);
    }

    /* Memory-map the examples file. */
    data = ReadSSVFile(data_filename, &num_data, &num_features, &ssvinfo);
    
    /* Initialize random number generator. */
    srandom(random_seed);

    if (ssvinfo.batch>0) {
      BatchMain(data, num_data, num_features, train_pct, prune_pct, test_pct, &ssvinfo);
      exit(0);
    } 

    /* Partition examples in train, test and prune sets. */
    PartitionExamples(data, &num_data, num_features,
		      &train_members, &num_train,
		      &test_members, &num_test,
		      &prune_members, &num_prune,
		      train_pct, prune_pct, test_pct,
		      &ssvinfo);

    /* Print the program arguments */
    PrintSection("Program arguments");

    printf("Random seed:    %u\n", random_seed);
    printf("Training:       %.0f%% (%d examples)\n", train_pct * 100.0, num_train);
    printf("Pruning:        %.0f%% (%d examples)\n", prune_pct * 100.0, num_prune);
    printf("Testing:        %.0f%% (%d examples)\n", test_pct * 100.0, num_test);
    printf("Data filename:  %s\n", data_filename);
  }

  if (num_train == 0) {
    fprintf(stderr, "%s: no examples to train on!\n", progname);
    exit(1);
  }
  
  /* Create a decision tree and print it */
  PrintSection("Growing decision tree");

  tree = CreateDecisionTree(data, num_data, num_features, prune_pct, test_pct,
			    train_members, num_train, &ssvinfo);


  //PrintSection("Printing decision tree");
  //PrintDecisionTreeStructure(tree, &ssvinfo);

  PrintSection("Computing decision tree statistics");
  PrintStats(tree, data, num_data, train_members, num_train, test_members, num_test, &ssvinfo);

  /* Post-prune the decision tree. */
  if (num_prune > 0) {

    PrintSection("Pruning decision tree");

    prev_count = CountNodes(tree);

    PruneDecisionTree(tree, tree, data, num_data,
			     prune_members, num_prune, &ssvinfo);

    count = CountNodes(tree);
    
    /* If the node count decreased, something must have been pruned */

    if (count < prev_count) {
      
      printf("\nPruning reduced the tree size from %d to %d nodes\n",prev_count,count);

      //PrintSection("Printing PRUNED decision tree");
      //PrintDecisionTreeStructure(tree, &ssvinfo);

    } else {
      
      printf("\nPruning did not remove any nodes\n");
    
    }

    /* Print the statistics again, for comparison */

    PrintSection("Computing decision tree statistics after pruning");  
    PrintStats(tree, data, num_data, train_members, num_train, test_members, num_test, &ssvinfo);

  }

  free(train_members);
  free(test_members);
  free(prune_members);
  exit(0);
}

/**************************************************************************/
