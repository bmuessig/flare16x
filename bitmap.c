//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// bitmap.c: Lightweight bitmap read/write implementation
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "error.h"
#include "canvas.h"

#include "bitmap.h"

// Creates a new 16-bit RGB565 bitmap, allocates the required memory and fills in the values
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_create16(uint16_t width, uint16_t height, flare16x_bitmap* bitmap)
{
    // Make sure the bitmap is not null
    if (bitmap == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Verify width and height
    if (width == 0 || height == 0 || height * width > FLARE16X_BITMAP_MAX_PIXELS)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_BITMAP);

    // Clear the target struct
    memset(bitmap, 0, sizeof(flare16x_bitmap));

    // Calculate the stride
    bitmap->stride = ((((width * 16) + 31) & ~31) >> 3);

    // Calculate the required memory
    bitmap->dib_size = sizeof(flare16x_bitmap_dib);
    bitmap->mask_size = sizeof(flare16x_bitmap_mask);
    bitmap->pixels_size = height * bitmap->stride;
    size_t total_size = bitmap->dib_size + bitmap->mask_size + bitmap->pixels_size + sizeof(flare16x_bitmap_header);

    // Allocate the required memory and clear it
    bitmap->header = malloc(sizeof(flare16x_bitmap_header));
    if (bitmap->header == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    memset(bitmap->header, 0, sizeof(flare16x_bitmap_header));
    bitmap->dib = malloc(bitmap->dib_size + bitmap->mask_size);
    bitmap->mask = (void*)bitmap->dib + bitmap->dib_size;
    if (bitmap->dib == NULL)
    {
        free(bitmap->header);
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }
    memset(bitmap->dib, 0, bitmap->dib_size + bitmap->mask_size);
    bitmap->pixels = malloc(bitmap->pixels_size);
    if (bitmap->pixels == NULL)
    {
        free(bitmap->dib);
        free(bitmap->header);
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }
    memset(bitmap->pixels, 0, bitmap->pixels_size);

    // Next, fill in the header struct
    bitmap->header->magic = FLARE16X_BITMAP_HEADER_MAGIC;
    bitmap->header->reserved = FLARE16X_BITMAP_HEADER_RESERVED;
    bitmap->header->file_size = total_size;
    bitmap->header->payload_offset = bitmap->dib_size + bitmap->mask_size + sizeof(flare16x_bitmap_header);

    // Followed by the DIB struct
    bitmap->dib->height = -height;
    bitmap->dib->width = width;
    bitmap->dib->compression = FLARE16X_BITMAP_COMPRESSION_BITFIELDS;
    bitmap->dib->bit_count = 16;
    bitmap->dib->planes = 1;
    bitmap->dib->size = bitmap->dib_size;
    bitmap->dib->size_image = bitmap->pixels_size;
    bitmap->dib->colors_important = 0;
    bitmap->dib->colors_used = 0;
    bitmap->dib->hor_px_per_meter = 0;
    bitmap->dib->ver_px_per_meter = 0;

    // And followed by the mask struct
    bitmap->mask->mask_red = 0xf800;
    bitmap->mask->mask_green = 0x7e0;
    bitmap->mask->mask_blue = 0x1f;

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}

// Creates a new 24-bit RGB888 bitmap, allocates the required memory and fills in the values
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_create24(uint16_t width, uint16_t height, flare16x_bitmap* bitmap)
{
    // Make sure the bitmap is not null
    if (bitmap == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Verify width and height
    if (width == 0 || height == 0 || height * width > FLARE16X_BITMAP_MAX_PIXELS)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_BITMAP);

    // Clear the target struct
    memset(bitmap, 0, sizeof(flare16x_bitmap));

    // Calculate the stride
    bitmap->stride = ((((width * 24) + 31) & ~31) >> 3);

    // Calculate the required memory
    bitmap->dib_size = sizeof(flare16x_bitmap_dib);
    bitmap->mask_size = 0;
    bitmap->pixels_size = height * bitmap->stride;
    size_t total_size = bitmap->dib_size + bitmap->pixels_size + sizeof(flare16x_bitmap_header);

    // Allocate the required memory and clear it
    bitmap->header = malloc(sizeof(flare16x_bitmap_header));
    if (bitmap->header == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    memset(bitmap->header, 0, sizeof(flare16x_bitmap_header));
    bitmap->dib = malloc(bitmap->dib_size);
    bitmap->mask = NULL;
    if (bitmap->dib == NULL)
    {
        free(bitmap->header);
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }
    memset(bitmap->dib, 0, bitmap->dib_size + bitmap->mask_size);
    bitmap->pixels = malloc(bitmap->pixels_size);
    if (bitmap->pixels == NULL)
    {
        free(bitmap->dib);
        free(bitmap->header);
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }
    memset(bitmap->pixels, 0, bitmap->pixels_size);

    // Next, fill in the header struct
    bitmap->header->magic = FLARE16X_BITMAP_HEADER_MAGIC;
    bitmap->header->reserved = FLARE16X_BITMAP_HEADER_RESERVED;
    bitmap->header->file_size = total_size;
    bitmap->header->payload_offset = bitmap->dib_size + sizeof(flare16x_bitmap_header);

    // Followed by the DIB struct
    bitmap->dib->height = -height;
    bitmap->dib->width = width;
    bitmap->dib->compression = FLARE16X_BITMAP_COMPRESSION_RGB;
    bitmap->dib->bit_count = 24;
    bitmap->dib->planes = 1;
    bitmap->dib->size = bitmap->dib_size;
    bitmap->dib->size_image = bitmap->pixels_size;
    bitmap->dib->colors_important = 0;
    bitmap->dib->colors_used = 0;
    bitmap->dib->hor_px_per_meter = 0;
    bitmap->dib->ver_px_per_meter = 0;

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}

// Creates a new 32-bit RGBA8888 bitmap, allocates the required memory and fills in the values
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_create32(uint16_t width, uint16_t height, flare16x_bitmap* bitmap)
{
    // Make sure the bitmap is not null
    if (bitmap == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Verify width and height
    if (width == 0 || height == 0 || height * width > FLARE16X_BITMAP_MAX_PIXELS)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_BITMAP);

    // Clear the target struct
    memset(bitmap, 0, sizeof(flare16x_bitmap));

    // Calculate the stride
    bitmap->stride = ((((width * 32) + 31) & ~31) >> 3);

    // Calculate the required memory
    bitmap->dib_size = sizeof(flare16x_bitmap_dib);
    bitmap->mask_size = 0;
    bitmap->pixels_size = height * bitmap->stride;
    size_t total_size = bitmap->dib_size + bitmap->pixels_size + sizeof(flare16x_bitmap_header);

    // Allocate the required memory and clear it
    bitmap->header = malloc(sizeof(flare16x_bitmap_header));
    if (bitmap->header == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    memset(bitmap->header, 0, sizeof(flare16x_bitmap_header));
    bitmap->dib = malloc(bitmap->dib_size);
    bitmap->mask = NULL;
    if (bitmap->dib == NULL)
    {
        free(bitmap->header);
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }
    memset(bitmap->dib, 0, bitmap->dib_size + bitmap->mask_size);
    bitmap->pixels = malloc(bitmap->pixels_size);
    if (bitmap->pixels == NULL)
    {
        free(bitmap->dib);
        free(bitmap->header);
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }
    memset(bitmap->pixels, 0, bitmap->pixels_size);

    // Next, fill in the header struct
    bitmap->header->magic = FLARE16X_BITMAP_HEADER_MAGIC;
    bitmap->header->reserved = FLARE16X_BITMAP_HEADER_RESERVED;
    bitmap->header->file_size = total_size;
    bitmap->header->payload_offset = bitmap->dib_size + sizeof(flare16x_bitmap_header);

    // Followed by the DIB struct
    bitmap->dib->height = -height;
    bitmap->dib->width = width;
    bitmap->dib->compression = FLARE16X_BITMAP_COMPRESSION_RGB;
    bitmap->dib->bit_count = 32;
    bitmap->dib->planes = 1;
    bitmap->dib->size = bitmap->dib_size;
    bitmap->dib->size_image = bitmap->pixels_size;
    bitmap->dib->colors_important = 0;
    bitmap->dib->colors_used = 0;
    bitmap->dib->hor_px_per_meter = 0;
    bitmap->dib->ver_px_per_meter = 0;

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}

// Attempts to load a bitmap from file
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_load(FILE* bitmap_file, flare16x_bitmap* bitmap_struct)
{
    // Make sure that there are no null pointers
    if (bitmap_file == NULL || bitmap_struct == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Next, clean up the target struct
    memset(bitmap_struct, 0, sizeof(flare16x_bitmap));

    // Allocate the memory for the header struct
    bitmap_struct->header = malloc(sizeof(flare16x_bitmap_header));
    if (bitmap_struct->header == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);

    // Then, read the header from the input file
    if (fread(bitmap_struct->header, sizeof(flare16x_bitmap_header), 1, bitmap_file) != 1)
    {
        // On failure, free the buffer struct again and return failure
        free(bitmap_struct->header);
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_IO, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // Now, validate the resulting struct starting with the magic number, reserved field, size and offset
    if (bitmap_struct->header->magic != FLARE16X_BITMAP_HEADER_MAGIC ||
        bitmap_struct->header->reserved != FLARE16X_BITMAP_HEADER_RESERVED ||
        bitmap_struct->header->file_size == 0 ||
        (bitmap_struct->header->payload_offset != 0x36 && bitmap_struct->header->payload_offset != 0x42))
    {
        // On failure, free the buffer struct again and return failure
        free(bitmap_struct->header);
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // Allocate more memory for the DIB and mask structs
    // The data offset minus the size of the file header yields the entire DIB plus optional mask size
    size_t dib_mask_size = bitmap_struct->header->payload_offset - sizeof(flare16x_bitmap_header);
    bitmap_struct->dib = malloc(dib_mask_size);
    if (bitmap_struct->dib == NULL)
    {
        // On failure, free the buffer struct again and return failure
        free(bitmap_struct->header);
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // Read the DIB struct from the input file
    if (fread(bitmap_struct->dib, dib_mask_size, 1, bitmap_file) != 1)
    {
        // On failure, free the buffer structs again and return failure
        free(bitmap_struct->dib);
        free(bitmap_struct->header);
        bitmap_struct->dib = NULL;
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_IO, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // Verify, that the reported DIB size, planes, bit-count, compression and maximum size matches
    if ((bitmap_struct->dib->size + sizeof(flare16x_bitmap_mask) != dib_mask_size &&
        bitmap_struct->dib->size != dib_mask_size) || bitmap_struct->dib->planes != 1 ||
        bitmap_struct->dib->width <= 0 || bitmap_struct->dib->height == 0 ||
        bitmap_struct->dib->width * abs(bitmap_struct->dib->height) > FLARE16X_BITMAP_MAX_PIXELS)
    {
        // On failure, free the buffer structs again and return failure
        free(bitmap_struct->dib);
        free(bitmap_struct->header);
        bitmap_struct->dib = NULL;
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // Copy the dib size
    bitmap_struct->dib_size = bitmap_struct->dib->size;

    // Calculate the stride
    bitmap_struct->stride = ((((bitmap_struct->dib->width * bitmap_struct->dib->bit_count) + 31) & ~31) >> 3);

    // TODO: Simplify this if-clause and merge the two equal outcomes
    // Next, determine the number of bits and calculate the image data size
    if (bitmap_struct->dib->bit_count == 16 && bitmap_struct->header->payload_offset == 0x42 &&
        bitmap_struct->dib->compression == FLARE16X_BITMAP_COMPRESSION_BITFIELDS)
    {
        // For 16 bit RGB565 calculate the mask struct pointer
        bitmap_struct->mask = (flare16x_bitmap_mask*)((void*)bitmap_struct->dib + bitmap_struct->dib->size);
        bitmap_struct->mask_size = dib_mask_size - bitmap_struct->dib->size;

        // As well as the pixel size
        bitmap_struct->pixels_size = bitmap_struct->stride * abs(bitmap_struct->dib->height);

        // And verify the masks
        if (bitmap_struct->mask->mask_red != FLARE16X_BITMAP_MASK_RGB565_RED ||
            bitmap_struct->mask->mask_green != FLARE16X_BITMAP_MASK_RGB565_GREEN ||
            bitmap_struct->mask->mask_blue != FLARE16X_BITMAP_MASK_RGB565_BLUE)
        {
            // On failure, free the buffer structs again and return failure
            free(bitmap_struct->dib);
            free(bitmap_struct->header);
            bitmap_struct->dib = NULL;
            bitmap_struct->header = NULL;
            return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
        }
    } else if (bitmap_struct->dib->bit_count == 24 && bitmap_struct->header->payload_offset == 0x36 &&
                bitmap_struct->dib->compression == FLARE16X_BITMAP_COMPRESSION_RGB)
    {
        // For 24-bit RGB888 calculate the pixel size
        bitmap_struct->pixels_size = bitmap_struct->stride * abs(bitmap_struct->dib->height);
    } else if (bitmap_struct->dib->bit_count == 32 && bitmap_struct->header->payload_offset == 0x36 &&
               bitmap_struct->dib->compression == FLARE16X_BITMAP_COMPRESSION_RGB)
    {
        // For 32-bit RGBA8888 calculate the pixel size
        bitmap_struct->pixels_size = bitmap_struct->stride * abs(bitmap_struct->dib->height);
    } else
    {
        // On failure, free the buffer structs again and return failure
        free(bitmap_struct->dib);
        free(bitmap_struct->header);
        bitmap_struct->dib = NULL;
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // Since the header worked out well, allocate space for the image data next
    bitmap_struct->pixels = malloc(bitmap_struct->pixels_size);
    if (bitmap_struct->pixels == NULL)
    {
        // On failure, free the buffer structs again and return failure
        free(bitmap_struct->dib);
        free(bitmap_struct->header);
        bitmap_struct->dib = NULL;
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // Now, read the image data
    if (fread(bitmap_struct->pixels, bitmap_struct->pixels_size, 1, bitmap_file) != 1)
    {
        // On failure, free the buffer structs again and return failure
        free(bitmap_struct->pixels);
        free(bitmap_struct->dib);
        free(bitmap_struct->header);
        bitmap_struct->pixels = NULL;
        bitmap_struct->dib = NULL;
        bitmap_struct->header = NULL;
        return flare16x_error_make(FLARE16X_ERROR_IO, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // If the image is a stupid bottom up one, flip the darn thing
    if (bitmap_struct->dib->height > 0)
    {
        // A higher efficiency would be a nice improvement in the future, as C# uses this format (the device does not)
        uint8_t* new_pixel_data = malloc(bitmap_struct->pixels_size);
        if (new_pixel_data == NULL)
        {
            free(bitmap_struct->pixels);
            free(bitmap_struct->dib);
            free(bitmap_struct->header);
            bitmap_struct->pixels = NULL;
            bitmap_struct->dib = NULL;
            bitmap_struct->header = NULL;
            return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);
        }

        // Reorder the data line by line from bottom to top
        int y;
        for (y = 0; y < abs(bitmap_struct->dib->height); y++)
            memcpy(new_pixel_data + (y * bitmap_struct->stride),
                    bitmap_struct->pixels + ((abs(bitmap_struct->dib->height) - y - 1) * bitmap_struct->stride),
                    bitmap_struct->stride);

        // Free the old buffer
        free(bitmap_struct->pixels);

        // And assign the new buffer
        bitmap_struct->pixels = (void*)new_pixel_data;

        // Finally flip the sign of the height
        bitmap_struct->dib->height = -bitmap_struct->dib->height;
    }

    // As everything worked out, return success
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}

// Attempts to store a bitmap to file
flare16x_error flare16x_bitmap_store(flare16x_bitmap* bitmap_struct, FILE* bitmap_file)
{
    // Make sure that there are no null pointers
    if (bitmap_struct == NULL || bitmap_file == NULL || bitmap_struct->header == NULL || bitmap_struct->dib == NULL ||
        bitmap_struct->pixels == NULL || (bitmap_struct->mask_size > 0 && bitmap_struct->mask == NULL))
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Validate the format
    // Note that this requires short-circuiting to work properly!
    if (bitmap_struct->dib_size == 0 ||
    bitmap_struct->pixels_size == 0 || bitmap_struct->dib->width <= 0 || bitmap_struct->dib->height >= 0 ||
    bitmap_struct->dib->height * bitmap_struct->dib->width > FLARE16X_BITMAP_MAX_PIXELS || bitmap_struct->stride == 0 ||
    bitmap_struct->dib->planes != 1 || (bitmap_struct->dib->bit_count != 16 && bitmap_struct->dib->bit_count != 24 &&
    bitmap_struct->dib->bit_count != 32) || (bitmap_struct->dib->compression != FLARE16X_BITMAP_COMPRESSION_RGB &&
    bitmap_struct->dib->compression != FLARE16X_BITMAP_COMPRESSION_BITFIELDS) ||
    (bitmap_struct->dib->compression == FLARE16X_BITMAP_COMPRESSION_BITFIELDS &&
    bitmap_struct->dib->bit_count != 16) || (bitmap_struct->dib->compression == FLARE16X_BITMAP_COMPRESSION_RGB &&
    !(bitmap_struct->dib->bit_count == 24 || bitmap_struct->dib->bit_count == 32)))
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);

    // First, write the header struct
    if (fwrite(bitmap_struct->header, sizeof(flare16x_bitmap_header), 1, bitmap_file) != 1)
        return flare16x_error_make(FLARE16X_ERROR_IO, FLARE16X_ERROR_SOURCE_BITMAP);

    // Then, write the DIB struct
    if (fwrite(bitmap_struct->dib, bitmap_struct->dib_size, 1, bitmap_file) != 1)
        return flare16x_error_make(FLARE16X_ERROR_IO, FLARE16X_ERROR_SOURCE_BITMAP);

    // If desired, write the mask struct
    if (bitmap_struct->mask_size > 0 && bitmap_struct->dib->compression == FLARE16X_BITMAP_COMPRESSION_BITFIELDS)
        if (fwrite(bitmap_struct->mask, bitmap_struct->mask_size, 1, bitmap_file) != 1)
            return flare16x_error_make(FLARE16X_ERROR_IO, FLARE16X_ERROR_SOURCE_BITMAP);

    // Finally, write the pixel data
    if (fwrite(bitmap_struct->pixels, bitmap_struct->pixels_size, 1, bitmap_file) != 1)
        return flare16x_error_make(FLARE16X_ERROR_IO, FLARE16X_ERROR_SOURCE_BITMAP);

    // And, it's done!
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}

// Destroys an existing bitmap context and frees the resources
flare16x_error flare16x_bitmap_destroy(flare16x_bitmap* bitmap)
{
    // Make sure the bitmap is no null pointer
    if (bitmap == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Free all allocated memory
    free(bitmap->header);
    free(bitmap->dib);
    free(bitmap->pixels);

    // And zero the struct to get rid of all pointers and state
    memset(bitmap, 0, sizeof(flare16x_bitmap));

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}

// Copies a region from a bitmap buffer to a canvas buffer
// Will overwrite any existing canvas state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_edit(flare16x_bitmap* bitmap, uint16_t offset_x, uint16_t offset_y,
                                    uint16_t width, uint16_t height, flare16x_canvas* canvas)
{
    // Make sure that there are no null pointers
    if (bitmap == NULL || canvas == NULL || bitmap->dib == NULL || bitmap->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Validate the format
    // Note that this requires short-circuiting to work properly!
    if (bitmap->dib_size == 0 || bitmap->pixels_size == 0 || bitmap->dib->width <= 0 ||
        bitmap->dib->height >= 0 || bitmap->dib->planes != 1 || bitmap->stride == 0 ||
        (-bitmap->dib->height) * bitmap->dib->width > FLARE16X_BITMAP_MAX_PIXELS)
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);

    // Validate the width and height
    if (width == 0 || height == 0 || width + offset_x > bitmap->dib->width ||
        height + offset_y > (-bitmap->dib->height))
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_BITMAP);

    // Copy width and height
    canvas->width = width;
    canvas->height = height;

    // Next, determine the size of the canvas buffer and allocate it
    size_t canvas_size = width * height * sizeof(uint16_t);
    canvas->pixels = malloc(canvas_size);
    if (canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_BITMAP);

    // Now, check the format and copy and convert the pixel data to RGB565
    if (bitmap->dib->bit_count == 16 && bitmap->dib->compression == FLARE16X_BITMAP_COMPRESSION_BITFIELDS)
    {
        // RGB565 just requires a copy operation
        // For good measure, verify the masks
        if (bitmap->mask->mask_red != FLARE16X_BITMAP_MASK_RGB565_RED ||
            bitmap->mask->mask_green != FLARE16X_BITMAP_MASK_RGB565_GREEN ||
            bitmap->mask->mask_blue != FLARE16X_BITMAP_MASK_RGB565_BLUE)
        {
            free(canvas->pixels);
            canvas->pixels = NULL;
            return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
        }

        // Now, copy the pixels line by line, pixel by pixel without changing them
        int x, y;
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
                canvas->pixels[y * width + x] = bitmap->pixels565[(y + offset_y) *
                                                                  (bitmap->stride / sizeof(uint16_t)) + x + offset_x];
    } else if (bitmap->dib->bit_count == 24 && bitmap->dib->compression == FLARE16X_BITMAP_COMPRESSION_RGB)
    {
        // RGB888 requires reducing the resolution and remapping the image data
        // Now, process the pixels line by line, pixel by pixel
        int x, y;
        uint8_t r, g, b;
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                // Fetch the r, g and b components and truncate them to RGB565
                r = bitmap->pixels[(y + offset_y) * bitmap->stride + (x + offset_x) * 4] >> 3; // R5
                g = bitmap->pixels[(y + offset_y) * bitmap->stride + (x + offset_x) * 4 + 1] >> 2; // G6
                b = bitmap->pixels[(y + offset_y) * bitmap->stride + (x + offset_x) * 4 + 2] >> 3; // B5

                // Assemble the new RGB565 pixel
                canvas->pixels[y * width + x] = r << 11 | g << 5 | b;
            }
    } else if (bitmap->dib->bit_count == 32 && bitmap->dib->compression == FLARE16X_BITMAP_COMPRESSION_RGB)
    {
        // RGBA8888 requires reducing the resolution, discarding the alpha channel and remapping the image data
        // Now, process the pixels line by line, pixel by pixel
        int x, y;
        uint8_t r, g, b;
        uint32_t p;
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                // Fetch the whole pixel
                p = bitmap->pixels8888[(y + offset_y) * (bitmap->stride / sizeof(uint32_t)) + x + offset_x];

                // Mask the pixel, split it into it's components components and truncate them to RGB565
                r = (p & 0x00ff0000ul) >> (3 + (8*2)); // R5
                g = (p & 0x0000ff00ul) >> (2 + 8); // G6
                b = (p & 0x000000fful) >> 3; // B5

                // Assemble the new RGB565 pixel
                canvas->pixels[y * width + x] = (r & 0x1fu) << 11 | (g & 0x3fu) << 5 | (b & 0x1fu);
            }
    } else
    {
        free(canvas->pixels);
        canvas->pixels = NULL;
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // And, it's done!
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}

// Copies a region from a canvas buffer to a bitmap buffer
flare16x_error flare16x_bitmap_merge(flare16x_canvas* canvas, uint16_t offset_x, uint16_t offset_y,
        flare16x_bitmap* bitmap)
{
    // Make sure that there are no null pointers
    if (canvas == NULL || bitmap == NULL || canvas->pixels == NULL || bitmap->dib == NULL || bitmap->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_BITMAP);

    // Validate the format
    // Note that this requires short-circuiting to work properly!
    if (canvas->width == 0 || canvas->height == 0 || bitmap->dib_size == 0 || bitmap->pixels_size == 0 ||
        bitmap->dib->width <= 0 || bitmap->dib->height >= 0 || bitmap->dib->planes != 1 || bitmap->stride == 0 ||
        bitmap->dib->height * bitmap->dib->width > FLARE16X_BITMAP_MAX_PIXELS)
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);

    // Validate the width and height
    if (canvas->width + offset_x > bitmap->dib->width || canvas->height + offset_y > (-bitmap->dib->height))
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_BITMAP);

    // Now, check the format and copy and convert the pixel data from RGB565 to the bitmap format
    if (bitmap->dib->bit_count == 16 && bitmap->dib->compression == FLARE16X_BITMAP_COMPRESSION_BITFIELDS)
    {
        // RGB565 just requires a copy operation
        // For good measure, verify the masks
        if (bitmap->mask->mask_red != FLARE16X_BITMAP_MASK_RGB565_RED ||
            bitmap->mask->mask_green != FLARE16X_BITMAP_MASK_RGB565_GREEN ||
            bitmap->mask->mask_blue != FLARE16X_BITMAP_MASK_RGB565_BLUE)
        {
            free(canvas->pixels);
            canvas->pixels = NULL;
            return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
        }

        // Now, copy the pixels line by line, pixel by pixel without changing them
        int x, y;
        for (y = 0; y < canvas->height; y++)
            for (x = 0; x < canvas->width; x++)
                bitmap->pixels565[(y + offset_y) * (bitmap->stride / sizeof(uint16_t)) + x + offset_x] =
                        canvas->pixels[y * canvas->width + x];
    } else if (bitmap->dib->bit_count == 24 && bitmap->dib->compression == FLARE16X_BITMAP_COMPRESSION_RGB)
    {
        // RGB888 requires reducing the resolution and remapping the image data
        // Now, process the pixels line by line, pixel by pixel
        int x, y;
        uint16_t p;
        uint8_t r, g, b;
        for (y = 0; y < canvas->height; y++)
            for (x = 0; x < canvas->width; x++)
            {
                // Fetch the RGB565 pixel
                p = canvas->pixels[y * canvas->width + x];

                // Unpack it and separate the r, g and b components while expanding them to RGB888
                r = p & 0xf800u; // R5
                g = (p << 5) & 0xfc00u; // G6
                b = (p << 11) & 0xf800u; // B5

                // Store the components
                bitmap->pixels[(y + offset_x) * bitmap->stride + (x + offset_x) * 3] = r;
                bitmap->pixels[(y + offset_y) * bitmap->stride + (x + offset_x) * 3 + 1] = g;
                bitmap->pixels[(y + offset_y) * bitmap->stride + (x + offset_x) * 3 + 2] = b;
            }
    } else if (bitmap->dib->bit_count == 32 && bitmap->dib->compression == FLARE16X_BITMAP_COMPRESSION_RGB)
    {
        // RGBA8888 requires reducing the resolution, discarding the alpha channel and remapping the image data
        // Now, process the pixels line by line, pixel by pixel
        int x, y;
        uint16_t p;
        uint8_t r, g, b;
        for (y = offset_y; y < offset_y + canvas->height; y++)
            for (x = offset_x; x < offset_x + canvas->width; x++)
            {
                // Fetch the RGB565 pixel
                p = canvas->pixels[y * canvas->width + x];

                // Unpack it and separate the r, g and b components while expanding them to RGB888
                r = p & 0xf800u; // R5
                g = (p << 5) & 0xfc00u; // G6
                b = (p << 11) & 0xf800u; // B5

                // Store the components with a 0xff alpha
                bitmap->pixels8888[(y + offset_x) * (bitmap->stride / sizeof(uint16_t)) + (x + offset_x) * 4] =
                        0xff000000ul | (r << 16) | (g << 8) | b;
            }
    } else
    {
        free(canvas->pixels);
        canvas->pixels = NULL;
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_BITMAP);
    }

    // And, it's done!
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_BITMAP);
}
