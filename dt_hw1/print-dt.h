/**************************************************************************
 *
 * print-dt.h
 *
 * Header file to print-dt.c
 *
 * (C) 1999 Dan Foygel (dfoygel@cs.cmu.edu)
 * Carnegie Mellon University
 * 
 * Based heavily on code written by Dimitris Margaritis (dmarg@cs.cmu.edu)
 *
 **************************************************************************/

#ifndef PRINT_DT_H
#define PRINT_DT_H 1

/* Function prototypes. */
void PrintSection(char *section); 
void PrintStats(DTNODE *tree, void **data, int num_data, uchar *train_members, 
		       int num_train, uchar *test_members, int num_test, SSVINFO *ssvinfo);
void PrintDecisionTreeStructure(DTNODE *root, SSVINFO *ssvinfo);
void PrintAllPaths(DTNODE *root, char *filename, SSVINFO *ssvinfo);

#endif // PRINT_DT_H
/**************************************************************************/
