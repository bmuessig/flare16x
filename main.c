//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// main.c: Application entry point
//

#include <stdio.h>
#include "error.h"
#include "bitmap.h"
#include "canvas.h"
#include "ocr.h"
#include "locator.h"
#include "palettes.h"
#include "thermal.h"

int main() {
    flare16x_error test = FLARE16X_ERROR_NONE;
    printf("Error: %s\n", flare16x_error_string(test));
    flare16x_error_push(FLARE16X_ERROR_IO, &test);
    flare16x_error_push(FLARE16X_ERROR_ASSERT, &test);
    printf("Error: %s\n", flare16x_error_string(test));
    printf("Error: %s\n", flare16x_error_string(flare16x_error_first(test)));

    flare16x_bitmap bmp;
    //FILE* sample_img = fopen("/home/benedikt/Desktop/7/Image/FLIR00087.bmp", "r");
    //FILE* sample_img = fopen("/home/benedikt/Desktop/7/Image/FLIR00086.bmp", "r");
    //FILE* sample_img = fopen("/home/benedikt/Desktop/7/Image/FLIR00080.bmp", "r");
    //FILE* sample_img = fopen("/home/benedikt/img.bmp", "r");
    FILE* sample_img = fopen("/home/benedikt/tg165_cs.bmp", "r");
    FILE* out_file = fopen("/home/benedikt/Desktop/test.bmp", "w");
    FILE* out_file2 = fopen("/home/benedikt/Desktop/test2.bmp", "w");
    if (sample_img == NULL || out_file == NULL || out_file2 == NULL)
        return 1;

    printf("Loading: %s\n", flare16x_error_string(flare16x_bitmap_load(sample_img, &bmp)));

    flare16x_locator locator;
    printf("Locator init: %s\n", flare16x_error_string(flare16x_locator_create(&bmp, &locator)));
    printf("Locator run: %s\n", flare16x_error_string(flare16x_locator_process(&locator)));

    uint8_t palette;
    printf("Palette det: %s\n", flare16x_error_string(flare16x_palettes_determine(locator.ir_canvas,
            FLARE16X_PALETTES_IGNORE_ERRORS, &palette)));
    printf("Current palette: %d\n", palette);

    flare16x_canvas canvas;
    printf("Editing: %s\n", flare16x_error_string(
            flare16x_bitmap_edit(&bmp, 0, 0, 174, 40, &canvas)));

    char ocr1[10];
    char ocr2[10];
    printf("OCR: %s: %s\n", flare16x_error_string(
            flare16x_ocr_large_string(2, 1, 0, 6, 1, &canvas, ocr1)), ocr1);
    printf("OCR: %s: %s\n", flare16x_error_string(
            flare16x_ocr_small_string(112, 4, 0, 6, 1, &canvas, ocr2)), ocr2);

    flare16x_bitmap bmp2;
    flare16x_bitmap_create16(locator.ir_canvas->width, locator.ir_canvas->height, &bmp2);
    flare16x_bitmap_merge(locator.ir_canvas, 0, 0, &bmp2);

    printf("Writing: %s\n", flare16x_error_string(
            flare16x_bitmap_store(&bmp2, out_file)));

    flare16x_thermal thermal;
    flare16x_canvas canvas2;
    printf("Thermal init: %s\n", flare16x_error_string(flare16x_thermal_create(&locator, &thermal)));
    printf("Thermal ocr: %s\n", flare16x_error_string(flare16x_thermal_ocr(&thermal)));
    printf("Temperature: %d °C*10, Emissivity: 0.%d\n", thermal.temperature_spot, thermal.emissivity);
    printf("Thermal process: %s\n", flare16x_error_string(flare16x_thermal_process(&thermal,
            FLARE16X_THERMAL_INTERPOLATION_SQUARE_LARGE,
            FLARE16X_THERMAL_QUANTIFICATION_MEDIAN_LOW)));
    printf("Thermal export: %s\n", flare16x_error_string(flare16x_thermal_export(&thermal,
            FLARE16X_PALETTES_IRON, &canvas2)));
    printf("Thermal crosshair: %s\n", flare16x_error_string(flare16x_thermal_crosshair(
            FLARE16X_LOCATOR_CROSSHAIR_BORDER, FLARE16X_LOCATOR_CROSSHAIR_FILL,
            &thermal, &canvas2)));
    printf("Thermal destroy: %s\n", flare16x_error_string(flare16x_thermal_destroy(&thermal)));

    flare16x_bitmap bmp3;
    flare16x_bitmap_create16(canvas2.width, canvas2.height, &bmp3);
    flare16x_bitmap_merge(&canvas2, 0, 0, &bmp3);
    printf("Writing: %s\n", flare16x_error_string(
            flare16x_bitmap_store(&bmp3, out_file2)));
    flare16x_canvas_destroy(&canvas2);
    fclose(out_file2);
    flare16x_bitmap_destroy(&bmp3);

    fclose(sample_img);
    fclose(out_file);

    flare16x_locator_destroy(&locator);
    flare16x_canvas_destroy(&canvas);
    flare16x_bitmap_destroy(&bmp);
    flare16x_bitmap_destroy(&bmp2);

    printf("Hello, World!\n");
    return 0;
}