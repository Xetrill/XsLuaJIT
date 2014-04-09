#pragma once
#ifndef XSLUALIB_XS_CB_H
#define XSLUALIB_XS_CB_H

#include <stdlib.h> // malloc, calloc, free
#include <string.h> // memcmp

//==============================================================================

#define XSCB_MIN_SIZE 2


//==============================================================================

typedef struct {
    int   size;   /* maximum number of elements           */
    int   start;  /* index of oldest element              */
    int   end;    /* index at which to write new element  */
    int   s_msb;
    int   e_msb;
    int*  elems;  /* vector of elements                   */
} CircularBuffer;


//==============================================================================

void cbInit(CircularBuffer* cb, int size, bool allocElements = true)
{
    cb->size  = size;
    cb->start = 0;
    cb->end   = 0;
    cb->s_msb = 0;
    cb->e_msb = 0;
    if (allocElements)
        cb->elems = static_cast<int*>(calloc(size, sizeof(int)));
}

void cbFree(CircularBuffer* cb)
{
    if (cb == NULL)
        return;
    cb->start = NULL;
    cb->end   = NULL;
    cb->s_msb = NULL;
    cb->e_msb = NULL;
    free(cb->elems);
    cb->elems = NULL;
    cb = NULL;
}

CircularBuffer* cbMake(int size)
{
    CircularBuffer* cb = static_cast<CircularBuffer*>(
        malloc(sizeof(CircularBuffer) + sizeof(int) * size)
    );
    cbInit(cb, size, false);
    return cb;
}

void cbNext(CircularBuffer* cb, int *p, int *msb)
{
    *p = *p + 1;
    if (*p == cb->size)
    {
        *msb ^= 1;
        *p    = 0;
    }
}

bool cbFull(const CircularBuffer* cb)
{
    return (cb->end == cb->start && cb->e_msb != cb->s_msb) ? true : false;
}

bool cbEmpty(const CircularBuffer* cb)
{
    return (cb->end == cb->start && cb->e_msb == cb->s_msb) ? true : false;
}

static void cbPut(CircularBuffer* cb, int elem, int* o_elem = NULL)
{
    if (o_elem)
        *o_elem = cb->elems[cb->end];

    cb->elems[cb->end] = elem;

    if (cbFull(cb))
        cbNext(cb, &cb->start, &cb->s_msb);
    cbNext(cb, &cb->end, &cb->e_msb);
}

/// Caller has to make sure that the buffer isn't empty.
void cbGet(CircularBuffer* cb, int* o_elem)
{
    *o_elem = cb->elems[cb->start];
    cbNext(cb, &cb->start, &cb->s_msb);
}

bool cbEquals(const CircularBuffer* lhs, const CircularBuffer* rhs)
{
    if (lhs == rhs)
        return true;

    if (lhs == NULL || rhs == NULL)
        return false;
    if (lhs->size != rhs->size)
        return false;

    return (memcmp(lhs->elems, rhs->elems, lhs->size) == 0) ? true : false;
}

#endif // XSLUALIB_XS_CB_H
