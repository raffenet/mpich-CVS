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
    MM_ENTER_FUNC(MM_CAR_INIT);
    MM_Car_allocator = BlockAllocInit(sizeof(MM_Car), 100, 100, malloc, free);
    MM_EXIT_FUNC(MM_CAR_INIT);
}

void mm_car_finalize()
{
    MM_ENTER_FUNC(MM_CAR_FINALIZE);
    BlockAllocFinalize(&MM_Car_allocator);
    MM_EXIT_FUNC(MM_CAR_FINALIZE);
}

MM_Car* mm_car_alloc()
{
    MM_Car *pCar;

    MM_ENTER_FUNC(MM_CAR_ALLOC);

    pCar = BlockAlloc(MM_Car_allocator);
    pCar->freeme = TRUE;

    MM_EXIT_FUNC(MM_CAR_ALLOC);

    return pCar;
}

void mm_car_free(MM_Car *car_ptr)
{
    MM_ENTER_FUNC(MM_CAR_FREE);
    if (car_ptr->freeme)
	BlockFree(MM_Car_allocator, car_ptr);
    MM_EXIT_FUNC(MM_CAR_FREE);
}
