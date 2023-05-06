// Code repurposed to load bmp to framebuffer. Original header below:

// Author: Christian Vallentin <vallentin.source@gmail.com>
// Website: http://vallentin.dev
// Repository: https://github.com/vallentin/LoadBMP
//
// Date Created: January 03, 2014
// Last Modified: August 13, 2016
//
// Version: 1.1.0

// Copyright (c) 2014-2016 Christian Vallentin <vallentin.source@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.

// Include loadbmp.hpp as following
// to create the implementation file.
//
// #define LOADBMP_IMPLEMENTATION
// #include "loadbmp.hpp"

#ifndef LOADBMP_H
#define LOADBMP_H

// Errors
#define LOADBMP_NO_ERROR 0

#define LOADBMP_OUT_OF_MEMORY 1

#define LOADBMP_FILE_NOT_FOUND 2
#define LOADBMP_FILE_OPERATION 3

#define LOADBMP_INVALID_FILE_FORMAT 4

#define LOADBMP_INVALID_SIGNATURE 5
#define LOADBMP_INVALID_BITS_PER_PIXEL 6

#ifdef LOADBMP_IMPLEMENTATION
#define LOADBMP_API
#else
#define LOADBMP_API extern
#endif

// LoadBMP uses raw buffers and support RGB.
// The order is RGBRGBRGB..., from top left
// to bottom right, without any padding.

#include <string>

LOADBMP_API unsigned int loadbmp_to_framebuffer(std::string filename, unsigned char *framebuffer, unsigned int width, unsigned int height, unsigned int stride);

#ifdef LOADBMP_IMPLEMENTATION

#include <stdio.h>  /* fopen(), fread(), fclose() */
#include <string.h> /* memset()*/

#include <string>
#include <vector>

LOADBMP_API unsigned int loadbmp_to_framebuffer(std::string filename, unsigned char *framebuffer, unsigned int width, unsigned int height,
                                                unsigned int stride) {
    FILE *f = fopen(filename.c_str(), "rb");

    if (!f) return LOADBMP_FILE_NOT_FOUND;

    unsigned char bmp_file_header[14];
    unsigned char bmp_info_header[40];

    thread_local std::vector<char> bmp_img;

    unsigned int w, h;

    unsigned int x, y, i, j;

    memset(bmp_file_header, 0, sizeof(bmp_file_header));
    memset(bmp_info_header, 0, sizeof(bmp_info_header));

    if (fread(bmp_file_header, sizeof(bmp_file_header), 1, f) == 0) {
        fclose(f);
        return LOADBMP_INVALID_FILE_FORMAT;
    }

    if (fread(bmp_info_header, sizeof(bmp_info_header), 1, f) == 0) {
        fclose(f);
        return LOADBMP_INVALID_FILE_FORMAT;
    }

    if ((bmp_file_header[0] != 'B') || (bmp_file_header[1] != 'M')) {
        fclose(f);
        return LOADBMP_INVALID_SIGNATURE;
    }

    if ((bmp_info_header[14] != 24) && (bmp_info_header[14] != 32)) {
        fclose(f);
        return LOADBMP_INVALID_BITS_PER_PIXEL;
    }

    w = (bmp_info_header[4] + (bmp_info_header[5] << 8) + (bmp_info_header[6] << 16) + (bmp_info_header[7] << 24));
    h = (bmp_info_header[8] + (bmp_info_header[9] << 8) + (bmp_info_header[10] << 16) + (bmp_info_header[11] << 24));

    bmp_img.resize(((w * 3 + 1) / 4) * 4 * h);
    fread(bmp_img.data(), 1, ((w * 3 + 1) / 4) * 4 * h, f);
    fclose(f);

    stride = stride ? stride : 1;

    // Limit image to framebuffer size
    w = w / stride > width ? width : w;
    h = h / stride > height ? height : h;

    if ((w > 0) && (h > 0)) {
        j = 0;
        for (y = 0; y < h; y++) {
            if (y % stride == 0) {
                for (x = 0; x < w / stride; x++) {
                    i = (y / stride + x * h / stride) * 3;

                    (framebuffer + i)[0] = bmp_img[j++];
                    (framebuffer + i)[1] = bmp_img[j++];
                    (framebuffer + i)[2] = bmp_img[j++];

                    if (stride > 1) {
                        j += x == w / stride - 1 ? 3 * (w - 1 - x * stride) : 3 * (stride - 1);
                    }
                }
            } else {
                j += 3 * w;
            }

            // padding
            // j += ((4 - (w * 3) % 4) % 4);
        }
    }

    return LOADBMP_NO_ERROR;
}

#endif

#endif