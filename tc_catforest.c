/*
 * \file tc_catforest.c
 * \brief Concatenate two decision forests
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "tc_image.h"
#include "tc_filter.h"
#include "tc_node.h"
#include "tc_tree.h"
#include "tc_forest.h"

#ifndef TC_CATFOREST_C
#define TC_CATFOREST_C

#ifndef SMALL
#define SMALL (1e-10)
#endif

#ifndef MAX_FORESTS
#define MAX_FORESTS (1024)
#endif

void usage()
{
    fprintf(stderr, "\n");
    fprintf(stderr, "catforest [OPTIONS] <forest1.rf> <forest2.rf> ...\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Use a mandatory '-o' to specify output, followed \n");
    fprintf(stderr, "by a list of decision forests to concatenate.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -o <forest.rf>        output file (no default)\n");
    fprintf(stderr, "\n");
    return;
}

int main(int argc, char **argv)
{
    fprintf(stderr, "Here we go!\n");
    int arg = 1, i, j;
    char *output_filename = NULL;
    tc_forest *forests[MAX_FORESTS];
    tc_colormap *colormaps[MAX_FORESTS];
    tc_colormap *master_colormap = NULL;
    int nforests = 0;
    int ntrees = 0;
    int ignorecolors = 0;

    if (argc < 3)
    {
        usage();
        exit(-1);
    }

    fprintf(stderr, "parsing arguments\n");
    while (argv[arg][0] == '-')
    {
        if (argv[arg][1] == 'o')
        {
            arg++;
            output_filename = argv[arg];
            fprintf(stderr,"I'll output to %s\n", output_filename);
        }
        else if (strncmp(argv[arg],"--ignorecolors",14) == 0)
        {
            ignorecolors = 1;
            fprintf(stderr, "Ignoring all color maps\n");
        }
        arg++;
    }
    if (output_filename == NULL)
    {
        fprintf(stderr,"Use '-o <output>' to specify an output file\n");
        usage();
        exit(-1);
    }

    while (arg<argc)
    {
        fprintf(stderr,"loading forest %s\n", argv[arg]);
        if (tc_load_forest(&(forests[nforests]), &(colormaps[nforests]),
                           argv[arg]) == ERR)
        {
            fprintf(stderr,"Failed to load input forest %s\n", argv[arg]);
            for (i=0; i<nforests; i++)
            {
                tc_free_forest(forests[i]);
                tc_free_colormap(colormaps[i]);
            }
            exit(-1);
        }
        ntrees = ntrees + forests[nforests]->ntrees;
        if (nforests > 0)
        {
            if (forests[0]->filterset != forests[nforests]->filterset)
            {
                fprintf(stderr,"filtersets do not match\n");
                exit(-1);
            }
            if (forests[0]->nclasses != forests[nforests]->nclasses)
            {
                fprintf(stderr,"number of classes does not match\n");
                exit(-1);
            }

            /* Merge the current color map with the master list */
            if (!ignorecolors && colormaps[nforests] != NULL)
            {
                if (colormaps[nforests]->colordepth != master_colormap->colordepth)
                {
                    fprintf(stderr,
                            "Color depth of color maps do not match, not merging map from %s\n",
                            argv[arg]);
                }
                else
                {
                    for (i = 0; i < master_colormap->nclasses; i++)
                    {
                        int matched = 1;
                        for (j = 0; j < master_colormap->colordepth; j++)
                        {
                            if (colormaps[nforests]->colormap[i][j] != master_colormap->colormap[i][j])
                            {
                                matched = 0;
                            }
                        }

                        if (!matched)
                        {
                            fprintf(stderr,"Class colors do not match between master list and %s\n",
                                    argv[arg]);
                        }
                    }
                    /* Add on any extra classes */
                    for (i = master_colormap->nclasses; i < colormaps[nforests]->nclasses; i++)
                    {
                        for (j = 0; j < colormaps[nforests]->colordepth; j++)
                        {
                            master_colormap->colormap[i][j] = colormaps[nforests]->colormap[i][j];
                        }
                    }
                }
            }
        }
        else if (!ignorecolors && colormaps[nforests] != NULL)
        {
            /* Assumes the color maps from all forests are the same depth as forest 0! */
            tc_init_colormap(&master_colormap, colormaps[nforests]->colordepth);

            for (i = 0; i < colormaps[nforests]->nclasses; i++)
            {
                for (j = 0; j < colormaps[nforests]->colordepth; j++)
                {
                    master_colormap->colormap[i][j] = colormaps[nforests]->colormap[i][j];
                }
            }
        }

        nforests++;
        arg++;
    }

    FILE *fout = fopen(output_filename, "w");

    if (fout == NULL)
    {
        fprintf(stderr,"could not output to file.\n");
        return ERR;
    }

    fprintf(fout,"forest %i %i %i\n",
            ntrees,
            forests[0]->filterset,
            forests[0]->nclasses);

    int tree = 0;
    for (i=0; i<nforests; i++)
    {
        for (j=0; j<forests[i]->ntrees; j++)
        {
            fprintf(fout,"\ntree %d\n", tree);
            if (tc_write_tree(&(forests[i]->trees[j]),
                              fout, forests[0]->nclasses) == ERR)
            {
                fprintf(stderr,"error writing to file.\n");
                return ERR;
            }
            tree++;
        }
    }

    if (!ignorecolors && master_colormap != NULL)
    {
        fprintf(fout,"\ncolormap %d\n", master_colormap->colordepth);
        for (i=0; i<forests[0]->nclasses; i++)
        {
            for (j = 0; j < master_colormap->colordepth; j++)
            {
                fprintf(fout, "%d ", (int)master_colormap->colormap[i][j]);
            }
            fprintf(fout, "\n");
        }
    }
    fclose(fout);

    for (i=0; i<nforests; i++)
    {
        tc_free_forest(forests[i]);
    }
    return OK;
}




#endif
