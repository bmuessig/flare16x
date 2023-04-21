//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// error.c: Error handling functions
//

#include <stddef.h>

#include "error.h"

// A list of error strings to represent the error codes
const char* const flare16x_error_names[FLARE16X_ERROR_COUNT] = {
    // FLARE16X_ERROR_NONE
    "no error",
    // FLARE16X_ERROR_NULL
    "invalid null pointer",
    // FLARE16X_ERROR_MALLOC
    "memory allocation failed",
    // FLARE16X_ERROR_LEAK
    "memory leak avoided",
    // FLARE16X_ERROR_RANGE
    "invalid argument range",
    // FLARE16X_ERROR_OPEN
    "file open failed",
    // FLARE16X_ERROR_IO
    "I/O operation failed",
    // FLARE16X_ERROR_SYNTAX
    "syntax error",
    // FLARE16X_ERROR_FORMAT
    "file format error",
    // FLARE16X_ERROR_IMAGE
    "image size or feature error",
    // FLARE16X_ERROR_UNKNOWN
    "unknown value",
    // FLARE16X_ERROR_ASSERT
    "assert failed",
    // FLARE16X_ERROR_CALLEE
    "callee error",
    // FLARE16X_ERROR_OTHER
    "other unknown error"
};

// A list of error source strings to represent the error codes
const char* const flare16x_error_source_names[FLARE16X_ERROR_SOURCE_COUNT] = {
    // FLARE16X_ERROR_SOURCE_GLOBAL
    "global",
    // FLARE16X_ERROR_SOURCE_BITMAP
    "bitmap",
    // FLARE16X_ERROR_SOURCE_CANVAS
    "canvas",
    // FLARE16X_ERROR_SOURCE_LOCATOR
    "locator",
    // FLARE16X_ERROR_SOURCE_OCR
    "OCR",
    // FLARE16X_ERROR_SOURCE_PALETTES
    "palettes",
    // FLARE16X_ERROR_SOURCE_THERMAL
    "thermal"
};

// Returns a matching error name for the latest error on the stack
const char* flare16x_error_string(flare16x_error error)
{
    error &= FLARE16X_ERROR_MASK;

    if (error  >= FLARE16X_ERROR_COUNT)
        return "invalid error";

    return flare16x_error_names[error];
}

// Returns a matching error source name for the latest error on the stack
const char* flare16x_error_source_string(flare16x_error error)
{
    error &= FLARE16X_ERROR_SOURCE_MASK;
    error >>= FLARE16X_ERROR_WIDTH;

    if (error >= FLARE16X_ERROR_SOURCE_COUNT)
        return "invalid error source";

    return flare16x_error_source_names[error];
}

// Searches and returns the latest error from the stack and returns it
flare16x_error flare16x_error_first(flare16x_error error)
{
    // Search through the errors until the most recent non-zero item is found
    int stack_item;
    for (stack_item = FLARE16x_ERROR_CAPACITY - 1; stack_item >= 0; stack_item--)
    {
        flare16x_error current_error;
        current_error = error >> ((FLARE16X_ERROR_WIDTH + FLARE16X_ERROR_SOURCE_WIDTH) * stack_item) &
                (FLARE16X_ERROR_SOURCE_MASK | FLARE16X_ERROR_MASK);

        if (current_error != FLARE16X_ERROR_NONE)
            return current_error;
    }

    // If no error could be found, return none
    return FLARE16X_ERROR_NONE;
}

// Retrieves the latest error from the stack and returns it
flare16x_error  flare16x_error_latest(flare16x_error error)
{
    return error & (FLARE16X_ERROR_SOURCE_MASK | FLARE16X_ERROR_MASK);
}

// Pops an error from the error stack and returns it
flare16x_error flare16x_error_pop(flare16x_error* error)
{
    if (error == NULL)
        return FLARE16X_ERROR_NULL;

    flare16x_error latest_error = *error & (FLARE16X_ERROR_SOURCE_MASK | FLARE16X_ERROR_MASK);
    *error = *error >> (FLARE16X_ERROR_WIDTH + FLARE16X_ERROR_SOURCE_WIDTH);

    return latest_error;
}

// Pushes a new error onto the error stack (and perhaps removes the first error on overflow)
void flare16x_error_push(flare16x_error new_error, flare16x_error* prev_errors)
{
    if (prev_errors == NULL)
        return;

    *prev_errors = (*prev_errors << (FLARE16X_ERROR_WIDTH + FLARE16X_ERROR_SOURCE_WIDTH)) |
            (new_error & (FLARE16X_ERROR_SOURCE_MASK | FLARE16X_ERROR_MASK));
}

// Pushes a new error onto a copy of the error stack and returns it
flare16x_error flare16x_error_wrap(flare16x_error new_error, flare16x_error prev_errors)
{
    return (prev_errors << (FLARE16X_ERROR_WIDTH + FLARE16X_ERROR_SOURCE_WIDTH)) |
            (new_error & (FLARE16X_ERROR_SOURCE_MASK | FLARE16X_ERROR_MASK));
}
