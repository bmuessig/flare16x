# flare16x
Thermal image analysis software package compatible with FLIR TG165 and TG167

## Abstract
This software is an experimental (but well-working) framework to automatically analyze thermal images.
It can change the color palette of an image, OCR the spot reading, remove crosshairs and attempts to convert the thermal image pixels into temperatures.
The pixel to temperature conversion is very experimental and I can't recall how much of it was actually implemented.
Before FLIR discontinued the TG167, I was planning to make this a commercial product with a lot more features and a GUI, but this never happened.
That is the reason I am now releasing this no-longer maintained code in the hope that someone might find it useful.
Aside the custom OCR, there is a full bitmap library present that I painstakingly developed from ancient bitmap format documentation.

## Building
Either CMake or CLion can be used to build the code in this repository.

## Platforms
This program runs well on Linux, but it should also be compatible with macOS and Windows with no or minor modifications.
