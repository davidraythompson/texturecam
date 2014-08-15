/**
 * \file tc_node.h
 * \brief Node in the texture decision tree.
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
#include "tc_dataset.h"
#include "tc_filter.h"

#ifndef tc_node_H
#define tc_node_H

/**
 * \brief A single node of the texture classification tree.
 *
 * It describes a single classification decision based on a particular
 * TC wavelet-like filter applied to an image pixel.  It maintains
 * information about the posterior class probababilities for any new
 * pixels reaching this node, the classification thresholding decision
 * itself, and also node indexes for child nodes in the decison tree.
 *
 * The key members are:
 * - class_counts : An array of counts of size MAX_CLASSES describing
 *     how many training points of each class reach this node.
 * - class_probs : A normalized version of the above, interpretable as
 *     a posterior probability distribution over texture classes.
 * - high / low : Indices into the classification tree for
 *     the children of this node.
 * - filter : The struct describing this node's filter
 * - threshold : Our decision boundary - the value of this node's
 *     filter above which we visit the "high" child instead of "low".
 *
 */
typedef struct tc_node_type
{

    /* classification parameters */
    float class_counts[MAX_N_CLASSES];   /* dirichlet parameters */
    float class_probs[MAX_N_CLASSES];    /* cached probabilities */
    class_t MAP_class;                   /* cached Maximum A Posteriori class */

    /* tree structure - high depth value signifies a leaf.*/
    struct tc_node_type
            *high;           /* pointer into node list for high TC_pixel */
    struct tc_node_type
            *low;            /* pointer into node list for low TC_pixel */
    int expandable;                      /* can we expand the node further? */

    /* filter */
    tc_filter filter;         /* attributes */
    feature_t threshold;      /* what threshold? */

    /* used for training only - subset of data at this node */
    tc_datum *data;

} tc_node;


/* set node to have default values */
int tc_init_node(tc_node *node);

/* ugs_counts, update MAP estimates and class_probs */
int tc_update_probs(tc_node *node, int nclasses);

/* is the node a leaf? */
int tc_isleaf(tc_node *node);

/* can we expand this node further? */
int tc_isexpandable(tc_node *node);

#endif
