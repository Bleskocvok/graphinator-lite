#pragma once

#ifndef __GRAPH_H__
#define __GRAPH_H__


#include <stdio.h>      // FILE


typedef enum
{
    DOTS, FILL

} style_t;


typedef struct
{
    int align;
    int decimals;

} desc_t;


void render_graph(const double* values, int count, double max_value, int height,
                  const desc_t* desc, style_t style);


double* read_input(FILE* file, int* count_out);


int style_from_str(const char* str, style_t* style);


#endif // __GRAPH_H__
