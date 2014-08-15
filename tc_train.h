/**
 * \file tc_train.h
 * \brief Learn a texture forest based on one or more input images.
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include "tc_image.h"
#include "tc_filter.h"
#include "tc_node.h"
#include "tc_tree.h"
#include "tc_forest.h"
#include "tc_dataset.h"

#ifndef TC_TRAIN_H
#define TC_TRAIN_H

/* Input options */
#define USE_IMAGES             (0)
#define USE_LISTS              (1)

#define TC_TRAIN_WIN_WIDTH     (61)
#define TC_TRAIN_TREES         (64)
#define TC_TRAIN_FEATURES      (64)
#define TC_TRAIN_EXPANSIONS    (64)
#define TC_TRAIN_THREADS       (1)
#define TC_TRAIN_MIN_SPLIT     (32)
#define TC_TRAIN_MIN_SAMPLES   (32)
#define TC_TRAIN_CROSSCHANNELS (0)
#define TC_TRAIN_NDATA         (100000)
#define MIN_THRESH             (-255)
#define N_THRESH               (512)

typedef struct tc_trainer_type
{
    feature_t best_threshold;
    tc_filter best_filter;
    tc_dataset *dataset;
    tc_datum  *subset;
    int winsize;
    int filterset;
    int valid;
    int nfeatures;
    int crosschannel;
    float best_score;
    pthread_t pthread;
} tc_trainer;

/**
 * Even subset assignment to root nodes
 **/
int tc_assign_evenly(tc_dataset *dataset, tc_forest *forest);

/**
 * Propagate training data all to the way to the leaf nodes
 **/
int tc_propagate_forest(tc_dataset *dataset, tc_forest *forest);

/**
 * Propagate any training data stored at a single node to its children
 */
int tc_propagate_node(tc_dataset *dataset, tc_tree *tree, tc_node *node);

/**
 * which is the next node to expand in the tree?
 */
tc_node* tc_next_expansion(tc_tree *tree);

/**
 * last step - repropagate all training data through all
 * forests and update class counts, MAP class estimates, etc.
 */
int tc_tally_classes(tc_dataset *dataset, tc_forest *forest);

/**
 * Grow a forest
 * */
int tc_grow(tc_dataset *dataset,
            tc_forest *forest,
            int filterset,
            int winsize,
            int nthreads,
            int nfeatures,
            int crosschannel);
/**
 * Entropy of a class probability distribution,
 * excluding the "unknown class" label
 */
double tc_class_probs_entropy(double *class_probs);

/**
 * Find the best split of a dataset according to the
 * criterion in training_mode
 */
void * tc_split_search(tc_trainer *trainer);

/**
 * Initialize trainer object
 */
void tc_init_trainer(tc_trainer *trainer,
                     tc_dataset *dataset,
                     tc_datum *subset,
                     int filterset,
                     int winsize,
                     int nfeatures,
                     int crosschannel);

/**
 * Read a directory of images
 */
int tc_read_image_dir(char **image_filenames, int *nimages,
                      const char *image_directory);

/**
 * Read a list of images
 */
int tc_read_list_file(char **filenames, int *nfiles,
                      const char *list_filename);


#endif
