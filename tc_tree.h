/**
 * \file tc_tree.h
 * \brief Simple tc_pixel classification trees.
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdlib.h>
#include <stdio.h>
#include "tc_image.h"
#include "tc_node.h"
#include "tc_filter.h"

#ifndef TC_tree_H
#define TC_tree_H

#define MAX_TREE_NODES  (512)

/**
 * \brief A complete tc_pixel decision tree classifier.
 *
 * The tree consists of an ordered array of tc_node structs.
 * It has a single root node at index 0 in this array.  The numbers
 * referenced in individual node structs' "low" and "high" values are
 * actually indices into this array.  We rely on the training procedure
 * to prevent cycles!
 */
typedef struct
{
    tc_node nodes[MAX_TREE_NODES];
    int nnodes;

} tc_tree;

int tc_init_tree(tc_tree *tree);
int tc_read_tree(tc_tree *tree, void *tc_io, int nclasses);
int tc_write_tree(tc_tree *tree, FILE *tc_io, int nclasses);
int tc_num_leaves(tc_tree *tree);
tc_node* tc_find_leaf(tc_tree *tree, tc_image *image, const int r,
                      const int c);

#endif
