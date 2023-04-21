//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// error.h: Header file for error handling functions
//

#ifndef FLARE16X_ERROR_H
#define FLARE16X_ERROR_H

#include <stdint.h>

// This type represents an error stack
typedef uint32_t flare16x_error;

// An enum containing all error reasons
enum {
    // No error
    FLARE16X_ERROR_NONE,
    // An argument is a null pointer
    FLARE16X_ERROR_NULL,
    // A new buffer could not be allocated
    FLARE16X_ERROR_MALLOC,
    // A memory leak through double initialization was avoided (where detected)
    FLARE16X_ERROR_LEAK,
    // The range of an argument is incorrect
    FLARE16X_ERROR_RANGE,
    // There has been an error opening a file
    FLARE16X_ERROR_OPEN,
    // There has been an error with file I/O
    FLARE16X_ERROR_IO,
    // There has been a syntax error with the command line input
    FLARE16X_ERROR_SYNTAX,
    // There has been a file format error
    FLARE16X_ERROR_FORMAT,
    // The image format is valid, but has an invalid size or does not contain all features
    FLARE16X_ERROR_IMAGE,
    // The argument or image value is unknown to the program
    FLARE16X_ERROR_UNKNOWN,
    // An assert failed
    FLARE16X_ERROR_ASSERT,
    // An callee returned an error
    FLARE16X_ERROR_CALLEE,
    // There has been an unknown error
    FLARE16X_ERROR_OTHER,
    // The number of errors known
    FLARE16X_ERROR_COUNT,
    // The error mask used to differentiate the stacked errors
    FLARE16X_ERROR_MASK = 0x0fu,
    // The width in bits of one error reason
    FLARE16X_ERROR_WIDTH = 0x4u,
    // The maximum number of errors contained inside an error stack
    FLARE16x_ERROR_CAPACITY = 0x4u
};

// An enum containing all error sources
enum {
    // Global / unknown function
    FLARE16X_ERROR_SOURCE_GLOBAL,
    // Bitmap
    FLARE16X_ERROR_SOURCE_BITMAP,
    // Canvas
    FLARE16X_ERROR_SOURCE_CANVAS,
    // Locator
    FLARE16X_ERROR_SOURCE_LOCATOR,
    // OCR
    FLARE16X_ERROR_SOURCE_OCR,
    // Palettes
    FLARE16X_ERROR_SOURCE_PALETTES,
    // Thermal
    FLARE16X_ERROR_SOURCE_THERMAL,
    // The number of error sources known
    FLARE16X_ERROR_SOURCE_COUNT,
    // The error source mask used to differentiate the stacked error sources
    FLARE16X_ERROR_SOURCE_MASK = 0xf0u,
    // The width in bits of one error source
    FLARE16X_ERROR_SOURCE_WIDTH = 0x4u
};

// Assembles a new error from an error and a source
#define flare16x_error_make(error,source) \
(((source) & (FLARE16X_ERROR_SOURCE_MASK) << (FLARE16X_ERROR_WIDTH)) | ((error) & (FLARE16X_ERROR_MASK)))

// Gets the reason from the error
#define flare16x_error_reason(error) ((error) & (FLARE16X_ERROR_MASK))

// Gets the source from the error
#define flare16x_error_source(error) (((error) & (FLARE16X_ERROR_SOURCE_MASK)) >> (FLARE16X_ERROR_WIDTH))

// Returns a matching error name for the latest error on the stack
const char* flare16x_error_string(flare16x_error error);

// Returns a matching error source name for the latest error on the stack
const char* flare16x_error_source_string(flare16x_error error);

// Contains a list of error strings to represent the error codes
extern const char* const flare16x_error_names[];

// Contains a list of error source strings to represent the error codes
extern const char* const flare16x_error_source_names[];

// Searches and returns the latest error from the stack and returns it
flare16x_error flare16x_error_first(flare16x_error error);

// Retrieves the latest error from the stack and returns it
flare16x_error flare16x_error_latest(flare16x_error error);

// Pops an error from the error stack and returns it
flare16x_error flare16x_error_pop(flare16x_error* error);

// Pushes a new error onto the error stack (and perhaps removes the first error on overflow)
void flare16x_error_push(flare16x_error new_error, flare16x_error* prev_errors);

// Pushes a new error onto a copy of the error stack and returns it
flare16x_error flare16x_error_wrap(flare16x_error new_error, flare16x_error prev_errors);

#endif //FLARE16X_ERROR_H
