#pragma once

#ifndef __GRAPH_H__
#define __GRAPH_H__


#include <stdio.h>      // FILE


typedef struct
{
    int align;
    int decimals;

} desc_t;


void render_graph(const double* values, int count, double max_value, int height,
                  const desc_t* desc);


double* read_input(FILE* file, int* count_out);



#endif // __GRAPH_H__
