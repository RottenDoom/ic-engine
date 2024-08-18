#pragma once

#include "defines.h"

#define DARRAY_DEFAULT_CAPACITY 1
#define DARRAY_RESIZE_FACTOR 2

enum {
    DARRAY_CAPACITY,    // u64 capacity | number of elements that can be stored
    DARRAY_LENGTH,      // u64 length   | number of elements currently contained
    DARRAY_STRIDE,      // u64 stride   | size of each element in bites
    DARRAY_FIELD_LENGTH // u64 field    | 3 byte space for header space that contains the length, stride and capacity.
};

class IC_API darray 
{
public:
    static void* darray_create(u64 length, u64 stride);

    void darray_destroy(void* array);

    void darray_push(void* array, const void* value_ptr);
    void darray_pop(void* array, void* dest);

    void* darray_pop_at(void* array, u64 index, void* dest);
    void darray_insert_at(void* array, u64 index, void* value_ptr);

    void darray_clear(void* array);

    u64 darray_capacity(void* array);
    u64 darray_length(void* array);
    u64 darray_stride(void* array);
    void darray_length_set(void* array, u64 value);
    darray();
    ~darray();

private:
    u64 _darray_field_get(void* array, u64 field);
    void _darray_field_set(void* array, u64 field, u64 value);

    void* darray_resize(void* array);
};

#define darray_type_create(type) darray::darray_create(DARRAY_DEFAULT_CAPACITY, sizeof(type))