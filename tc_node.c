/*
 * \file tc_node.c
 * \brief Simple image pixel classifier tree nodes.
 * \author David Ray Thompson \n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tc_image.h"
#include "tc_filter.h"
#include "tc_node.h"

#ifndef tc_node_C
#define tc_node_C


/** Is the node a leaf? */
int tc_isleaf(tc_node *node)
{
    if (node == NULL)
    {
        return ERR;
    }
    return (node->high == NULL);
}


/** Can we expand this node further? */
int tc_isexpandable(tc_node *node)
{
    if (node == NULL)
    {
        return 0;
    }
    return (node->expandable);
}


/** Default settings for a tree node. */
int tc_init_node(tc_node *node)
{
    int i;
    if (node == NULL)
    {
        return ERR;
    }
    node->MAP_class  = ERROR_CLASS;
    node->threshold  = 0;   /* this test will never happen (nowhere to go) */
    node->high       = NULL;
    node->low        = NULL;
    node->data       = NULL;
    node->expandable = 1;
    for (i=0; i < MAX_N_CLASSES; i++)
    {
        node->class_counts[i] = 0;
        node->class_probs[i] = 0;
    }
    return tc_init_filter(&(node->filter));
}



/**
 * Updates the cached MAP estimate and class probabilities
 * of a node, based on class counts
 */
int tc_update_probs(tc_node *node, int nclasses)
{
    int i, max_ind;
    float total_counts, max_counts;
    if (node == NULL)
    {
        return ERR;
    }

    /* handle unclassified category separately */
    node->class_counts[0] = 0;
    node->class_probs[0] = 0;

    /* Tally counts */
    max_counts = -1;
    total_counts = 0;
    max_ind = 0;

    for (i=1; i < nclasses; i++)
    {
        total_counts += node->class_counts[i];
        if (node->class_counts[i] > max_counts)
        {
            max_counts = node->class_counts[i];
            max_ind = i;
        }
    }

    /* MAP class*/
    node->MAP_class = max_ind;

    /* Normalized probabilities */
    for (i=1; i < nclasses; i++)
    {
        node->class_probs[i] =
            (total_counts == 0)? 0:
            (node->class_counts[i] / ((float) total_counts));
    }
    return OK;
}

#endif

