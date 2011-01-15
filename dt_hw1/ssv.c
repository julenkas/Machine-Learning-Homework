/**************************************************************************
 *
 * ssv.c
 *
 * Source file containing routines related to reading an SSV-format file.
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <search.h>
#include <malloc.h>
#include "ssv.h"
#include "main.h"

/* ----------------------------------------------------------------------

   Auxiliary mini-functions.

   ---------------------------------------------------------------------- */

/* Read a binary (0/1) value. */
unsigned char read_attrib_b(void **data, int example, int feature)
{
  return READ_BITARRAY(data[feature], example);
}
/* Write a binary (0/1) value. */
void write_attrib_b(void **data, int example, int feature, unsigned char val)
{
  WRITE_BITARRAY(data[feature], example, val);
}

/* Read an integer. */
int read_attrib_i(void **data, int example, int feature)
{
  return ((int *) data[feature])[example];
}
/* Write an integer. */
void write_attrib_i(void **data, int example, int feature, int val)
{
  ((int *) data[feature])[example] = val;
}

/* Read a double. */
double read_attrib_c(void **data, int example, int feature)
{
  return ((double *) data[feature])[example];
}
/* Write a double. */
void write_attrib_c(void **data, int example, int feature, double val)
{
  ((double *) data[feature])[example] = val;
}

/* Read of any type.  Check the feature's type and make the appropriate
   call. */
void read_attrib(void **data, int example, int feature, char *types,
		 void *result)
{
  switch (types[feature]) {
  case 'b':
    *((unsigned char *) result) = read_attrib_b(data, example, feature);
    break;
  case 'd':
    *((int *) result) = read_attrib_i(data, example, feature);
    break;
  case 'c':
    *((double *) result) = read_attrib_c(data, example, feature);
    break;
  default:
    USER_ERROR1("unknown type '%c'", types[feature]);
  }
}
/* Write of any type.  Check the feature's type and make the appropriate
   call. */
void write_attrib(void **data, int example, int feature, char *types,
		  void *val)
{
  switch (types[feature]) {
  case 'b':
    write_attrib_b(data, example, feature, *((unsigned char *) val));
    break;
  case 'd':
    write_attrib_i(data,example, feature, *((int *) val));
    break;
  case 'c':
    write_attrib_c(data,example, feature, *((double *) val));
    break;
  default:
    USER_ERROR1("unknown type '%c'", types[feature]);
  }
}

/* ----------------------------------------------------------------------

   Partition the input data into three sets.  Split the set of examples in
   test, prune and train sets according to approx_test_pct, approx_prune_pct
   and (1 - approx_prune_pct - approx_test_pct) fractions respectively.
   After that equalize the number of positive and negative examples in all
   train, test and pruning sets by duplication.

   ---------------------------------------------------------------------- */

void PartitionExamples(void **data, int *num_data_ptr, int num_features,
		       uchar **train_members_ptr, int *num_train_ptr,
		       uchar **test_members_ptr, int *num_test_ptr,
		       uchar **prune_members_ptr, int *num_prune_ptr,
		       double train_pct, double prune_pct, double test_pct,
		       SSVINFO *ssvinfo)
{
  uchar *train_members, *test_members, *prune_members;
  uchar *assigned;
  int num_train, num_test, num_prune;
  int example, num_data = *num_data_ptr;
  int i, j, idx, num_not_assigned;

  /* Keep track of the examples that are assinged to one of the sets: train,
     prune and test. */
  assigned = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(assigned, num_data);
  num_not_assigned = num_data;

  /* --- Training examples. --- */
  num_train = (int) rint(num_data * train_pct);
  train_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(train_members, num_data);
  for (example = 0; (example < num_train) && (num_not_assigned>0); example++) {
    /* Assign one of the unassigned examples. */
    idx = (int) uniform(0.0, (double) num_not_assigned);
    for (i = j = 0; j < idx || READ_BITARRAY(assigned, i); i++) 
      if (!READ_BITARRAY(assigned, i))
	j++;
    WRITE_BITARRAY(train_members, i, 1);
    WRITE_BITARRAY(assigned, i, 1);
    num_not_assigned--;
  }
  num_train = example;
  /* --- Test examples. --- */
  num_test = (int) rint(num_data * test_pct);
  test_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(test_members, num_data);
  for (example = 0; (example < num_test) && (num_not_assigned>0); example++) {
    /* Assign one of the unassigned examples. */
    idx = (int) uniform(0.0, (double) num_not_assigned);
    for (i = j = 0; j < idx || READ_BITARRAY(assigned, i); i++)
      if (!READ_BITARRAY(assigned, i))
	j++;
    WRITE_BITARRAY(test_members, i, 1);
    WRITE_BITARRAY(assigned, i, 1);
    num_not_assigned--;
  }
  num_test = example;
  /* --- Pruning examples. --- */
  num_prune = (int) rint(num_data * prune_pct);
  prune_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(prune_members, num_data);
  for (example = 0; (example < num_prune) && (num_not_assigned>0); example++) {
    /* Assign one of the unassigned examples. */
    idx = (int) uniform(0.0, (double) num_not_assigned);
    for (i = j = 0; j < idx || READ_BITARRAY(assigned, i); i++)
      if (!READ_BITARRAY(assigned, i))
	j++;
    WRITE_BITARRAY(prune_members, i, 1);
    WRITE_BITARRAY(assigned, i, 1);
    num_not_assigned--;
  }
  num_prune = example;

  free(assigned);

  *train_members_ptr = train_members; *num_train_ptr = num_train;
  *test_members_ptr = test_members; *num_test_ptr = num_test;
  *prune_members_ptr = prune_members; *num_prune_ptr = num_prune;
  
}

/* ----------------------------------------------------------------------

   Read an ssv file and construct an array of pointers to the data contained
   in it.  Each of the pointers points to a suitable type array of elements,
   and each array corresponds to an array of values of one of the
   attributes.

   ---------------------------------------------------------------------- */

#define TEMP_STR_SIZE		32768
#define DEFAULT_DATA_SIZE	65536

char *clean_str(char *str) {

  int   i, len;
  char *new_str;
  
  if (str == NULL)
    return NULL;

  len = strlen (str);
  new_str = str;

  /* remove the leading white space */
  for (i = 0; i < len; i++, new_str++)
    if (!isspace(*new_str))
      break;

  /* remove the trailing white space */
  for (i = len - 1; i >= 0; str[i] = '\0', i--)
    if (!isspace(str[i]))
      break;

  return new_str;
}

char *fgets_clean_eof(char *s, FILE *stream) {
  char *ret_str;
  while (1) {
    if (fgets (s, TEMP_STR_SIZE, stream) == NULL)
      return NULL;
    s[TEMP_STR_SIZE-1] = '\0';
    if (s[0] == '#')
      continue;
    ret_str = clean_str(s);
    if (ret_str[0] != '\0')
      break;
  }
  return ret_str;
}

char *fgets_clean(char *s, FILE *stream) {
  char *retval = fgets_clean_eof(s, stream);
  if (retval == NULL)
    USER_ERROR1("input file terminated permaturely%s", "");
  return retval;
}

char *next_word(char **str) {

  int   i, len;
  char *ret_str;
  
  if (*str == NULL)
    return NULL;

  len = strlen (*str);

  /* skip leading white space */
  for (i = 0; i < len; i++, (*str)++)
    if (!isspace(**str))
      break;
  ret_str = *str;
  
  /* terminate the string */
  for (; i < len; i++, (*str)++)
    if (isspace(**str)) {
      **str = '\0';
      (*str)++;
      break;
    }

  if (*ret_str == '\0')
    USER_ERROR1("incorrect input file format%s", "");

  return ret_str;
}

void **ReadTPT(char *train_filename, char *prune_filename, char *test_filename,
               uchar **train_members_ptr, uchar **prune_members_ptr, uchar **test_members_ptr,
               int *num_train_ptr, int *num_prune_ptr, int *num_test_ptr,
               int *num_data_ptr, int *num_features_ptr, SSVINFO *ssvinfo){
  SSVINFO ssvinfo_train, ssvinfo_prune, ssvinfo_test, ssvinfo_temp;
  void **data_train, **data_prune, **data_test, **data_temp, **data_result;
  uchar *train_members, *prune_members, *test_members;

  int num_train, num_prune, num_test, num_temp, num_data;
  int num_features;

  /* Read in the data files */
  data_train = ReadSSVFile(train_filename, &num_train, &num_features, &ssvinfo_train);
  data_prune = ReadSSVFile(prune_filename, &num_prune, &num_features, &ssvinfo_prune);
  data_test = ReadSSVFile(test_filename, &num_test, &num_features, &ssvinfo_test);

  /* Merge them */
  data_temp = MergeSSVFile(num_train, num_prune, &ssvinfo_train, &ssvinfo_prune, 
                           data_train, data_prune, &ssvinfo_temp, num_features);
  num_temp = num_train + num_prune;
  data_result = MergeSSVFile(num_temp, num_test, &ssvinfo_temp, &ssvinfo_test,
                             data_temp, data_test, ssvinfo, num_features);
  num_data = num_temp + num_test;
  *num_data_ptr = num_data;

  /* Assign members properly */
  train_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(train_members,num_data);
  SET_BITARRAY_RANGE(train_members,0,num_train-1);
  *train_members_ptr = train_members;

  prune_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(prune_members,num_data);
  SET_BITARRAY_RANGE(prune_members,num_train,num_train+num_prune-1);
  *prune_members_ptr = prune_members;

  test_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(test_members,num_data);
  SET_BITARRAY_RANGE(test_members,num_temp,num_temp+num_test-1);
  *test_members_ptr = test_members;

  *num_train_ptr = num_train;
  *num_prune_ptr = num_prune;
  *num_test_ptr = num_test;
  *num_features_ptr = num_features;
  return data_result;
}
void **ReadTwo(char *train_filename, char *prune_filename,
               uchar **train_members_ptr, uchar **prune_members_ptr, 
               int *num_train_ptr, int *num_prune_ptr,
               int *num_data_ptr, int *num_features_ptr, SSVINFO *ssvinfo){
  SSVINFO ssvinfo_train, ssvinfo_prune;
  void **data_train, **data_prune, **data_result;
  uchar *train_members, *prune_members;

  int num_train, num_prune, num_data;
  int num_features;


  /* Read in the data files */
  data_train = ReadSSVFile(train_filename, &num_train, &num_features, &ssvinfo_train);
  data_prune = ReadSSVFile(prune_filename, &num_prune, &num_features, &ssvinfo_prune);

  /* Merge them */
  data_result = MergeSSVFile(num_train, num_prune, &ssvinfo_train, &ssvinfo_prune, 
                             data_train, data_prune, ssvinfo, num_features);
  num_data = num_train + num_prune;
  *num_data_ptr = num_data;

  /* Assign members properly */
  train_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(train_members,num_data);
  SET_BITARRAY_RANGE(train_members,0,num_train-1);
  *train_members_ptr = train_members;

  prune_members = CREATE_BITARRAY(num_data);
  ZERO_BITARRAY(prune_members,num_data);
  SET_BITARRAY_RANGE(prune_members,num_train,num_train+num_prune-1);
  *prune_members_ptr = prune_members;

  *num_train_ptr = num_train;
  *num_prune_ptr = num_prune;
  *num_features_ptr = num_features;
  return data_result;
}



/* Destructively merges ssvinfo_A and ssvinfo_B into ssvinfo_result */
void **MergeSSVFile(int num_data_A, int num_data_B, 
                    SSVINFO *ssvinfo_A, SSVINFO *ssvinfo_B,
                    void **data_A, void **data_B,
                    SSVINFO *ssvinfo_result,
                    int num_features)
{
  int num_data_alloc = num_data_A + num_data_B;
  int feature, j, valA, valB;
  int num_discrete_vals_A, num_discrete_vals_B;
  int max_discrete_vals, total_discrete_vals;
  int *discrete_mapping;
  void **data;

  ssvinfo_result->types = ssvinfo_A->types;
  free(ssvinfo_B->types);
  ssvinfo_result->feat_names = ssvinfo_A->feat_names;
  ssvinfo_result->discrete_vals = (char ***) getmem(num_features * sizeof(char **));
  ssvinfo_result->num_discrete_vals = (int *) getmem(num_features * sizeof(int));
  data = (void **)getmem(num_features * sizeof(void *));

  for (feature = 0; feature < num_features; feature++) {
    free(ssvinfo_B->feat_names[feature]);
    switch (ssvinfo_result->types[feature]) {
    case 'b':  /* Binary, use packed bits. */
      data[feature] = CREATE_BITARRAY(num_data_alloc);
      COPY_BITARRAY(data[feature],data_A[feature],num_data_A);
      COPY_BITARRAY_RANGE(data[feature],num_data_A,
                          data_B[feature],0,num_data_B);
      break;
    case 'd':  /* Discrete, use integers. */
      num_discrete_vals_A = ssvinfo_A->num_discrete_vals[feature];
      num_discrete_vals_B = ssvinfo_B->num_discrete_vals[feature];
      max_discrete_vals = num_discrete_vals_A
	                + num_discrete_vals_B;
      total_discrete_vals = num_discrete_vals_A;
      ssvinfo_result->discrete_vals[feature] = (char **) getmem(max_discrete_vals * sizeof(char *));
      for(valA=0;valA<num_discrete_vals_A;valA++)
        ssvinfo_result->discrete_vals[feature][valA] = ssvinfo_A->discrete_vals[feature][valA];
	/* memcpy(ssvinfo_result->discrete_vals[feature],
             ssvinfo_A->discrete_vals[feature], 
             num_discrete_vals_A * sizeof(char *)); */
      /* ssvinfo_result->discrete_vals[feature] = realloc(ssvinfo_A->discrete_vals[feature],
                                                       max_discrete_vals * sizeof(char *)); */

      discrete_mapping = (int *) getmem(num_discrete_vals_B * sizeof(int));
      for(valB=0;valB<num_discrete_vals_B;valB++){
        discrete_mapping[valB]=-1;
	for(valA=0;valA<num_discrete_vals_A;valA++)
	  if (!strcmp(ssvinfo_A->discrete_vals[feature][valA],
                      ssvinfo_B->discrete_vals[feature][valB]))
            discrete_mapping[valB]=valA;
        if (discrete_mapping[valB]==-1){
          ssvinfo_result->discrete_vals[feature][total_discrete_vals] 
            = ssvinfo_B->discrete_vals[feature][valB];
          discrete_mapping[valB]=total_discrete_vals;
          total_discrete_vals++;
        } else {
          free(ssvinfo_B->discrete_vals[feature][valB]);
	}
      }
      free(ssvinfo_A->discrete_vals[feature]);
      free(ssvinfo_B->discrete_vals[feature]);
      ssvinfo_result->num_discrete_vals[feature] = total_discrete_vals;
      data[feature] = (int *) getmem(num_data_alloc * sizeof(int));
      memcpy(data[feature],data_A[feature],num_data_A * sizeof(int));
      for(j=0;j<num_data_B;j++)
        write_attrib_i(data,num_data_A+j,feature,
                          discrete_mapping[read_attrib_i(data_B,j,feature)]);
      free(discrete_mapping);
      break;
    case 'c':  /* Continuous, use doubles. */
      data[feature] = (double *) getmem(num_data_alloc * sizeof(double));
      memcpy(data[feature],data_A[feature],num_data_A * sizeof(double));
      memcpy((double *)(data[feature])+num_data_A,
             data_B[feature],num_data_B * sizeof(double));
      break;
    }
    free(data_A[feature]);
    free(data_B[feature]);
    /* WHY CAN'T WE FREE THIS WITHOUT IT CRASHING??? */
    /* free(ssvinfo_B->feat_names[feature]); */
  }
  free(ssvinfo_B->feat_names);
  free(ssvinfo_A->discrete_vals);
  free(ssvinfo_B->discrete_vals);
  free(ssvinfo_A->num_discrete_vals);
  free(ssvinfo_B->num_discrete_vals);
  free(data_A);
  free(data_B);
  return data;
} 

void **ReadSSVFile(char *filename, int *num_data_ptr,
		   int *num_features_ptr, SSVINFO *ssvinfo)
{ 
  int example, feature;
  unsigned char value_b;
  void **data;
  int num_data, num_data_alloc, num_features;
/*   static ENTRY hentry = { NULL, NULL }; */
  ENTRY *hentry;
  ENTRY *result;
  int val, attr_name_len;
  FILE *fptr;
  char *temp_str = getmem(TEMP_STR_SIZE);
  char *data_str, *word_str;

#define feat_names (ssvinfo->feat_names)
#define types (ssvinfo->types)

  if ((fptr = fopen(filename, "r")) == NULL)
    SYS_ERROR1("fopen(\"%s\", \"r\")", filename);

  /* get number of features and data */
  data_str = fgets_clean(temp_str, fptr);
  num_features = atoi(next_word(&data_str));
  num_data     = atoi(next_word(&data_str));
  if (num_data == 0) 
    num_data_alloc = DEFAULT_DATA_SIZE;
  else
    num_data_alloc = num_data;


  /* Skip over names of features, after duplicating them. */
  feat_names = (char **) getmem(num_features * sizeof(char *));
  data_str = fgets_clean(temp_str, fptr);
  for (feature = 0; feature < num_features; feature++) {
    int feat_name_len;
    word_str = next_word(&data_str);
    feat_name_len = strlen (word_str);
    feat_names[feature] = (char *) getmem((feat_name_len + 1) * sizeof(char));
    strcpy(feat_names[feature], word_str);
  }
  /* Skip over types string. */
  data_str = fgets_clean(temp_str, fptr);
  types = (char *) getmem((strlen(data_str)+1) * sizeof(char));
  strcpy(types, data_str);

  /* Record all data in an array of pointers to arrays of the data
     elements.  Each array may be of different type (that's why we have an
     array of (void *)) as per the types string. */
  data = (void **) getmem(num_features * sizeof(void *));
  ssvinfo->num_discrete_vals = (int *) getmem(num_features * sizeof(int));
  bzero(ssvinfo->num_discrete_vals, num_features * sizeof(int));
  ssvinfo->discrete_vals = (char ***) getmem(num_features * sizeof(char **));
  bzero(ssvinfo->discrete_vals, num_features * sizeof(char **));
  (void) hcreate(num_data_alloc * num_features);
  for (feature = 0; feature < num_features; feature++) {
    switch (types[feature]) {
    case 'b':  /* Binary, use packed bits. */
      data[feature] = CREATE_BITARRAY(num_data_alloc);
      break;
    case 'd':  /* Discrete, use integers. */
      data[feature] = (int *) getmem(num_data_alloc * sizeof(int));
      break;
    case 'c':  /* Continuous, use doubles. */
      data[feature] = (double *) getmem(num_data_alloc * sizeof(double));
      break;
    }
  }

  /* Now read the data into the arrays. */
  for (example = 0; example < num_data_alloc; example++) {
    if (num_data == 0) {
      if ((data_str = fgets_clean_eof(temp_str, fptr)) == NULL)
	break;
    }
    else {
      data_str = fgets_clean(temp_str, fptr);
    }
    for (feature = 0; feature < num_features; feature++) {
      word_str = next_word(&data_str);
      switch(types[feature]) {
      case 'b':
	value_b = *word_str - '0';
	if (value_b != 0 && value_b != 1)
	  USER_ERROR3("ReadSSVFile(): example %d, feature %d "
		      "is not binary (value = %d)\n", example, feature,
		      value_b);
	write_attrib_b(data, example, feature, value_b);
	break;
      case 'd':
	attr_name_len = strlen(word_str);
	hentry = (ENTRY *) getmem(sizeof(ENTRY));
	hentry->key = getmem(attr_name_len + strlen(feat_names[feature]) + 1);
	strcpy(hentry->key, feat_names[feature]);
	strncpy(hentry->key + strlen(feat_names[feature]),
		word_str, attr_name_len);
	(hentry->key)[strlen(feat_names[feature]) + attr_name_len] = '\0';
	/* Check whether we've seen this name before, and assign it an
           integer. */
	if ((result = (ENTRY *) hsearch(*hentry, FIND)) == NULL) {
	  val = ssvinfo->num_discrete_vals[feature]++;
	  if (ssvinfo->discrete_vals[feature] == NULL)
	    ssvinfo->discrete_vals[feature] = (char **)
	      getmem(sizeof(char *));
	  else
	    ssvinfo->discrete_vals[feature] = (char **)
	      realloc((char *) ssvinfo->discrete_vals[feature],
		      (val+1)*sizeof(char *));
	  if (ssvinfo->discrete_vals[feature] == NULL)
	    SYS_ERROR1("realloc(%d)", (val+1)*sizeof(char *));

          /* Seems to not work right: trying different approach */
	  ssvinfo->discrete_vals[feature][val] =
	    my_strndup(word_str, attr_name_len);
	  /* my_strdup(word_str); Different approach didn't work either */
	  hentry->data = (char *) getmem(sizeof(int));
	  *((int *) hentry->data) = val;
	  if (hsearch(*hentry, ENTER) == NULL)
	    USER_ERROR1("%s", "cannot insert entry in hash table");
	} else {
	  val = *((int *) (result->data));
	}
	write_attrib_i(data, example, feature, val);
	break;
      case 'c':
	write_attrib_c(data, example, feature, atof(word_str));
	break;
      default:
	USER_ERROR1("unknown type '%c' encountered", types[feature]);
      }
    }
  }

  hdestroy();

  if (fgets_clean_eof(temp_str, fptr) != NULL) {
    if (num_data == 0) {
      USER_ERROR1("data set larger than max default size in file \"%s\"", filename);
    }
    else {
      USER_ERROR1("additional data at end of file \"%s\"", filename);
    }
  }

  if (num_data == 0)
    num_data = example;
  *num_data_ptr = num_data;
  *num_features_ptr = num_features;
  fclose (fptr);
  free (temp_str);
  return data;

#undef feat_names
#undef types
}
/**************************************************************************/
