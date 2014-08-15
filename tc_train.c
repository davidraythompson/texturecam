/*
 * \file tclearn.c
 * \brief Make a decision tree.
 * \author David Ray Thompson \n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 *
 * Implements tree-learning algorithms for texture classification.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "tc_image.h"
#include "tc_colormap.h"
#include "tc_filter.h"
#include "tc_node.h"
#include "tc_tree.h"
#include "tc_forest.h"
#include "tc_train.h"

#ifndef TC_TRAIN_C
#define TC_TRAIN_C

#ifndef SMALL
#define SMALL (1e-10)
#endif

char* TC_FILTERSET_NAMES[] = { "points",
                               "ratios",
                               "rectangles"
                             };

void usage()
{
    fprintf(stderr, "\n");
    fprintf(stderr, "tctrain [OPTIONS] [<input1.pgm> <labels1.pgm>] ...\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Use a mandatory '-o' to specify output, followed \n");
    fprintf(stderr, "by a list of class,label pgm image pairs. A pgm \n");
    fprintf(stderr, "value of zero leaves a class undefined.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -o <forest.rf>     output file (no default)\n");
    fprintf(stderr, "  -w <int>           window size (default: %d)\n",
            TC_TRAIN_WIN_WIDTH);
    fprintf(stderr,
            "  -n <int>           number of training points to randomly sample (default: %d)\n",
            TC_TRAIN_NDATA);
    fprintf(stderr, "  -t <int>           number of trees (default: %d)\n",
            TC_TRAIN_TREES);
    fprintf(stderr,
            "  -b                 use binary classification convention [blue/red]\n");
    fprintf(stderr,
            "  -l <int>           number of expansions per tree (default: %d)\n",
            TC_TRAIN_EXPANSIONS);
    fprintf(stderr,
            "  -f <int>           number of features searched (default: %d)\n",
            TC_TRAIN_FEATURES);
    fprintf(stderr,
            "  -c <int>           number of threads to run (default: %d)\n",
            TC_TRAIN_THREADS);
    fprintf(stderr, "  [--points | --rectangles | --ratios] \n");
    fprintf(stderr,
            "                     set of features to search (default: %s)\n",
            TC_FILTERSET_NAMES[TC_FILTERSET_DEFAULT]);
    fprintf(stderr, "  [--onechannel | --crosschannel]\n");
    fprintf(stderr,
            "                     apply features across channels? (default: %d)\n",
            TC_TRAIN_CROSSCHANNELS);
    fprintf(stderr,
            "  [--fromfile]       The first argument is a file in which the ith line\n");
    fprintf(stderr,
            "                     is a string of the absolute file location of the ith\n");
    fprintf(stderr, "                     training image.\n");
    fprintf(stderr,
            "                     The second argument is a file in which the ith line\n");
    fprintf(stderr,
            "                     is a string of the absolute file location of the ith\n");
    fprintf(stderr, "                     label file\n");
    fprintf(stderr, "  -s <int>           random seed\n");
    fprintf(stderr, "\n");
    return;
}

int main(int argc, char **argv)
{
    tc_write_log("Here we go!\n");
    int arg = 1;
    char *output_filename = NULL;
    tc_dataset *dataset   = NULL;
    tc_forest *forest     = NULL;
    char ** image_filenames = (char **) malloc(sizeof(char*) * 252);
    char ** label_filenames = (char **) malloc(sizeof(char*) * 252);
    tc_colormap *label_colormap = NULL;
    int i,iter = 0;
    int nimages = 0;
    int nlabels = 0;
    long int seed = 0;
    int ndata           = TC_TRAIN_NDATA;
    int filterset       = TC_FILTERSET_DEFAULT;
    int crosschannel    = TC_TRAIN_CROSSCHANNELS;
    int input_method    = USE_IMAGES;
    int sample_method   = TC_RANDOM_SAMPLING;
    int colorchans      = 3; // color labels by default
    int binaryclasses   = 0;
    int winsize         = TC_TRAIN_WIN_WIDTH;
    int ntrees          = TC_TRAIN_TREES;
    int nthreads        = TC_TRAIN_THREADS;
    int nfeatures       = TC_TRAIN_FEATURES;
    int niter           = TC_TRAIN_EXPANSIONS;

    if (argc < 4)
    {
        fprintf(stderr, "Too few arguments.\n");
        usage();
        free(image_filenames);
        free(label_filenames);
        exit(-1);
    }

    fprintf(stdout, "Parsing arguments.\n");
    while (argv[arg][0] == '-')
    {
        if (argv[arg][1] == 'o')
        {
            arg++;
            output_filename = argv[arg];
            fprintf(stdout,"I'll output to %s\n", output_filename);
        }
        else if (argv[arg][1] == 'n')
        {
            arg++;
            ndata = atoi(argv[arg]);
            fprintf(stdout,"%i data points\n", ndata);
        }
        else if (argv[arg][1] == 'l')
        {
            arg++;
            niter = atoi(argv[arg]);
            if (niter < 1)
            {
                fprintf(stderr,"Must have at least one level.\n");
                free(image_filenames);
                free(label_filenames);
                exit(-1);
            }
        }
        else if (argv[arg][1] == 's')
        {
            arg++;
            seed = atoi(argv[arg]);
        }
        else if (argv[arg][1] == 'f')
        {
            arg++;
            nfeatures = atoi(argv[arg]);
            if (nfeatures < 1)
            {
                fprintf(stderr,"Must search at least one feature per expansion.\n");
                free(image_filenames);
                free(label_filenames);
                exit(-1);
            }
        }
        else if (argv[arg][1] == 't')
        {
            arg++;
            ntrees = atoi(argv[arg]);
            if (ntrees < 1)
            {
                fprintf(stderr,"Must grow at least one tree.\n");
                free(image_filenames);
                free(label_filenames);
                exit(-1);
            }
        }
        else if (argv[arg][1] == 'w')
        {
            arg++;
            winsize = atoi(argv[arg]);
            if ((winsize % 2 != 1) || (winsize < 5))
            {
                fprintf(stderr,"window size must be an odd number >= 5\n");
                free(image_filenames);
                free(label_filenames);
                exit(-1);
            }
            fprintf(stdout,"window size %i\n", winsize);
        }
        else if (argv[arg][1] == 'c')
        {
            arg++;
            nthreads = atoi(argv[arg]);
            if (nthreads < 1)
            {
                fprintf(stderr,"Must have at least one thread.\n");
                free(image_filenames);
                free(label_filenames);
                exit(-1);
            }
        }
        else if (argv[arg][1] == 'b')
        {
            fprintf(stdout,"Using binary classification (BG blue, FG red)\n");
            binaryclasses = 1;
        }
        else if (strncmp(argv[arg],"--help",6) == 0)
        {
            usage();
            free(image_filenames);
            free(label_filenames);
            exit(0);
        }
        else if (strncmp(argv[arg],"--rectangles",12) == 0)
        {
            fprintf(stdout,"Using area features (integral image)\n");
            filterset = TC_FILTERSET_RECTANGLES;
        }
        else if (strncmp(argv[arg],"--points",8) == 0)
        {
            fprintf(stdout,"Using point comparison features\n");
            filterset = TC_FILTERSET_POINTS;
        }
        else if (strncmp(argv[arg],"--ratios",8) == 0)
        {
            fprintf(stdout,"Using point ratio features\n");
            filterset = TC_FILTERSET_RATIOS;
        }
        else if (strncmp(argv[arg],"--crosschannel",14) == 0)
        {
            fprintf(stdout,"Using cross-channel features \n");
            crosschannel = 1;
        }
        else if (strncmp(argv[arg],"--onechannel",12) == 0)
        {
            fprintf(stdout,"Using single-channel features \n");
            crosschannel = 0;
        }
        else if (strncmp(argv[arg],"--fromfile",10) == 0)
        {
            input_method = USE_LISTS;
        }
        else if (strncmp(argv[arg],"--balance",9) == 0)
        {
            fprintf(stdout,"Using balanced sampling\n");
            sample_method = TC_BALANCED_SAMPLING;
        }
        else
        {
            fprintf(stderr,"I don't recognize the parameter %s\n",argv[arg]);
            usage();
            free(image_filenames);
            free(label_filenames);
            exit(-1);
        }
        arg++;
    }
    if (output_filename == NULL)
    {
        fprintf(stderr,"Use '-o <output>' to specify an output file\n");
        usage();
        free(image_filenames);
        free(label_filenames);
        exit(-1);
    }
    while (arg<argc)
    {

        /* Load images from folders.
         * It's possible scandir is not supported on every machine!
         * USE_LISTS allocates new memory to image_filenames
         * which must be freedd below.*/
        if(input_method == USE_LISTS)
        {
            fprintf(stdout, "Loading images from file %s\n", argv[arg]);
            tc_read_list_file(image_filenames, &nimages, argv[arg]);
            arg++;

            fprintf(stdout, "Loading labels from file %s\n", argv[arg]);
            tc_read_list_file(label_filenames, &nlabels, argv[arg]);
            arg++;
        }
        else
        {
            fprintf(stdout,"Loading image %s\n", argv[arg]);
            image_filenames[nimages] = argv[arg];
            arg++;

            fprintf(stdout,"\tlabels: %s\n", argv[arg]);
            label_filenames[nimages] = argv[arg];
            arg++;
            nimages++;
        }
    }

    if (colorchans == 3)
    {
        fprintf(stdout,"Saving color labels.\n");
    }

    /* Load all labeled images and map every pixel's color to a label.
     *    The color map will be initialized once the first image is loaded.
     */
    if (binaryclasses)
    {
        tc_binary_colormap(&label_colormap);
        fprintf(stdout,"Using blue = background, red = foreground convention\n");
    }
    else
    {
        for (i = 0; i < nimages; i++)
        {
            if (tc_find_classes(&label_colormap, label_filenames[i], colorchans) == ERR)
            {
                fprintf(stderr,"Error in tc_find_classes\n");
                free(image_filenames);
                free(label_filenames);
                exit(-1);
            }
        }
        fprintf(stdout,"Found %d total classes.\n", label_colormap->nclasses);
    }

    fprintf(stdout,"Initializing random dataset, %d samples.\n", ndata);
    if (tc_random_dataset(&dataset,
                          image_filenames,
                          label_filenames,
                          label_colormap,
                          nimages,
                          ndata,
                          sample_method,
                          seed) == ERR)
    {
        fprintf(stderr,"Error in random_dataset\n");
        free(image_filenames);
        free(label_filenames);
        exit(-1);
    }

    fprintf(stdout,"Initializing random forest, %d trees.\n", ntrees);
    if (tc_init_forest(&forest,
                       ntrees,
                       filterset,
                       dataset->nclasses,
                       winsize) == ERR)
    {
        fprintf(stderr,"Error initializing random forest.\n");
        free(image_filenames);
        free(label_filenames);
        exit(-1);
    }

    fprintf(stdout,"Even assignment.\n");
    tc_assign_evenly(dataset, forest);

    /* klw: This doesn't do anything, since
     * all of the trees are just leaf nodes at this point. */
    /*fprintf(stdout,"Propagation.\n");
      tc_propagate_forest(dataset, forest);*/

    for (iter=0; iter<niter; iter++)
    {
        fprintf(stdout, "Grow forest, iteration %d/%d\n",
                iter+1, niter);
        if (tc_grow(dataset,
                    forest,
                    filterset,
                    winsize,
                    nthreads,
                    nfeatures,
                    crosschannel) == ERR)
        {
            fprintf(stderr, "Error in grow.\n");
        }
    }

    fprintf(stdout,"Tallying probabilities.\n");
    tc_tally_classes(dataset, forest);

    /* write output */
    fprintf(stdout,"Writing output.\n");
    if (tc_save_forest(forest, output_filename, label_colormap) == ERR)
    {
        fprintf(stderr, "Error in write forest\n");
        free(image_filenames);
        free(label_filenames);
        return ERR;
    }

    /* clean up */
    fprintf(stdout,"Clean up.\n");
    tc_free_forest(forest);
    tc_free_dataset(dataset);
    tc_free_colormap(label_colormap);

    if (input_method == USE_LISTS)
    {
        /* With these input methods we must free the image filenames */
        for (i=0; i<nimages; i++)
        {
            free(image_filenames[i]);
        }
    }
    free(image_filenames);
    free(label_filenames);
    return 0;
}



/**
 * For a given forest, recalculate all class probabilities and MAP
 * classes
 * */
int tc_reestimate_probs(tc_dataset *dataset, tc_forest *forest)
{

    int i, t, size;
    if (forest == NULL || dataset == NULL) return ERR;

    for (i=0; i<dataset->ndata; i++)
    {
        tc_datum *datum = &(dataset->data[i]);
        tc_tree *tree = &(forest->trees[(i % forest->ntrees)]);
        tc_node *root = &(tree->nodes[0]);
        datum->next = root->data;
        root->data = datum;
    }

    for (t=0; t<forest->ntrees; t++)
    {
        size=0;
        tc_tree *tree = &(forest->trees[t]);
        tc_node *root = &(tree->nodes[0]);
        tc_datum *datum = root->data;
        while (datum != NULL)
        {
            size++;
            datum = datum->next;
        }
        fprintf(stdout,"tree %i size %i\n",t,size);
    }

    return OK;
}



/**
 * Even subset assignment to root nodes of the forest
 * */
int tc_assign_evenly(tc_dataset *dataset, tc_forest *forest)
{

    int i, t, size;
    if (forest == NULL || dataset == NULL) return ERR;

    for (i=0; i<dataset->ndata; i++)
    {
        t = (int)((float)(i*forest->ntrees)/(float)(dataset->ndata));
        tc_datum *datum = &(dataset->data[i]);
        tc_tree *tree = &(forest->trees[t]);
        tc_node *root = &(tree->nodes[0]);
        datum->next = root->data;
        root->data = datum;
    }

    for (t=0; t<forest->ntrees; t++)
    {
        size=0;
        tc_tree *tree = &(forest->trees[t]);
        tc_node *root = &(tree->nodes[0]);
        tc_datum *datum = root->data;
        while (datum != NULL)
        {
            size++;
            datum = datum->next;
        }
        fprintf(stdout,"tree %i assigned %i items.\n",t,size);
    }

    return OK;
}

/**
 * Propagate all training data in a forest to the leaves
 */
int tc_propagate_forest(tc_dataset *dataset, tc_forest *forest)
{
    int t, n;
    if (forest == NULL || dataset == NULL) return ERR;

    for (t=0; t<forest->ntrees; t++)
    {
        tc_tree *tree = &(forest->trees[t]);
        for (n=0; n<tree->nnodes; n++)
        {
            tc_node *node = &(tree->nodes[n]);
            tc_propagate_node(dataset, tree, node);
        }
    }
    return OK;
}


/**
 * Propagate training data from a single node to its children
 */
int tc_propagate_node(tc_dataset *dataset, tc_tree *tree, tc_node *node)
{
    if (dataset == NULL || tree == NULL || node == NULL) return ERR;

    tc_node *d = NULL;
    tc_image *image;
    tc_datum *cur, *next;
    feature_t result;
    int r, c;

    if (!tc_isleaf(node) && node->data != NULL)
    {
        cur = node->data;
        while (cur != NULL)
        {
            next = cur->next;
            image = dataset->images[cur->image];
            r = cur->r;
            c = cur->c;

            /* we throw out pixels on the border
             * during propagation! */
            if (tc_filter_pixel(&(node->filter), image,
                                r, c, &result) != ERR)
            {
                if (result > node->threshold)
                {
                    d = node->high;
                }
                else
                {
                    d = node->low;
                }
                (*cur).next = d->data;
                d->data = cur;
            }
            cur = next;
        }
        node->data = NULL;
    }
    return OK;
}


/**
 * For a given tree, find the node to expand next
 */
tc_node* tc_next_expansion(tc_tree *tree)
{
    if (tree == NULL)
    {
        fprintf(stderr,"tc_next_expansion: NULL tree?\n");
        return ERR;
    }

    tc_node *best_node = NULL;
    int n, size, best_size =
        TC_TRAIN_MIN_SAMPLES; /* we require at least this many */

    for (n=0; n<tree->nnodes; n++)
    {
        tc_node *node = &(tree->nodes[n]);

        /* check for leaf and unexpandable status */
        tc_datum *i = node->data;
        size = 0;
        while (i != NULL)
        {
            i = i->next;
            size++;
        }

        if (tc_isexpandable(node))
        {
            if (size > best_size)
            {
                best_size = size;
                best_node = node;
            }
        }
    }
    return best_node;
}



/**
 * Last step of training.
 * Propagate ALL data through each tree, computing class
 * probability distributions and MAP estimates, and renormalizing.
 * This could be replaced by a calls to propagate_ functions
 */
int tc_tally_classes(tc_dataset *dataset, tc_forest *forest)
{
    tc_image *image;
    tc_node *node;
    tc_tree *tree;
    tc_datum *datum;
    int label, r, c, t, n, i;
    feature_t result;

    if (dataset == NULL || forest == NULL)
    {
        return ERR;
    }

    /* zero all class counts */
    for (t=0; t<forest->ntrees; t++)
    {
        tree = &(forest->trees[t]);
        for (n=0; n<tree->nnodes; n++)
        {
            node = &(tree->nodes[n]);
            for (c=0; c<forest->nclasses; c++)
            {
                node->class_counts[c] = 0;
                node->class_probs[c] = 0.0;
            }
        }
    }

    /* propagate all datapoints */
    for (i=0; i<dataset->ndata; i++)
    {
        datum = &(dataset->data[i]);
        image = dataset->images[datum->image];
        label = datum->label;
        r = datum->r;
        c = datum->c;

        /* propagate each datapoint through each tree */
        for (t=0; t<forest->ntrees; t++)
        {
            /* start at the root node */
            tree = &(forest->trees[t]);
            node = &(tree->nodes[0]);
            node->class_counts[label] +=
                (1.0f/((float)dataset->represented[label]));

            while (!tc_isleaf(node))
            {
                if (tc_filter_pixel(&(node->filter), image,
                                    r, c, &result) == ERR)
                {
                    /* current strategy throws away edge data points */
                    break;
                }

                if (result > node->threshold)
                {
                    node = node->high;
                }
                else
                {
                    node = node->low;
                }
                node->class_counts[label] +=
                    (1.0f/((float)dataset->represented[label]));
            }
        }
    }

    /* normalize all probabilities, update MAP estimates */
    for (t=0; t<forest->ntrees; t++)
    {
        tree = &(forest->trees[t]);
        for (n=0; n<tree->nnodes; n++)
        {
            node = &(tree->nodes[n]);
            tc_update_probs(node, forest->nclasses);
        }
    }
    return OK;
}


/**
 * Grow each tree in the forest forest by one iteration, with a parallel
 * search over splitting features.
 */
int tc_grow(tc_dataset *dataset,
            tc_forest *forest,
            int filterset,
            int winsize,
            int nthreads,
            int nfeatures,
            int crosschannel)
{

    if (dataset == NULL || forest == NULL)
    {
        return ERR;
    }
    int t=0, i=0;

    for (t=0; t<forest->ntrees; t++)
    {
        tc_tree *tree = &(forest->trees[t]);
        /*fprintf(stdout,"Expanding tree number: %i\n", t); */

        if (tree->nnodes >= (MAX_TREE_NODES-2))
        {
            fprintf(stderr,"grow: Maximum nodes exceeded in tree.\n");
            continue;
        }

        /* Get the dataset to expand next */
        tc_node *node = tc_next_expansion(tree);
        if (!node)
        {
            fprintf(stderr,"grow: Can't expand tree.\n");
            return ERR;
        }
        tc_node *root = &(tree->nodes[0]);

        size_t n = (((size_t)node) - ((size_t)root)) / sizeof(tc_node);
        /*fprintf(stdout,"Expanding node number: %li\n", (long int) n); */
        tc_datum *subset = node->data;

        /* Look for the best split */
        tc_trainer *trainers =
            (tc_trainer *) malloc(sizeof(tc_trainer) * nthreads);
        if (trainers == NULL)
        {
            return ERR;
        }

        /* Parallel random search to find the best split */
        for (i=0; i<nthreads; i++)
        {
            tc_init_trainer(&(trainers[i]), dataset, subset,
                            filterset, winsize, nfeatures, crosschannel);
            pthread_create(&(trainers[i].pthread), NULL,
                           (void *)(void *) tc_split_search,
                           (void *) &(trainers[i]));
        }
        /*fprintf(stdout,"Threads are done\n");*/

        /* Reduce step */
        int winner = 0;
        for (i=0; i<nthreads; i++)
        {
            pthread_join(trainers[i].pthread, NULL);
            if (((!trainers[winner].valid) &&
                    trainers[i].valid) ||
                    (trainers[winner].valid &&
                     trainers[i].valid &&
                     trainers[i].best_score > trainers[winner].best_score))
            {
                winner = i;
            }
        }

        /* modify the tree */
        if (!trainers[winner].valid)
        {
            /*fprintf(stdout,"Can't expand this node further. \n"); */
            node->expandable = 0;
        }
        else
        {
            /* We can split the leaf.  Add two nodes to the tree. */
            node->threshold = trainers[winner].best_threshold;
            tc_copy_filter(&(node->filter), &(trainers[winner].best_filter));
            char *best_filter_str = malloc(MAX_STRING * sizeof(char));

            tc_filter_tostring(&(trainers[winner].best_filter),
                               best_filter_str);
            fprintf(stdout,"Tree %d, node %i: splitting %s at %i, score %2.2f\n",
                    t, (int) n, best_filter_str,
                    (int) trainers[winner].best_threshold,
                    trainers[winner].best_score);

            /* add two new leaf nodes to the tree */
            node->low = &(tree->nodes[tree->nnodes]);
            tc_init_node(node->low);
            node->high = &(tree->nodes[tree->nnodes+1]);
            tc_init_node(node->high);
            tree->nnodes += 2;

            /* propagate training data down to the new level */
            tc_propagate_node(dataset, tree, node);
            free(best_filter_str);
        }
        free(trainers);
    }
    return OK;
}



/** Transform a threshold value to a numerical index, 0-n */
feature_t get_threshold_index(feature_t threshold, int min, int n)
{
    feature_t ind = (threshold - min);
    ind = (ind < 0)? 0 : ind;
    ind = (ind >= n)? (n-1) : ind;
    return ind;
}



/** Transform a numerical index 0-n into a threshold */
feature_t index_to_threshold(feature_t thresh_ind, int min)
{
    return (thresh_ind + min);
}



/** Initialize trainer object */
void tc_init_trainer(tc_trainer *trainer,
                     tc_dataset *dataset,
                     tc_datum *subset,
                     int filterset,
                     int winsize,
                     int nfeatures,
                     int crosschannel)
{
    if ((trainer == NULL) ||
            (dataset == NULL) ||
            (subset == NULL))
    {
        fprintf(stderr,"NULL parameter in tc_init_trainer\n");
        return;
    }
    trainer->best_score     = -9e99;
    trainer->best_threshold = -1;
    trainer->dataset        = dataset;
    trainer->subset         = subset;
    trainer->filterset      = filterset;
    trainer->winsize        = winsize;
    trainer->nfeatures      = nfeatures;
    trainer->valid          = 0;
    trainer->crosschannel   = crosschannel;
    tc_init_filter(&(trainer->best_filter));
}



/**
 * Find the best split for a given subset of pixels from
 * a set of image stacks and a bank of filters.
 */
void * tc_split_search(tc_trainer * trainer)
{
    /*fprintf(stdout,"Thread starting\n");*/
    if (trainer == NULL)
    {
        fprintf(stderr,"best_split: bad params\n");
        return NULL;
    }

    /* local arrays and variables used in search */
    int i, t, iter;
    feature_t thresh_ind, result;
    int min_split_size = TC_TRAIN_MIN_SPLIT;
    int total_low, total_high;
    int nclasses = trainer->dataset->nclasses;
    int ncounts = N_THRESH * nclasses;
    float counts[ncounts];
    float accum_counts[ncounts];
    float log_counts[ncounts];
    float accum_log_counts[ncounts];
    float split_score;
    float entropy_high, entropy_low;
    float expected_posterior_entropy;
    float lowdist_prob_class;
    float mass_scale[nclasses];
    float max_represented = 0;
    float highdist_prob_class;
    int lowdist_class_cnts[nclasses];
    int highdist_class_cnts[nclasses];


    /* get mass scaling for each class */
    for (i = 0; i < nclasses; i++)
    {
        if (trainer->dataset->represented[i] > max_represented)
        {
            max_represented = trainer->dataset->represented[i];
        }
    }
    for (i = 0; i < nclasses; i++)
    {
        mass_scale[i] = max_represented /
                        ((float)(trainer->dataset->represented[i]));
    }

    /* get the minimum number of channels across all images */
    int min_chans = trainer->dataset->images[0]->chans;
    for (i=0; i<trainer->dataset->nimages; i++)
    {
        int chans = trainer->dataset->images[i]->chans;
        if (chans < min_chans) min_chans = chans;
    }

    /* random search for best filter */
    for (iter=0; iter<trainer->nfeatures; iter++)
    {
        tc_filter candidate;
        tc_randomize_filter(&candidate,
                            min_chans,
                            trainer->filterset,
                            trainer->winsize,
                            trainer->crosschannel);

        if ((iter%100 == 0) && (trainer->best_score > -9e98))
        {
            fprintf(stdout,"  %i filters tried - best score %f\n",
                    iter, trainer->best_score);
        }

        /* find the split that results in the best possible reduction in
         * expected entropy.  */
        for (i = 0; i < ncounts; i++)
        {
            counts[i] = 0;
            log_counts[i]=0;
            accum_counts[i]=0;
            accum_log_counts[i]=0;
        }

        /* filter input images, find counts */
        tc_datum *d = trainer->subset;
        while (d != NULL)
        {
            tc_image *image = trainer->dataset->images[d->image];
            if (tc_filter_pixel(&candidate, image,
                                d->r, d->c, &result) != ERR)
            {
                /*
                 * For this test, we accumulate counts of all classes
                 * at each threshold.
                 * We scale result to interval of thresholds tested.
                 */
                feature_t result_ind = get_threshold_index(result,
                                       MIN_THRESH, N_THRESH);

                /* increment the class / counts array */
                int label = d->label;
                counts[(label*N_THRESH)+((int) (result_ind))] +=
                    mass_scale[label];
            }
            d = d->next;
        }

        /* get cumulative counts for each class / value combination */
        for (i = 0; i < nclasses; i++)
        {
            /* first for threshold level zero... */
            accum_counts[i*N_THRESH+0] = counts[i*N_THRESH+0];
            log_counts[i*N_THRESH+0] = log(counts[i*N_THRESH+0]);
            accum_log_counts[i*N_THRESH+0] = log_counts[i*N_THRESH+0];

            /* accumulate values for later thresholds */
            for (t = i * N_THRESH+1;  t < (i+1) * N_THRESH; t++)
            {
                log_counts[t] = log(counts[t]);
                accum_counts[t] = counts[t] + accum_counts[t-1];
                accum_log_counts[t] = log_counts[t] + accum_log_counts[t-1];
            }
        }

        /*
         * Now we test all thresholds on this candidate feature,
         * using only the best filter. This uses a linear search,
         * but we could do something smarter.  Also, we could pull
         * much of this computation outside the loop
         */
        for (thresh_ind = 1; thresh_ind < N_THRESH-1; thresh_ind++)
        {
            /*
             * Find the total number of data points falling above and below
             * this threshold
             */
            total_low = 0;
            total_high = 0;
            for (i = 0; i < nclasses; i++)
            {
                lowdist_class_cnts[i] =
                    accum_counts[i*N_THRESH+thresh_ind];
                highdist_class_cnts[i] =
                    (accum_counts[i*N_THRESH+(N_THRESH-1)]
                     - lowdist_class_cnts[i]);
                total_low += lowdist_class_cnts[i];
                total_high += highdist_class_cnts[i];
            }

            /*
             * Should we bother splitting?
             */
            if ((total_low < min_split_size) ||
                    (total_high < min_split_size))
            {
                continue;
            }

            /* Get entropy of class distribution for pixels below
             * and above filter threshold. */
            entropy_low = 0;
            entropy_high = 0;

            for (i = 0; i < nclasses; i++)
            {
                if (lowdist_class_cnts[i] >= SMALL &&
                        highdist_class_cnts[i] >= SMALL)
                {

                    lowdist_prob_class =
                        ((float)lowdist_class_cnts[i]) /
                        ((float)total_low);
                    highdist_prob_class =
                        ((float)highdist_class_cnts[i]) /
                        ((float)total_high);

                    entropy_low += lowdist_prob_class *
                                   log(lowdist_prob_class);
                    entropy_high += highdist_prob_class *
                                    log(highdist_prob_class);
                }
            }

            /* compute expected posterior entropy after splitting at the
             * filter threshold */
            expected_posterior_entropy =
                -(total_high * entropy_high +
                  total_low * entropy_low) /
                (total_low + total_high);

            split_score = -expected_posterior_entropy;

            if (split_score > trainer->best_score)
            {
                trainer->best_score = split_score;
                trainer->best_threshold =
                    index_to_threshold(thresh_ind, MIN_THRESH);
                tc_copy_filter(&(trainer->best_filter), &candidate);
                trainer->valid = 1;
            }
        }
    }
    /*fprintf(stdout,"Thread finished\n");*/
    return NULL;
}



/*
 * Read all images listed in the given file
 * Allocates new memory to image_filenames that must be freed elsewhere
 */
int tc_read_list_file(char **filenames, int *nfiles,
                      const char *list_filename)
{
    char *msg = (char*) malloc(sizeof(char) * MAX_STRING);
    char *linetmp = (char *) malloc(MAX_STRING*sizeof(char));
    FILE *file;
    char *line = NULL;
    int i=0;

    file = fopen(list_filename, "r");
    if (!file)
    {
        snprintf(msg,MAX_STRING,"couldn't open %s\n", list_filename);
        tc_write_log(msg);
        free(msg);
        free(linetmp);
        return ERR;
    }

    while(fgets(linetmp, MAX_STRING, file) != NULL)
    {
        for (i=0; i<strlen(linetmp); i++)
        {
            if(linetmp[i] == '\n' || linetmp[i] == '\r')
            {
                linetmp[i] = '\0';
            }
        }
        line = malloc(MAX_STRING*sizeof(char));
        strncpy(line, linetmp, MAX_STRING);
        filenames[*nfiles] = line;

        snprintf(msg, MAX_STRING, "\timage: '%s'\n", filenames[*nfiles]);
        tc_write_log(msg);
        *nfiles = *nfiles+1;
    }

    free(linetmp);
    fclose(file);
    free(msg);
    return OK;
}


#endif
