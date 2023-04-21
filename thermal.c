//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// thermal.c: Functions for working with infrared image data
//

#include <string.h>

#include "error.h"
#include "canvas.h"
#include "locator.h"
#include "ocr.h"
#include "palettes.h"

#include "thermal.h"

// Initializes the thermal context using a locator struct and calculates the crosshair mask
// Will destroy the locator struct supplied by moving its pointers to the thermal context
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_thermal_create(flare16x_locator* locator, flare16x_thermal* thermal)
{
    // Make sure the locator and thermal structs are not null
    if (locator == NULL || thermal == NULL || locator->ir_canvas == NULL || locator->text_canvas == NULL ||
        locator->ir_canvas->pixels == NULL || locator->text_canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Validate the rest of the data starting with the basic data
    if (locator->ir_canvas->width == 0 || locator->ir_canvas->height == 0 ||
        locator->text_canvas->width == 0 || locator->text_canvas->height == 0)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // And followed by the model
    switch (locator->device_model)
    {
        case FLARE16X_LOCATOR_MODEL_TG165:
        case FLARE16X_LOCATOR_MODEL_TG167:
            if (locator->crosshair_width == 0 || locator->aperture_width == 0 ||
                locator->crosshair_height == 0 || locator->aperture_height == 0)
                return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);
            break;

        case FLARE16X_LOCATOR_MODEL_UNKNOWN:
            break;

        default:
            return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);
    }

    // Zero the thermal context first
    memset(thermal, 0, sizeof(flare16x_thermal));

    // Copy the regular values first
    thermal->mask.width = locator->ir_canvas->width;
    thermal->mask.height = locator->ir_canvas->height;
    thermal->device_model = locator->device_model;
    // Note, that the locator calls this area aperture, while the thermal struct references it as aperture
    // Both terms can be used interchangeably here, as the aperture refers to the center spot reading of the camera
    thermal->spot_width = locator->aperture_width;
    thermal->spot_height = locator->aperture_height;
    thermal->spot_x = locator->aperture_x;
    thermal->spot_y = locator->aperture_y;

    // Attempt to allocate memory for the mask
    thermal->mask.pixels = malloc(thermal->mask.width * thermal->mask.height);
    if (thermal->mask.pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_THERMAL);
    memset(thermal->mask.pixels, 0, thermal->mask.width * thermal->mask.height);

    // Next, generate but not verify the mask
    int x, y;
    for (y = 0; y < locator->ir_canvas->height; y++)
        for (x = 0; x < locator->ir_canvas->width; x++)
            thermal->mask.pixels[y * thermal->mask.width + x] = flare16x_locator_detect(locator, x, y);

    // Move the pointers over next
    thermal->visible_image = locator->ir_canvas;
    thermal->text_image = locator->text_canvas;
    locator->ir_canvas = NULL;
    locator->text_canvas = NULL;

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}

// Runs OCR on the image and attempts to parse the OSD text
// If this function returns no error, it is safe to destroy the text image
flare16x_error flare16x_thermal_ocr(flare16x_thermal* thermal)
{
    // Make sure the thermal struct is not null
    if (thermal == NULL || thermal->text_image == NULL || thermal->text_image->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Verify the text image dimensions
    if (thermal->text_image->width != FLARE16X_LOCATOR_TEXT_WIDTH ||
        thermal->text_image->height != FLARE16X_LOCATOR_TEXT_HEIGHT)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Allocate space for the OCR strings
    char temperature_string[FLARE16X_LOCATOR_TEMPERATURE_DIGITS + 1];
    char emissivity_string[FLARE16X_LOCATOR_EMISSIVITY_DIGITS + 1];

    // Clear the strings
    memset(temperature_string, 0, FLARE16X_LOCATOR_TEMPERATURE_DIGITS + 1);
    memset(emissivity_string, 0, FLARE16X_LOCATOR_EMISSIVITY_DIGITS + 1);

    // Run the OCR on the temperature first
    flare16x_error error;
    error = flare16x_ocr_large_string(FLARE16X_LOCATOR_TEMPERATURE_OFFSET_X, FLARE16X_LOCATOR_TEMPERATURE_OFFSET_Y,
            FLARE16X_LOCATOR_TEMPERATURE_PITCH, FLARE16X_LOCATOR_TEMPERATURE_DIGITS, 0,
            thermal->text_image, temperature_string);
    if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE, FLARE16X_ERROR_SOURCE_THERMAL),
                error);

    // Followed by an OCR on the emissivity
    error = flare16x_ocr_small_string(FLARE16X_LOCATOR_EMISSIVITY_OFFSET_X, FLARE16X_LOCATOR_EMISSIVITY_OFFSET_Y,
            FLARE16X_LOCATOR_EMISSIVITY_PITCH, FLARE16X_LOCATOR_EMISSIVITY_DIGITS, 0,
            thermal->text_image, emissivity_string);
    if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE, FLARE16X_ERROR_SOURCE_THERMAL),
               error);

    // Next, read the temperature
    char temperature_unit = 0;
    int temperature, temperature_integer = 0, temperature_fractional = 0;
    if (sscanf(temperature_string, "%d.%1u%c", &temperature_integer, &temperature_fractional,
            &temperature_unit) != 3)
        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Check, if the temperature is negative and negate the fractional
    if (temperature_integer < 0)
        temperature_fractional = -temperature_fractional;

    // Check, assemble and optionally convert the temperature
    switch (temperature_unit)
    {
        // Degrees celsius
        case 'C':
            // Conversion is simple here
            thermal->temperature_spot = temperature_integer * 10 + temperature_fractional;
            break;

        // Degrees fahrenheit (convert it to celsius)
        case 'F':
            // The buffering of the temperature is desired, as the maximum temperature that can be stored in 16 bits
            // is 687,36F which results in about 363,89C, which is at the edge of the device's resolution
            temperature = ((temperature_integer - 32) * 10 + temperature_fractional) * 5;
            // Round up, if the remainder is larger than 4
            if (temperature % 9 >= 5)
                temperature += 8;
            thermal->temperature_spot = temperature / 9;
            break;

        // Unknown unit
        default:
            return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_THERMAL);
    }

    // Next, prepare reading the emissivity
    char emissivity_prefix[3];

    // Clear the prefix
    memset(emissivity_prefix, 0, 3);

    // Read the values
    if (sscanf(emissivity_string, "%2s0.%2hhu", emissivity_prefix, &thermal->emissivity) != 2)
        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Verify the prefix and value
    if (strcmp(emissivity_prefix, "E:") != 0 || thermal->emissivity == 0 || thermal->emissivity > 99)
        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // That's it, return success
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}

// Runs the palette analysis and converts the visible light image into relative IR data
// This step may take a while, as it calculates every pixel at least twice
// If this function returns no error, it is safe to destroy the thermal image
flare16x_error flare16x_thermal_process(flare16x_thermal* thermal, uint8_t interpolation_mode,
        uint8_t quantification_mode)
{
    // Make sure the thermal struct is not null
    if (thermal == NULL || thermal->visible_image == NULL || thermal->visible_image->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Also make sure the interpolation and quantification modes are within range
    if (interpolation_mode >= FLARE16X_THERMAL_INTERPOLATION_COUNT ||
        quantification_mode >= FLARE16X_THERMAL_QUANTIFICATION_COUNT)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Verify the IR image dimensions
    if (thermal->visible_image->width < 1 || thermal->visible_image->height < 1)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Check, if there is an old thermal image that has to be destroyed first
    if (thermal->thermal_image != NULL)
        return flare16x_error_make(FLARE16X_ERROR_LEAK, FLARE16X_ERROR_SOURCE_THERMAL);

    // Initially, perform the palette analysis, which may take a moment and might fail
    uint8_t palette_index;
    flare16x_error error = flare16x_palettes_determine(thermal->visible_image, FLARE16X_PALETTES_IGNORE_ERRORS,
            &palette_index);
    if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE, FLARE16X_ERROR_SOURCE_THERMAL),
                                   error);

    // Allocate memory for the new relative infrared image struct
    thermal->thermal_image = malloc(sizeof(flare16x_thermal_image));
    if (thermal->thermal_image == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_THERMAL);

    // Clear the new memory
    memset(thermal->thermal_image, 0, sizeof(flare16x_thermal_image));

    // Copy width, height and the quantification mode
    thermal->thermal_image->width = thermal->visible_image->width;
    thermal->thermal_image->height = thermal->visible_image->height;
    thermal->thermal_image->mode = quantification_mode;

    // Allocate memory for the new relative infrared image data
    thermal->thermal_image->points = malloc(thermal->thermal_image->width * thermal->thermal_image->height *
            sizeof(flare16x_thermal_point));
    if (thermal->thermal_image->points == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_THERMAL);

    // To check, if a pixel needs to be replaced, the locator detect function is used
    // Apart from that pixels that have been interpolated have the width of 1 with the intermediate value

    // This counter counts any skipped points
    uint32_t skipped_points = 0;

    // For the min, max and med, keep the respective markers
    uint32_t value_med_sum = 0, value_med_count = 0;
    uint8_t value_min = 0xff, value_max = 0;

    // To accelerate the second pass, store the first line with border data
    int start_y = -1;

    // Allocate a palette cache (init can't fail, as palette_cache can't be a null pointer)
    flare16x_palette_cache palette_cache;
    flare16x_palettes_cache_init(&palette_cache);

    // Perform the first iteration over the input pixel data and process all non-crosshair pixels and verify the mask
    int x, y;
    for (y = 0; y < thermal->visible_image->height; y++)
        for (x = 0; x < thermal->visible_image->width; x++)
        {
            // Fetch the current color and mask pixel
            uint16_t color = flare16x_canvas_raw(x, y, thermal->visible_image);
            uint8_t mask = thermal->mask.pixels[y * thermal->mask.width + x];

            // Allocate the palette entry pointer
            const flare16x_palette_entry *palette_entry;

            // Check the type of the mask pixel
            switch (mask)
            {
                case FLARE16X_LOCATOR_DETECT_IMAGE:
                    // For regular image pixels, just calculate the palette entry (this may take a moment)
                    error = flare16x_palettes_find_color(color, palette_index, &palette_cache, &palette_entry);

                    // Check, if the point could not be found
                    if (flare16x_error_reason(error) == FLARE16X_ERROR_IMAGE)
                    {
                        // The point has an invalid color
                        // Mark it as invalid in the mask
                        mask = FLARE16X_LOCATOR_DETECT_INVALID;
                        thermal->mask.pixels[y * thermal->mask.width + x] = mask;

                        // Check, if this is the first line with invalid data and store the current one, if true
                        if (start_y < 0)
                            start_y = y;

                        // Finally, count this pixel as skipped
                        skipped_points++;

                        // And move on to the next pixel
                        break;
                    }
                    else if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
                    {
                        // On any other error, fail
                        flare16x_thermal_image_destroy(thermal->thermal_image);
                        free(thermal->thermal_image);
                        thermal->thermal_image = NULL;
                        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE,
                                FLARE16X_ERROR_SOURCE_THERMAL), error);
                    }

                    // Verify the width (yes, this is redundant for the exact mode but that's not a big issue)
                    if (palette_entry->width < 1)
                    {
                        flare16x_thermal_image_destroy(thermal->thermal_image);
                        free(thermal->thermal_image);
                        thermal->thermal_image = NULL;
                        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_RANGE);
                    }

                    // Then, do the statistical analysis
                    value_med_sum += palette_entry->base;
                    value_med_count++;
                    if (value_max < palette_entry->base)
                        value_max = palette_entry->base;
                    if (value_min > palette_entry->base)
                        value_min = palette_entry->base;

                    // Now, quantify the palette entry
                    switch (quantification_mode)
                    {
                        case FLARE16X_THERMAL_QUANTIFICATION_EXACT:
                            // Make sure that every value has really only width 1
                            if (palette_entry->width != 1)
                            {
                                flare16x_thermal_image_destroy(thermal->thermal_image);
                                free(thermal->thermal_image);
                                thermal->thermal_image = NULL;
                                return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_THERMAL);
                            }
                            // Fallthrough to the floor method, which is otherwise identical

                        case FLARE16X_THERMAL_QUANTIFICATION_FLOOR:
                            // Set the lowest thermal value and certainty
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value =
                                    palette_entry->base;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty =
                                    palette_entry->width;

                            // And continue
                            break;

                        case FLARE16X_THERMAL_QUANTIFICATION_CEILING:
                            // Set the highest thermal value and certainty
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value =
                                    (palette_entry->width - 1) + palette_entry->base;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty =
                                    palette_entry->width;

                            // And continue
                            break;

                        case FLARE16X_THERMAL_QUANTIFICATION_MEDIAN_LOW:
                            // Set the median thermal value (rounding down) and certainty
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value =
                                    ((palette_entry->width - 1) / 2) + palette_entry->base;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty =
                                    palette_entry->width;

                            // And continue
                            break;

                        case FLARE16X_THERMAL_QUANTIFICATION_MEDIAN_HIGH:
                            // Set the median thermal value (rounding up) and certainty
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value =
                                    (palette_entry->width / 2) + palette_entry->base;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty =
                                    palette_entry->width;

                            // And continue
                            break;

                        default:
                            // Assert: This code cannot be reached
                            flare16x_thermal_image_destroy(thermal->thermal_image);
                            free(thermal->thermal_image);
                            thermal->thermal_image = NULL;
                            return flare16x_error_make(FLARE16X_ERROR_ASSERT, FLARE16X_ERROR_RANGE);
                    }
                    // And continue
                    break;

                case FLARE16X_LOCATOR_DETECT_CROSSHAIR:
                    // Check, if this is the first line with crosshair data and store the current one, if true
                    if (start_y < 0)
                        start_y = y;

                    // Check, if the zero interpolation mode is used
                    if (interpolation_mode == FLARE16X_THERMAL_INTERPOLATION_ZERO)
                    {
                        // Set the IR value to zero with width 1, as desired
                        flare16x_thermal_image_raw(x, y, thermal->thermal_image).value = 0;
                        flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty = 1;
                        break;
                    }

                    // Otherwise count the point as skipped
                    skipped_points++;
                    break;

                default:
                    // Something went wrong generating the mask
                    flare16x_thermal_image_destroy(thermal->thermal_image);
                    free(thermal->thermal_image);
                    thermal->thermal_image = NULL;
                    return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);
            }
        }

    // Assert: Min <= Max
    if (value_min > value_max)
    {
        flare16x_thermal_image_destroy(thermal->thermal_image);
        free(thermal->thermal_image);
        thermal->thermal_image = NULL;
        return flare16x_error_make(FLARE16X_ERROR_ASSERT, FLARE16X_ERROR_SOURCE_THERMAL);
    }

    // Now, check if a second pass is really necessary
    // It will be skipped for images without any masked points and for ones with zero interpolation
    if (skipped_points == 0)
        return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Assert: If the number of skipped points is non-zero, a start line must have been found
    // Assert: The image contains at least one point
    if (start_y < 0 || value_med_count < 1)
    {
        flare16x_thermal_image_destroy(thermal->thermal_image);
        free(thermal->thermal_image);
        thermal->thermal_image = NULL;
        return flare16x_error_make(FLARE16X_ERROR_ASSERT, FLARE16X_ERROR_SOURCE_THERMAL);
    }

    // Now, calculate the median value
    uint8_t value_med = value_med_sum / value_med_count;

    // As it is necessary, perform a partial second pass
    for (y = start_y; y < thermal->visible_image->height; y++)
        for (x = 0; x < thermal->visible_image->width; x++)
        {
            // Fetch the current mask pixel
            uint8_t mask = thermal->mask.pixels[y * thermal->mask.width + x];

            // Clear the existing median variables to use them for the cross average mode
            value_med_sum = 0;
            value_med_count = 0;

            // Allocate the x and y variables for square, as well as the weight scale
            int offset_x, offset_y, weight_scale = 1;
            if (interpolation_mode == FLARE16X_THERMAL_INTERPOLATION_SQUARE_WEIGHT)
                weight_scale = 4;

            // Check the type of the current mask pixel
            switch (mask)
            {
                case FLARE16X_LOCATOR_DETECT_IMAGE:
                    // Ignore regular image pixels, as they have already been converted in the previous pass
                    break;

                case FLARE16X_LOCATOR_DETECT_INVALID:
                    // For invalid pixels, clear them from the mask immediately
                    mask = FLARE16X_LOCATOR_DETECT_IMAGE;
                    thermal->mask.pixels[y * thermal->mask.width + x] = mask;

                    // Then, fall through to the regular crosshair routine

                case FLARE16X_LOCATOR_DETECT_CROSSHAIR:
                    // Decrement the skipped pixel counter
                    skipped_points--;

                    // Crosshair pixels have to be replaced with interpolated or fixed data
                    switch (interpolation_mode)
                    {
                        case FLARE16X_THERMAL_INTERPOLATION_MIN:
                            // Simply replace the unknown points with the minimum value observed in the image
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value = value_min;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty = 1;
                            break;

                        case FLARE16X_THERMAL_INTERPOLATION_MAX:
                            // Simply replace the unknown points with the maximum value observed in the image
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value = value_max;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty = 1;
                            break;

                        case FLARE16X_THERMAL_INTERPOLATION_MED:
                            // Simply replace the unknown points with the average value observed in the image
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value = value_med;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty = 1;
                            break;

                        case FLARE16X_THERMAL_INTERPOLATION_SQUARE_LARGE:
                            // This mode calculates the average value of the surrounding square with size 6
                            // Check, if each point is within bounds and fetch its value
                            for (offset_y = -6; offset_y <= 6; offset_y++)
                                for (offset_x = -6; offset_x <= 6; offset_x++)
                                    if (flare16x_thermal_valid(offset_x, offset_y, x, y, thermal))
                                    {
                                        value_med_sum += flare16x_thermal_image_raw(x + offset_x, y + offset_y,
                                                thermal->thermal_image).value;
                                        value_med_count++;
                                    }

                            // Fall through
                            // This will add the center square multiple times making it more important

                        case FLARE16X_THERMAL_INTERPOLATION_SQUARE_WEIGHT:
                            // This mode calculates the average value of the surrounding square with size 1 and 2
                            // Check, if each point is within bounds and fetch its value
                            for (offset_y = -1; offset_y <= 1; offset_y++)
                                for (offset_x = -1; offset_x <= 1; offset_x++)
                                    if (flare16x_thermal_valid(offset_x, offset_y, x, y, thermal))
                                    {
                                        value_med_sum += flare16x_thermal_image_raw(x + offset_x, y + offset_y,
                                                thermal->thermal_image).value * weight_scale;
                                        value_med_count += weight_scale;
                                    }

                        case FLARE16X_THERMAL_INTERPOLATION_SQUARE_SMALL:
                            // This mode calculates the average value of the surrounding square with size 2
                            // Check, if each point is within bounds and fetch its value
                            for (offset_y = -2; offset_y <= 2; offset_y++)
                                for (offset_x = -2; offset_x <= 2; offset_x++)
                                if (flare16x_thermal_valid(offset_x, offset_y, x, y, thermal))
                                {
                                    value_med_sum += flare16x_thermal_image_raw(x + offset_x, y + offset_y,
                                                                                thermal->thermal_image).value;
                                    value_med_count++;
                                }

                            // Verify, that at least one point was found
                            if (value_med_count < 1)
                            {
                                flare16x_thermal_image_destroy(thermal->thermal_image);
                                free(thermal->thermal_image);
                                thermal->thermal_image = NULL;
                                return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_THERMAL);
                            }

                            // After all points have been added and the count increased, calculate the median
                            value_med = value_med_sum / value_med_count;

                            // Finally, set the current value to this median and a width of 1
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).value = value_med;
                            flare16x_thermal_image_raw(x, y, thermal->thermal_image).uncertainty = 1;

                            // And continue
                            break;

                        default:
                            // Assert: This can't be reached as any other mode should have been dealt with earlier
                            flare16x_thermal_image_destroy(thermal->thermal_image);
                            free(thermal->thermal_image);
                            thermal->thermal_image = NULL;
                            return flare16x_error_make(FLARE16X_ERROR_ASSERT, FLARE16X_ERROR_SOURCE_THERMAL);
                    }

                    // And continue
                    break;

                default:
                    // Assert: This can't be reached as any error should have been caught in the first pass
                    flare16x_thermal_image_destroy(thermal->thermal_image);
                    free(thermal->thermal_image);
                    thermal->thermal_image = NULL;
                    return flare16x_error_make(FLARE16X_ERROR_ASSERT, FLARE16X_ERROR_SOURCE_THERMAL);
            }
        }

    // Assert: The skipped pixel counter is now zero
    if (skipped_points != 0)
    {
        flare16x_thermal_image_destroy(thermal->thermal_image);
        free(thermal->thermal_image);
        thermal->thermal_image = NULL;
        return flare16x_error_make(FLARE16X_ERROR_ASSERT, FLARE16X_ERROR_SOURCE_THERMAL);
    }

    // Success!
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}

// Converts the relative thermal image into a visible image using the supplied palette
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_thermal_export(flare16x_thermal* thermal, uint8_t palette_index, flare16x_canvas* canvas)
{
    // Make sure the thermal struct and canvas are not null
    if (thermal == NULL || thermal->thermal_image == NULL || thermal->thermal_image->points == NULL || canvas == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Verify the thermal image dimensions
    if (thermal->thermal_image->width < 1 || thermal->thermal_image->height < 1)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Validate the palette
    if (flare16x_palettes_get_length(palette_index) < 1)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Initialize the canvas
    flare16x_error error;
    error = flare16x_canvas_create(thermal->thermal_image->width, thermal->thermal_image->height, canvas);
    if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE, FLARE16X_ERROR_SOURCE_THERMAL),
                error);

    // Allocate a palette cache (init can't fail, as palette_cache can't be a null pointer)
    flare16x_palette_cache palette_cache;
    flare16x_palettes_cache_init(&palette_cache);

    // Next, iterate over all points in the image, convert them to colors and store them to the canvas
    int x, y;
    for (y = 0; y < thermal->thermal_image->height; y++)
        for (x = 0; x < thermal->thermal_image->width; x++)
        {
            // Fetch the value first
            uint8_t value = flare16x_thermal_image_raw(x, y, thermal->thermal_image).value;

            // Next, attempt to find the correct palette item
            const flare16x_palette_entry* palette_entry;
            error = flare16x_palettes_find_value(value, palette_index, &palette_cache, &palette_entry);
            if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
            {
                flare16x_canvas_destroy(canvas);
                return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE,
                                                               FLARE16X_ERROR_SOURCE_THERMAL), error);
            }

            // Verify the width of the entry
            if (palette_entry->width < 1)
            {
                flare16x_canvas_destroy(canvas);
                return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_RANGE);
            }

            // Now, store the color of the entry at the current position in the canvas
            flare16x_canvas_raw(x, y, canvas) = palette_entry->color;
        }

    // Success :-)
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}

// Adds a custom colored crosshair onto an exported thermal image using the mask
flare16x_error flare16x_thermal_crosshair(uint16_t crosshair_border, uint16_t crosshair_fill,
        flare16x_thermal* thermal, flare16x_canvas* canvas)
{
    // Make sure the thermal struct and canvas are not null
    if (thermal == NULL || canvas == NULL || thermal->mask.pixels == NULL || canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Verify the thermal image dimensions
    if (thermal->mask.width != canvas->width || thermal->mask.height != canvas->height ||
        canvas->width < 1 || canvas->height < 1)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // This keeps the cross state machine state
    int cross_state = FLARE16X_THERMAL_MASK_NONE, cross_length = 0;

    // Loop over the mask
    int x, y;
    for (y = 0; y < thermal->mask.height; y++, cross_state = FLARE16X_THERMAL_MASK_NONE, cross_length = 0)
        for(x = 0; x < thermal->mask.width; x++)
        {
            // Fetch the mask pixel
            uint8_t mask = thermal->mask.pixels[y * thermal->mask.width + x];

            // And decide what to do
            switch (mask)
            {
                case FLARE16X_LOCATOR_DETECT_IMAGE:
                    switch (cross_state)
                    {
                        case FLARE16X_THERMAL_MASK_FILL:
                            // The current pixel was in fill state, therefore make it a border pixel and reset
                            // Check, if the border length permits going back and making it a
                            if (cross_length > 1)
                                flare16x_canvas_raw(x - 1, y, canvas) = crosshair_border;
                            // Fall through and reset the state

                        case FLARE16X_THERMAL_MASK_BORDER:
                            // Reset the state and length
                            cross_state = FLARE16X_THERMAL_MASK_NONE;
                            cross_length = 0;
                            break;

                        default:
                            // Do nothing
                            break;
                    }
                    break;

                case FLARE16X_LOCATOR_DETECT_CROSSHAIR:
                    switch (cross_state)
                    {
                        case FLARE16X_THERMAL_MASK_BORDER:
                            // Set the state to fill
                            cross_state = FLARE16X_THERMAL_MASK_FILL;

                            // And fall through to have this pixel filled

                        case FLARE16X_THERMAL_MASK_FILL:
                            // The current pixel is in fill state, so fill it
                            flare16x_canvas_raw(x, y, canvas) = crosshair_fill;
                            cross_length++;
                            break;

                        default:
                            // The current pixel is the first border pixel
                            flare16x_canvas_raw(x, y, canvas) = crosshair_border;

                            // Also set the state to border
                            cross_state = FLARE16X_THERMAL_MASK_BORDER;

                            // And increment the length
                            cross_length++;
                            break;
                    }
                    break;

                default:
                    // There was an issue with the mask
                    return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);
            }
        }

    // Run the second, vertical, pass
    cross_state = FLARE16X_THERMAL_MASK_NONE;
    cross_length = 0;
    for(x = 0; x < thermal->mask.width; x++, cross_state = FLARE16X_THERMAL_MASK_NONE, cross_length = 0)
        for (y = 0; y < thermal->mask.height; y++)
        {
            // Fetch the mask pixel
            uint8_t mask = thermal->mask.pixels[y * thermal->mask.width + x];

            // And decide what to do
            switch (mask)
            {
                case FLARE16X_LOCATOR_DETECT_IMAGE:
                    switch (cross_state)
                    {
                        case FLARE16X_THERMAL_MASK_FILL:
                            // The current pixel was in fill state, therefore make it a border pixel and reset
                            // Check, if the border length permits going back and making it a
                            if (cross_length > 1)
                                flare16x_canvas_raw(x, y - 1, canvas) = crosshair_border;
                            // Fall through and reset the state

                        case FLARE16X_THERMAL_MASK_BORDER:
                            // Reset the state and length
                            cross_state = FLARE16X_THERMAL_MASK_NONE;
                            cross_length = 0;
                            break;

                        default:
                            // Do nothing
                            break;
                    }
                    break;

                case FLARE16X_LOCATOR_DETECT_CROSSHAIR:
                    switch (cross_state)
                    {
                        case FLARE16X_THERMAL_MASK_BORDER:
                            // Set the state to fill
                            cross_state = FLARE16X_THERMAL_MASK_FILL;

                            // And fall through to have this pixel counted

                        case FLARE16X_THERMAL_MASK_FILL:
                            // The current pixel is in fill state, but as this is the second run, just count it
                            cross_length++;
                            break;

                        default:
                            // The current pixel is the first border pixel
                            flare16x_canvas_raw(x, y, canvas) = crosshair_border;

                            // Also set the state to border
                            cross_state = FLARE16X_THERMAL_MASK_BORDER;

                            // And increment the length
                            cross_length++;
                            break;
                    }
                    break;

                default:
                    // There was an issue with the mask
                    return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);
            }
        }

    // It succeeded
    return flare16x_error_make(FLARE16X_THERMAL_MASK_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}

// Frees all resources used by a thermal struct including the canvas data
flare16x_error flare16x_thermal_destroy(flare16x_thermal* thermal)
{
    // Make sure the thermal struct is not null
    if (thermal == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Check, if there is a thermal image and free it
    if (thermal->thermal_image != NULL)
    {
        flare16x_thermal_image_destroy(thermal->thermal_image);
        free(thermal->thermal_image);
        thermal->thermal_image = NULL;
    }

    // Check, if there is a text canvas and free it
    if (thermal->text_image != NULL)
    {
        flare16x_canvas_destroy(thermal->text_image);
        free(thermal->text_image);
        thermal->text_image = NULL;
    }

    // Check, if there is an IR canvas and free it
    if (thermal->visible_image != NULL)
    {
        flare16x_canvas_destroy(thermal->visible_image);
        free(thermal->visible_image);
        thermal->visible_image = NULL;
    }

    // Free the mask
    free(thermal->mask.pixels);

    // Finally, zero the struct
    memset(thermal, 0, sizeof(flare16x_thermal));

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}

// Allocates the resources for a new thermal image with exact quantification
flare16x_error flare16x_thermal_image_init(uint16_t width, uint16_t height, flare16x_thermal_image* image)
{
    // Make sure the thermal image struct is not null
    if (image == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Verify the parameters
    if (width < 1 || height < 1)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_THERMAL);

    // Clear the target structure
    memset(image, 0, sizeof(flare16x_thermal_image));

    // Allocate the required memory buffer
    image->points = malloc(width * height * sizeof(flare16x_thermal_point));
    if (image->points == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_THERMAL);

    // Store the parameters
    image->width = width;
    image->height = height;
    image->mode = FLARE16X_THERMAL_QUANTIFICATION_EXACT;

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}

// Frees the resources used by a thermal image
flare16x_error flare16x_thermal_image_destroy(flare16x_thermal_image* image)
{
    // Make sure the thermal image struct is not null
    if (image == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_THERMAL);

    // Free the points
    free(image->points);

    // Clear the structure
    memset(image, 0, sizeof(flare16x_thermal_image));

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_THERMAL);
}
