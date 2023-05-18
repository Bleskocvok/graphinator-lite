
// c
#include <stdlib.h>         // calloc, free

#include "queue.h"


void queue_init( queue_t* queue, size_t capacity )
{
    *queue = (queue_t){ 0 };
    queue->vals = calloc( capacity, sizeof( *queue->vals ) );
    queue->capacity = capacity;
}


void queue_free( queue_t* queue )
{
    free( queue->vals );
    *queue = (queue_t){ 0 };
}


size_t queue_count( const queue_t* queue )
{
    return queue->count;
}


size_t queue_capacity( const queue_t* queue )
{
    return queue->capacity;
}


double queue_at( const queue_t* queue, size_t i )
{
    return queue->vals[ (i + queue->end) % queue->capacity ];
}


void queue_push( queue_t* queue, double val )
{
    if (queue->count < queue->capacity)
    {
        queue->vals[ queue->count ] = val;
        queue->count++;
    }
    else
    {
        queue->vals[ queue->end ] = val;
        queue->end = (queue->end + 1) % queue->capacity;
    }
}


void queue_resize( queue_t* queue, size_t new_cap )
{
    if ( new_cap == queue->capacity )
        return;

    queue_t new_queue = { 0 };
    queue_init( &new_queue, new_cap );

    for ( size_t i = 0; i < queue->count; i++ )
        queue_push( &new_queue, queue_at( queue, i ) );

    queue_free( queue );
    *queue = new_queue;
}
