/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2001 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "blockallocator.h"

BlockAllocator MM_Car_allocator;

void mm_car_init()
{
    MM_Car_allocator = BlockAllocInit(sizeof(MM_Car), 100, 100, malloc, free);
}

void mm_car_finalize()
{
    BlockAllocFinalize(&MM_Car_allocator);
}

MM_Car* mm_car_alloc()
{
    MM_Car *pCar;
    pCar = BlockAlloc(MM_Car_allocator);
    pCar->freeme = TRUE;
    return pCar;
}

void mm_car_free(MM_Car *car_ptr)
{
    if (car_ptr->freeme)
	BlockFree(MM_Car_allocator, car_ptr);
}
