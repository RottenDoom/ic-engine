#include "darray.h"

#include "core/ic_memory.h"
#include "core/logger.h"

static memory mem;

void* darray::darray_create(u64 length, u64 stride) {
    u64 head = DARRAY_FIELD_LENGTH * sizeof(u64); // create memory for header space that defines the properties of array
    u64 array_size = length * stride; // the size of the array
    u64* new_array = (u64*)mem.ic_allocate(head + array_size, MEMORY_TAG_DARRAY); // allocate the joint memory
    mem.ic_set_memory(new_array, 0, head + array_size); // make sure memory set to zero
    new_array[DARRAY_CAPACITY] = length; // header info setup
    new_array[DARRAY_LENGTH] = 0;
    new_array[DARRAY_STRIDE] = stride;
    return (void*)(new_array + DARRAY_FIELD_LENGTH); // slide pointers by 24 bytes and return the array
}

void darray::darray_destroy(void* array) {
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
    u64 total_size = header_size + header[DARRAY_CAPACITY] * header[DARRAY_STRIDE]; // calculate total size to free memory
    mem.ic_free(header, total_size, MEMORY_TAG_DARRAY);
}

void darray::darray_push(void* array, const void* value_ptr) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (length >= darray_capacity(array)) {
        array = darray_resize(array);
    }

    u64 addr = (u64)array;
    addr += (length * stride);
    mem.ic_copy_memory((void*)addr, value_ptr, stride);
    _darray_field_set(array, DARRAY_LENGTH, length + 1);
}

void darray::darray_pop(void* array, void* dest) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    u64 addr = (u64)array;
    addr += ((length - 1) * stride);
    mem.ic_copy_memory(dest, (void*)addr, stride);
    _darray_field_set(array, DARRAY_LENGTH, length -1);
}

void* darray::darray_pop_at(void* array, u64 index, void* dest) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (index >= length) {
        IC_ERROR("Index outside the bounds of this array! Lenght: %i, index: %index", length, index);
        return array;
    }

    u64 addr = (u64)array;
    mem.ic_copy_memory(dest, (void*)(addr + (index * stride)), stride);

    // If not on the last element, snip out the entry and copy the rest inward.
    if (index != length - 1) {
        mem.ic_copy_memory(
            (void*)(addr + (index * stride)),
            (void*)(addr + ((index + 1) * stride)),
            stride * (length - index));
    }

    _darray_field_set(array, DARRAY_LENGTH, length - 1);
    return array;
}

void darray::darray_insert_at(void* array, u64 index, void* value_ptr) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if (index >= length) {
        IC_ERROR("Index outside the bounds of this array! Length: %i, index: %index", length, index);
    }
    if (length >= darray_capacity(array)) {
        array = darray_resize(array);
    }

    u64 addr = (u64)array;

    // If not on the last element, copy the rest outward.
    if (index != length - 1) {
        mem.ic_copy_memory(
            (void*)(addr + ((index + 1) * stride)),
            (void*)(addr + (index * stride)),
            stride * (length - index));
    }

    // Set the value at the index
    mem.ic_copy_memory((void*)(addr + (index * stride)), value_ptr, stride);

    _darray_field_set(array, DARRAY_LENGTH, length + 1);
}

u64 darray::_darray_field_get(void* array, u64 field) {
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;
    return *(header + field);
}

void darray::_darray_field_set(void* array, u64 field, u64 value) {
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;
    header[field] = value;
}

void* darray::darray_resize(void* array) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);

    void* temp = darray_create(
        (DARRAY_RESIZE_FACTOR * darray_capacity(array)),
        stride);
    mem.ic_copy_memory(temp, array, length * stride);

    _darray_field_set(temp, DARRAY_LENGTH, length);
    darray_destroy(array);
    return temp;
}

void darray::darray_clear(void* array) {
    _darray_field_set(array, DARRAY_LENGTH, 0);
}

u64 darray::darray_capacity(void* array) {
    return _darray_field_get(array, DARRAY_CAPACITY);
}

u64 darray::darray_length(void* array) {
    return _darray_field_get(array, DARRAY_LENGTH);
}

u64 darray::darray_stride(void* array) {
    return _darray_field_get(array, DARRAY_STRIDE);
}

void darray::darray_length_set(void* array, u64 value) {
    _darray_field_set(array, DARRAY_LENGTH, value);
}

darray::darray() {
}

darray::~darray() {
}


