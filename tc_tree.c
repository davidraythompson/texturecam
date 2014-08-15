/*
 * \file tc_tree.c
 * \brief Simple image texture classifiers.
 * \author David Ray Thompson \n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include <string.h>
#include "tc_image.h"
#include "tc_filter.h"
#include "tc_node.h"
#include "tc_tree.h"
#include "tc_io.h"

#ifndef FIIAT_tc_tree_C
#define FIIAT_tc_tree_C

int tc_num_leaves_below(tc_node *node);

/**
 * Given an image stack, row and column values, classify a single pixel.
 * return node_hits (inidces of touched nodes), leaf class probability
 * distribution.
 */
int tc_classify(tc_tree *tree,
                tc_image *image,
                const int r,
                const int c)
{

    return ERR; /* never get here */
}

/** Initialize tree to a single root node. */
int tc_init_tree(tc_tree *tree)
{
    if (tree == NULL)
    {
        return ERR;
    }
    tree->nnodes = 1;
    return tc_init_node(&(tree->nodes[0]));
}



/** Safely read a texture tree from a filestream. */
int tc_read_tree(tc_tree *tree, void *tc_io, int nclasses)
{
    int i, j, my_index, MAP_class, threshold, highind, lowind;
    char * buffer = (char *) malloc(BUF_SIZE * sizeof(char));
    char delim = '\n';
    tc_node *node;

    if (tree == NULL || tc_io == NULL)
    {
        free(buffer);
        return ERR;
    }

    if ( (tc_getline_io(tc_io, buffer, BUF_SIZE, delim) != OK) ||
            (sscanf(buffer,"nnodes %d\n", &(tree->nnodes)) != 1) )
    {
        tc_write_log("tc_tree_read: syntax error in header.\r\n");
        free(buffer);
        return ERR;
    }

    /* read nodes one at a time */
    for (i=0; i<(tree->nnodes); i++)
    {
        delim = ' ';
        node = &(tree->nodes[i]);

        if ((tc_getline_io(tc_io, buffer, BUF_SIZE, delim) != OK) ||
                (sscanf(buffer, "%d ", &(my_index)) != 1) ||
                (i != my_index) ||
                (tc_getline_io(tc_io, buffer, BUF_SIZE, delim) != OK) ||
                (sscanf(buffer, "%d ", &(MAP_class)) != 1) ||
                (tc_getline_io(tc_io, buffer, BUF_SIZE, delim) != OK) ||
                (sscanf(buffer, "%d ", &(threshold)) != 1) ||
                (tc_getline_io(tc_io, buffer, BUF_SIZE, delim) != OK) ||
                (sscanf(buffer, "%d ", &(highind)) != 1) ||
                (tc_getline_io(tc_io, buffer, BUF_SIZE, delim) != OK) ||
                (sscanf(buffer, "%d ", &(lowind)) != 1))
        {
            tc_write_log("read_node: Syntax error in decision_tree for node.\r\n");
            free(buffer);
            return ERR;
        }
        node->threshold = threshold;
        node->MAP_class = MAP_class;

        /* set up node pointers or flag as a leaf */
        if (!highind)
        {
            node->high = NULL;
            node->low = NULL;
        }
        else
        {
            node->high = &(tree->nodes[highind]);
            node->low  = &(tree->nodes[lowind]);
        }

        if (tc_read_filter(&(node->filter), tc_io) != OK)
        {
            snprintf(buffer,MAX_STRING,
                     "read_node: Syntax error reading filter for node %i.\n",i);
            tc_write_log(buffer);
            free(buffer);
            return ERR;
        }
        for (j=0; j<nclasses; j++)
        {
            /* Read nclass. */
            /* Read through to newline on last number */
            if( j == (nclasses-1) )
            {
                delim = '\n';
            }
            if ((tc_getline_io(tc_io, buffer, BUF_SIZE, delim) != OK ) ||
                    (sscanf(buffer," %f", &(node->class_counts[j])) != 1))
            {
                tc_write_log("read_node: Syntax error in class counts array for node.\r\n");
                free(buffer);
                return ERR;
            }
        }
        /* zero the rest of the class instance counts, update probabilities */
        for (j=nclasses; j<MAX_N_CLASSES; j++)
        {
            node->class_counts[j] = 0;
        }
        tc_update_probs(node, nclasses);
    }
    free(buffer);
    return OK;
}


/** Write a tree to a stream */
int tc_write_tree(tc_tree *tree, FILE *file, int nclasses)
{
    int i, j;
    tc_node *node, *root = &(tree->nodes[0]);

    if (tree == NULL || file == NULL)
    {
        return ERR;
    }

    fprintf(file,"nnodes %d\n", tree->nnodes);
    for (i=0; i<tree->nnodes; i++)
    {
        fprintf(file,"%d ", i);
        node = &(tree->nodes[i]);
        fprintf(file,"%i ", (int) node->MAP_class);
        fprintf(file,"%i ", (int) node->threshold);
        if (tc_isleaf(node))
        {
            fprintf(file,"0 ");
            fprintf(file,"0 ");
        }
        else
        {
            /* convert from pointers to numerical indices into array */
            int highind = (((size_t) node->high) - ((size_t) root))
                          / sizeof(tc_node);
            int lowind = (((size_t) node->low) - ((size_t) root))
                         / sizeof(tc_node);
            fprintf(file,"%i ", highind);
            fprintf(file,"%i ", lowind);
        }
        tc_write_filter(&(node->filter), file);
        for (j=0; j<nclasses; j++)
        {
            fprintf(file, " %8f", node->class_counts[j]);
        }
        fprintf(file,"\n");
    }
    return OK;
}

/* Return the number of leaves in the tree */
int tc_num_leaves(tc_tree *tree)
{
    tc_node *root = &(tree->nodes[0]);

    return tc_num_leaves_below(root);
}

/* Return the number of leaves below this node */
int tc_num_leaves_below(tc_node *node)
{
    if (tc_isleaf(node))
        return 1;
    else
        return (tc_num_leaves_below(node->low) +
                tc_num_leaves_below(node->high));
}

/**
 * Find the leaf node to which this pixel is assigned.
 */
tc_node* tc_find_leaf(tc_tree *tree,
                      tc_image *image,
                      const int r,
                      const int c)
{
    feature_t result;

    if (tree == NULL || image == NULL)
    {
        fprintf(stderr,"tc_tree: NULL parameter");
        return ERR;
    }

    /* start at the root node of this tree */
    tc_node *node, *root = &(tree->nodes[0]);

    node = root;
    while (1)
    {
        /* check for termination */
        if (tc_isleaf(node))
            return node;

        /* it's a decision branch. */
        if (tc_filter_pixel(&(node->filter), image, r, c, &result) == ERR)
            return (tc_node*)NULL;
        else if (result > node->threshold)
            node = node->high;
        else
            node = node->low;
    }
}




#endif

