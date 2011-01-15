/**************************************************************************
 *
 * ssv.h
 *
 * Header file to ssv.c
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#ifndef SSV_H
#define SSV_H 1

#define SKIPSPACE(ptr) {					\
  while (*(ptr) == ' ' || *(ptr) == '\t' || *(ptr) == '\n')	\
    (ptr)++;							\
}
#define SKIPWORD(ptr) {						\
  SKIPSPACE(ptr)						\
  while (*(ptr) != ' ' && *(ptr) != '\t' && *(ptr) != '\n')	\
    (ptr)++;							\
  SKIPSPACE(ptr)						\
}

/* Read a binary (0/1) value. */
#define READ_ATTRIB_B(data, example, feature)	\
  READ_BITARRAY(data[feature], example)

/* Read a double. */
#define READ_ATTRIB_C(data, example, feature)	\
  (((double *) (data)[feature])[example])

/* Read an integer. */
#define READ_ATTRIB_I(data, example, feature)	\
  (((int *) (data)[feature])[example])

/* Structure holding information about the SSV file. */
typedef struct ssvinfo {
  char *types;             /* Types of every feature (column) of the SSV
			      file.  Each type is either 'b' (binary, 0/1),
			      'd' (discrete, each corresponding to a
			      string), or 'c' (continuous, a double
			      float). */
  char **feat_names;       /* This array holds the name of every
			      attribute (every column) as specified in the
			      SSV file.  It is an array of pointers to
			      strings, one for each attribute. */
  char ***discrete_vals;   /* This holds the names of each discrete value,
			      as specified in the SSV file e.g. "sunny",
			      "cloudy", "rainy" can be the set of possible
			      values of the discrete-valued attribute
			      "weather".  The code actually uses an integer
			      for each such value when doing comparisons.
			      This is and array of pointers to arrays of
			      string pointers (sorry).  Binary and
			      continuous attributes have NULL at their
			      entry.*/
  int *num_discrete_vals;  /* The number of discrete values, as contained in
			      discrete_vals[i].  0 fir binary and continuous
			      attributes. */
  int batch;               /* the number of times to repeat the dt learner */
} SSVINFO;

#include "auxi.h"
#include "dt.h"

/* Function prototypes. */
void **ReadTPT(char *train_filename, char *prune_filename, char *test_filename,
               uchar **train_members_ptr, uchar **prune_members_ptr, uchar **test_members_ptr,
               int *num_train_ptr, int *num_prune_ptr, int *num_test_ptr,
               int *num_data_ptr, int *num_features_ptr, SSVINFO *ssvinfo);
void **ReadTwo(char *train_filename, char *prune_filename,
               uchar **train_members_ptr, uchar **prune_members_ptr,
               int *num_train_ptr, int *num_prune_ptr,
               int *num_data_ptr, int *num_features_ptr, SSVINFO *ssvinfo);
void **MergeSSVFile(int num_data_A, int num_data_B, 
                    SSVINFO *ssvinfo_A, SSVINFO *ssvinfo_B,
                    void **data_A, void **data_B,
                    SSVINFO *ssvinfo_result,
                    int num_features);
void **ReadSSVFile(char *filename, int *num_data_ptr,
		   int *num_features_ptr, SSVINFO *ssvinfo);
unsigned char read_attrib_b(void **data, int example, int feature);
void write_attrib_b(void **data, int example, int feature,
		    unsigned char val);
int read_attrib_i(void **data, int example, int feature);
void write_attrib_i(void **data, int example, int feature, int val);
double read_attrib_c(void **data, int example, int feature);
void write_attrib_c(void **data, int example, int feature, double val);
void PartitionExamples(void **data, int *num_data_ptr, int num_features,
		       uchar **train_members_ptr, int *num_train_ptr,
		       uchar **test_members_ptr, int *num_test_ptr,
		       uchar **prune_members_ptr, int *num_prune_ptr,
		       double train_pct, double prune_pct, double test_pct,
		       SSVINFO *ssvinfo);
#endif // SSV_H
/**************************************************************************/
