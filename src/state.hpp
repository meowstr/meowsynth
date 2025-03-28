#pragma once

#include <string.h>

template < typename T > void array_swap_last( T * arr, int count, int index )
{
    memcpy( arr + index, arr + count - 1, sizeof( T ) );
}


struct state_t {
    float freq;
};

extern state_t state;
