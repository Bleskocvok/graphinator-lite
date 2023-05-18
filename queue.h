#pragma once

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdlib.h>         // size_t


typedef struct
{
    double* vals;
    size_t capacity;
    size_t count;
    size_t end;

} queue_t;


void queue_init( queue_t* queue, size_t capacity );

void queue_free( queue_t* queue );

size_t queue_count( const queue_t* queue );

size_t queue_capacity( const queue_t* queue );

double queue_at( const queue_t* queue, size_t i );

void queue_push( queue_t* queue, double val );

void queue_resize( queue_t* queue, size_t capacity );


#endif  // __QUEUE_H__
