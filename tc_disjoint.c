/**
 * \file disjoint.c
 *
 * \author David Ray Thompson\n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdlib.h>
#include <string.h>
#include "tc_disjoint.h"

#ifndef TC_DISJOINT_C
#define TC_DISJOINT_C


/* Initialize the merge union structure to entirely disjoint sets */
int tc_init_disjoint(tc_disjoint **d, int nlabels)
{
    int i;
    if (d==NULL || nlabels < 1)
    {
        tc_write_log("Bad parameters to merge union initialization!\n");
        return ERR;
    }

    tc_disjoint *dnew = (tc_disjoint *) malloc(sizeof(tc_disjoint));
    if (dnew == NULL)
    {
        tc_write_log("Cannot initialize merge unions structure!\n");
        return ERR;
    }

    dnew->nlabels = nlabels;
    dnew->parent = (int *) malloc(sizeof(int) * nlabels);
    if (dnew->parent  == NULL)
    {
        tc_write_log("Cannot initialize merge union parents!\n");
        free(dnew);
        return ERR;
    }

    dnew->rank   = (int *) malloc(sizeof(int) * nlabels);
    if (dnew->rank == NULL)
    {
        tc_write_log("Cannot initialize merge union ranks\n");
        free(dnew->parent);
        free(dnew);
        return ERR;
    }

    for (i=0; i<nlabels; i++)
    {
        dnew->parent[i] = i;
        dnew->rank[i] = 1;
    }
    *d = dnew;
    return OK;
}


/* Freeeeeedom!!! */
void tc_free_disjoint(tc_disjoint *d)
{
    if (d==NULL)
    {
        tc_write_log("Cannot free a null pointer!\n");
        return;
    }
    free(d->parent);
    free(d->rank);
    free(d);
}


/* Find the equivalence class for a given point */
int tc_find_disjoint(tc_disjoint *d, int i)
{
    while (d->parent[i] != i)
    {
        i = d->parent[i];
    }
    return i;
}


/* Merge two equivalence classes into a (hopefully somewhat balanced) tree */
void tc_merge_disjoint(tc_disjoint *d, int i, int j)
{
    int ic = tc_find_disjoint(d,i);
    int jc = tc_find_disjoint(d,j);

    if (d->rank[ic] > d->rank[jc])
    {
        d->parent[jc] = ic;
        if (d->rank[ic] == d->rank[jc])
        {
            d->rank[ic]++;
        }
    }
    else
    {
        d->parent[ic] = jc;
        if (d->rank[ic] == d->rank[jc])
        {
            d->rank[jc]++;
        }
    }
}

#endif
